/*! -*-c++-*-
 @file   drishti/hci/FaceFinder.cpp
 @author David Hirvonen
 @brief  Face detection and tracking class with GPU acceleration.
 
 \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
 \license{This project is released under the 3 Clause BSD License.}
 
 */

#include "drishti/hci/FaceFinder.h"
#include "drishti/hci/FaceFinderImpl.h"

#include "drishti/core/drishti_operators.h"      // cv::Size * float
#include "drishti/core/make_unique.h"            // make_unique<>
#include "drishti/core/timing.h"                 // ScopeTimeLogger
#include "drishti/face/FaceDetectorAndTracker.h" // *
#include "drishti/geometry/Primitives.h"         // operator
#include "drishti/geometry/motion.h"             // transformation::
#include "drishti/hci/EyeBlob.h"                 // EyeBlobJob
#include "drishti/core/ImageView.h"
#include "drishti/ml/ObjectDetector.h"
#include "drishti/ml/ObjectDetectorACF.h"

#include <functional>
#include <deque>

#include <spdlog/fmt/ostr.h>

// clang-format off
#ifdef ANDROID
#  define TEXTURE_FORMAT GL_RGBA
#  define TEXTURE_FORMAT_IS_RGBA 1
#else
#  define TEXTURE_FORMAT GL_BGRA
#  define TEXTURE_FORMAT_IS_RGBA 0
#endif
// clang-format on

static const char* sBar = "#################################################################";

// === utility ===

using drishti::face::operator*;
using drishti::core::operator*;

DRISHTI_HCI_NAMESPACE_BEGIN

static void chooseBest(std::vector<cv::Rect>& objects, std::vector<double>& scores);
static int getDetectionImageWidth(float, float, float, float, float);

#if DRISHTI_HCI_FACEFINDER_DEBUG_PYRAMIDS
static cv::Mat draw(const acf::Detector::Pyramid& pyramid);
static void logPyramid(const std::string& filename, const acf::Detector::Pyramid& P);
#endif // DRISHTI_HCI_FACEFINDER_DEBUG_PYRAMIDS

static int modulo(int a, int b)
{
    return (((a % b) + b) % b);
}

static ogles_gpgpu::Size2d convert(const cv::Size& size)
{
    return {size.width, size.height};
}

FaceFinder::FaceFinder(FaceDetectorFactoryPtr& factory, Settings& args, void* glContext)
{
    impl = drishti::core::make_unique<Impl>(factory, args, glContext);
}

void FaceFinder::setBrightness(float value)
{
    impl->brightness = value;
}

bool FaceFinder::doAnnotations() const
{
    return impl->doAnnotations;
}

void FaceFinder::setImageLogger(const ImageLogger& logger)
{
    impl->imageLogger = logger;
}

void FaceFinder::tryEnablePlatformOptimizations()
{
    ogles_gpgpu::ACF::tryEnablePlatformOptimizations();
}

std::unique_ptr<FaceFinder>
FaceFinder::create(FaceDetectorFactoryPtr& factory, Settings& settings, void* glContext)
{
    auto finder = drishti::core::make_unique<FaceFinder>(factory, settings, glContext);
    finder->initialize();
    return finder;
}

// Initialize outside of constructor for virtual calls:
void FaceFinder::initialize()
{
    impl->hasInit = true;
    init2(*impl->factory);
    init(impl->sensor->intrinsic().getSize());
}

FaceFinder::~FaceFinder()
{
    try
    {
        if (impl && impl->scene.valid())
        {
            // If this has already been retrieved it will throw
            impl->scene.get(); // block on any abandoned calls
        }
    }
    catch (std::exception &e)
    {
        if(impl && impl->logger)
        {
            impl->logger->error("Exception {}", e.what());
        }
    }
    catch (...)
    {

    }
}

bool FaceFinder::needsDetection(const TimePoint& now) const
{
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - impl->objects.first).count();
    return (elapsed > impl->faceFinderInterval);
}

float FaceFinder::getMinDistance() const
{
    return impl->minDistanceMeters;
}

float FaceFinder::getMaxDistance() const
{
    return impl->maxDistanceMeters;
}

void FaceFinder::setDoCpuAcf(bool flag)
{
    impl->doCpuACF = flag;
}

bool FaceFinder::getDoCpuAcf() const
{
    return impl->doCpuACF;
}

void FaceFinder::setFaceFinderInterval(double interval)
{
    impl->faceFinderInterval = interval;
}

double FaceFinder::getFaceFinderInterval() const
{
    return impl->faceFinderInterval;
}

void FaceFinder::registerFaceMonitorCallback(FaceMonitor* callback)
{
    impl->faceMonitorCallback.push_back(callback);
}

void FaceFinder::dumpEyes(ImageViews& frames, EyeModelPairs& eyes, int n, bool getImage)
{
    impl->eyeFilter->dump(frames, eyes, n, getImage);
}

void FaceFinder::dumpFaces(ImageViews& frames, int n, bool getImage, int skipFrames)
{
    const auto length = static_cast<int>(impl->fifo->getBufferCount());
    
    frames.resize(std::min(static_cast<std::size_t>(n), static_cast<std::size_t>(length)));
    for (int i = 0, j = skipFrames; i < static_cast<int>(frames.size()); i++, j++)
    {
        // Reverse fifo for storage such that the largest timestamp comes first
        auto* filter = (*impl->fifo)[length - j - 1];
        const auto size = filter->getOutFrameSize();
        
        // Always assign texture (no cost)
        frames[i].texture = { { size.width, size.height }, filter->getOutputTexId() };
        
        if (getImage)
        {
            frames[i].image.create(size.height, size.width);
            filter->getResultData(frames[i].image.ptr<uint8_t>());
        }
    }
}

int FaceFinder::computeDetectionWidth(const cv::Size& inputSizeUp) const
{
    CV_Assert(impl->detector);
    
    // Some detectors may expect column major storage, so we transpose
    // the detector window dimensions for such cases here.
    auto winSize = impl->detector->getWindowSize();
    if(!impl->detector->getIsRowMajor())
    {
        std::swap(winSize.width, winSize.height);
    }
    
    const float faceWidthMeters = 0.12f; // reasonable constant
    const float fx = impl->sensor->intrinsic().m_fx;
    return getDetectionImageWidth(faceWidthMeters, fx, impl->maxDistanceMeters, winSize.width, inputSizeUp.width);
}

// Side effect: set impl->pyramdSizes
void FaceFinder::initACF(const cv::Size& inputSizeUp)
{
    // ### ACF (Transpose) ###
    // Find the detection image width required for object detection at the max distance:
    const int detectionWidth = computeDetectionWidth(inputSizeUp);
    impl->ACFScale = float(inputSizeUp.width) / float(detectionWidth);

    // ACF implementation uses reduced resolution transposed image:
    cv::Size detectionSize = inputSizeUp * (1.0f / impl->ACFScale);
    cv::Mat I(detectionSize.width, detectionSize.height, CV_32FC3, cv::Scalar::all(0));

    MatP Ip(I);
    impl->detector->computePyramid(Ip, impl->P);
    
    if(impl->P.nScales <= 0)
    {
        throw std::runtime_error("There are no valid detections scales for your provided configuration");
    }

    const auto& pChns = impl->detector->opts.pPyramid->pChns;
    const int shrink = pChns->shrink.get();

    impl->pyramidSizes.resize(impl->P.nScales);
    std::vector<ogles_gpgpu::Size2d> sizes(impl->P.nScales);
    for (int i = 0; i < impl->P.nScales; i++)
    {
        const auto size = impl->P.data[i][0][0].size();
        sizes[i] = { size.width * shrink, size.height * shrink };
        impl->pyramidSizes[i] = { size.width * shrink, size.height * shrink };

        // CPU processing works with tranposed images for col-major storage assumption.
        // Undo that here to map to row major representation.  Perform this step
        // to make transpose operation explicit.
        if(!impl->detector->getIsRowMajor())
        {
            std::swap(sizes[i].width, sizes[i].height);
            std::swap(impl->pyramidSizes[i].width, impl->pyramidSizes[i].height);
        }
    }

    const int grayWidth = impl->doLandmarks ? std::min(inputSizeUp.width, impl->landmarksWidth) : 0;
    const auto featureKind = ogles_gpgpu::getFeatureKind(*pChns);

    CV_Assert(featureKind != ogles_gpgpu::ACF::kUnknown);

    const ogles_gpgpu::Size2d size(inputSizeUp.width, inputSizeUp.height);
    impl->acf = std::make_shared<ogles_gpgpu::ACF>(impl->glContext, size, sizes, featureKind, grayWidth, shrink);
    impl->acf->setRotation(impl->outputOrientation);
    impl->acf->setLogger(impl->logger);
    impl->acf->setUsePBO((impl->glVersionMajor >= 3) && impl->usePBO);
}

// ### Fifo ###
void FaceFinder::initFIFO(const cv::Size& inputSize, std::size_t n)
{
    impl->fifo = std::make_shared<ogles_gpgpu::FifoProc>(n);
    impl->fifo->init(inputSize.width, inputSize.height, INT_MAX, false);
    impl->fifo->createFBOTex(false);
}

void FaceFinder::initBlobFilter()
{
    // ### Blobs ###
    assert(impl->eyeFilter.get());
    impl->blobFilter = drishti::core::make_unique<ogles_gpgpu::BlobFilter>();
    impl->blobFilter->init(128, 64, INT_MAX, false);
    impl->blobFilter->createFBOTex(false);

    // ### Send eye images into the blob filter ###
    impl->eyeFilter->getOutputFilter()->add(impl->blobFilter->getInputFilter());
}

void FaceFinder::initIris(const cv::Size& size)
{
    // ### Ellipsopolar warper ####
    for (auto & i : impl->ellipsoPolar)
    {
        i = std::make_shared<ogles_gpgpu::EllipsoPolarWarp>();
        i->setOutputSize(size.width, size.height);
    }
}

void FaceFinder::initEyeEnhancer(const cv::Size& inputSizeUp, const cv::Size& eyesSize)
{
    // ### Eye enhancer ###
    const auto mode = ogles_gpgpu::EyeFilter::kNone;
    const float cutoff = 0.5;

    impl->eyeFilter = drishti::core::make_unique<ogles_gpgpu::EyeFilter>(convert(eyesSize), mode, cutoff, impl->history);
    impl->eyeFilter->setAutoScaling(impl->doEyesScaling);
    impl->eyeFilter->setOutputSize(eyesSize.width, eyesSize.height);

    if (impl->doIris)
    {
        // Add a callback to retrieve updated eye models automatically:
        cv::Matx33f N = transformation::scale(0.5, 0.5) * transformation::translate(1.f, 1.f);
        for (int i = 0; i < 2; i++)
        {
            // clang-format off
            std::function<drishti::eye::EyeWarp()> eyeDelegate = [&, N, i]()
            {
                auto eye = impl->eyeFilter->getEyeWarps()[i];
                eye.eye = N * eye.H * eye.eye;
                return eye;
            };
            // clang-format on
            impl->ellipsoPolar[i]->addEyeDelegate(eyeDelegate);
            impl->eyeFilter->add(impl->ellipsoPolar[i].get());
        }
    }

    if (impl->doEyeFlow)
    { // optical flow for eyes:
        impl->eyeFlow = drishti::core::make_unique<ogles_gpgpu::FlowOptPipeline>(0.004, 1.0, false);
#if TEXTURE_FORMAT_IS_RGBA
        impl->eyeFlowBgra = drishti::core::make_unique<ogles_gpgpu::SwizzleProc>();
        impl->eyeFlow->add(impl->eyeFlowBgra.get());
        impl->eyeFlowBgraInterface = impl->eyeFlowBgra.get();
#else
        impl->eyeFlowBgraInterface = impl->eyeFlow.get();
#endif

        impl->eyeFilter->add(impl->eyeFlow.get());
    }

    impl->eyeFilter->prepare(inputSizeUp.width, inputSizeUp.height, static_cast<GLenum>(GL_RGBA));
}

void FaceFinder::initPainter(const cv::Size& /* inputSizeUp */)
{
    impl->logger->info("Init painter");
}

void FaceFinder::initFaceFilters(const cv::Size& inputSizeUp)
{
    const auto outputRenderOrientation = ::ogles_gpgpu::degreesToOrientation(360 - impl->outputOrientation);
    impl->rotater = drishti::core::make_unique<ogles_gpgpu::TransformProc>();
    impl->rotater->setOutputRenderOrientation(outputRenderOrientation);

    impl->warper = drishti::core::make_unique<ogles_gpgpu::TransformProc>();
    impl->warper->add(impl->rotater.get());
    impl->warper->prepare(inputSizeUp.width, inputSizeUp.width, GL_RGBA);
}

GLuint FaceFinder::stabilize(GLuint inputTexId, const cv::Size& inputSizeUp, const drishti::face::FaceModel& face)
{
    if (face.points.has)
    {
        const cv::Matx33f S = transformation::denormalize(inputSizeUp);
        const cv::Matx33f Sinv = S.inv();
        const cv::Matx33f H = drishti::face::FaceStabilizer::stabilize(face, inputSizeUp, 0.33);

        cv::Matx44f MVP;
        transformation::R3x3To4x4(Sinv * H * S, MVP);

        ogles_gpgpu::Mat44f MVPt{};
        cv::Mat(MVP.t()).copyTo(cv::Mat(4, 4, CV_32FC1, &MVPt.data[0][0]));
        impl->warper->setTransformMatrix(MVPt);
    }

    impl->warper->process(inputTexId, 1, GL_TEXTURE_2D);
    return impl->rotater->getOutputTexId();
}

void FaceFinder::init(const cv::Size& inputSize)
{
    //impl->logger->set_level(spdlog::level::err);
    impl->doOptimizedPipeline &= static_cast<bool>(impl->threads);
    impl->latency = impl->doOptimizedPipeline ? 2 : 0;
    impl->start = HighResolutionClock::now();

    auto inputSizeUp = inputSize;
    bool hasTranspose = ((impl->outputOrientation / 90) % 2);
    if (hasTranspose)
    {
        std::swap(inputSizeUp.width, inputSizeUp.height);
    }

    impl->faceEstimator = std::make_shared<drishti::face::FaceModelEstimator>(*impl->sensor);

    // If we are runnign an optimized pipeline there will be some latency
    // as specified by "latency" in addition to the user specified history,
    // so we must allocate our full frame FIFO large enough to store both.
    const int fullHistory = impl->history + impl->latency;
    
    initColormap();
    initACF(inputSizeUp);                     // initialize ACF first (configure opengl platform extensions)
    initFIFO(inputSizeUp, fullHistory);       // keep last N frames
    initPainter(inputSizeUp);                 // {inputSizeUp.width/4, inputSizeUp.height/4}
    initFaceFilters(inputSizeUp);             // gpu "filter" (effects)

    if (impl->doIris)
    {
        initIris({ 640, 240 });
    }

    // Must initial eye filter before blobFilter:
    initEyeEnhancer(inputSizeUp, impl->eyesSize);

    if (impl->doBlobs)
    {
        // Must initialize blobFilter after eye filter:
        initBlobFilter();
    }
}

template <typename Container>
void push_fifo(Container& container, const typename Container::value_type& value, int size)
{
    container.push_front(value);
    if (container.size() > size)
    {
        container.pop_back();
    }
}

// ogles_gpgpu::VideoSource can support list of subscribers
//
//       +=> FIFO[1][0] == ACF ====>
// VIDEO |       |
//       +=======+======== FLOW ===>

std::pair<GLuint, ScenePrimitives> FaceFinder::runFast(const FrameInput& frame2, bool doDetection)
{
    FrameInput frame1;
    frame1.size = frame2.size;

    ScenePrimitives scene2(impl->frameIndex), scene1, scene0, *outputScene = &scene2;

    if (impl->fifo->getBufferCount() > 0)
    {
        core::ScopeTimeLogger preprocessTimeLogger = [this](double t) { impl->timerInfo.acfProcessingTime = t; };

        // read GPU results for frame n-1

        // Here we always trigger GPU pipeline reads
        // to ensure upright + redeuced grayscale images will
        // be available for regression, even if we won't be using ACF detection.
        impl->acf->getChannels();

        if (impl->acf->getChannelStatus())
        {
            // If the ACF textures were loaded in the last call, then we know
            // that detections were requrested for the last frame, and we will
            // populate an ACF pyramid for the detection step.
            scene1.m_P = std::make_shared<decltype(impl->P)>();
            fill(*scene1.m_P);
        }

        // ### Grayscale image ###
        if (impl->doLandmarks)
        {
            scene1.image() = impl->acf->getGrayscale();
        }
    }

    // Start GPU pipeline for the current frame, immediately after we have
    // retrieved results for the previous frame.
    computeAcf(frame2, false, doDetection);
    GLuint texture2 = impl->acf->first()->getOutputTexId(), texture0 = 0, outputTexture = texture2;

    if (impl->fifo->getBufferCount() > 0)
    {
        if (impl->fifo->getBufferCount() >= 2)
        {
            const int index = modulo(-2, impl->fifo->getBufferCount());

            // Retrieve CPU processing for frame n-2
            scene0 = impl->scene.get();                        // scene n-2
            texture0 = (*impl->fifo)[index]->getOutputTexId(); // texture n-2
            updateEyes(texture0, scene0);                      // update the eye texture
            
            outputTexture = paint(scene0, texture0);
            outputScene = &scene0;
        }

        // Run CPU detection + regression for frame n-1
        impl->scene = impl->threads->process([scene1, frame1, this]() {
            ScenePrimitives sceneOut = scene1;
            detect(frame1, sceneOut, scene1.m_P != nullptr);
            if (doAnnotations())
            {
                // prepare line drawings for rendering while gpu is busy
                sceneOut.draw(impl->renderFaces, impl->renderPupils, impl->renderCorners);
            }
            return sceneOut;
        });
    }

    // Maintain a history for last N textures and scenes.
    // Note that with the current optimized pipeline (i.e., runFast) we introduce a latency of T=2
    // so when we the texture for time T=2 to the OpenGL FIFO we wil push the most recent available
    // scene for T-2 (T=0) to our scene buffer.
    //
    // IMAGE : { image[n-0], image[n-1], image[n-2] }
    // SCENE : { __________, __________, scene[n-2], ... }
    
    // Add the current frame to FIFO
    impl->fifo->useTexture(texture2, 1);
    impl->fifo->render();

    // Clear face motion estimate, update window
    impl->faceMotion = { 0.f, 0.f, 0.f };
    push_fifo(impl->scenePrimitives, *outputScene, impl->history);

    return std::make_pair(outputTexture, *outputScene);
}

std::pair<GLuint, ScenePrimitives> FaceFinder::runSimple(const FrameInput& frame1, bool doDetection)
{
    // Run GPU based processing on current thread and package results as a task for CPU
    // processing so that it will be available on the next frame.  This method will compute
    // ACF output using shaders on the GPU, and may optionally extract other GPU related
    // features.
    ScenePrimitives scene1(impl->frameIndex), *outputScene = nullptr; // time: n+1 and n
    preprocess(frame1, scene1, doDetection);

    // Initialize input texture with ACF upright texture:
    GLuint texture1 = impl->acf->first()->getOutputTexId(), outputTexture = 0;

    detect(frame1, scene1, doDetection);

    if (doAnnotations())
    {
        updateEyes(texture1, scene1);

        // Excplicit output variable configuration:
        scene1.draw(impl->renderFaces, impl->renderPupils, impl->renderCorners);
        outputTexture = paint(scene1, texture1); // was 1
        outputScene = &scene1;
    }
    else
    {
        outputTexture = texture1;
        outputScene = &scene1; // empty
    }

    // Add the current frame to FIFO
    impl->fifo->useTexture(texture1, 1);
    impl->fifo->render();

    // Clear face motion estimate, update window:
    impl->faceMotion = { 0.f, 0.f, 0.f };
    push_fifo(impl->scenePrimitives, *outputScene, impl->history);

    return std::make_pair(outputTexture, *outputScene);
}

GLuint FaceFinder::operator()(const FrameInput& frame1)
{
    // clang-format off
    std::string methodName = DRISHTI_LOCATION_SIMPLE;
    core::ScopeTimeLogger faceFinderTimeLogger = [this, methodName](double elapsed)
    {
        if (impl->logger)
        {
            impl->logger->info("TIMING:{} : {} full={}", methodName, impl->timerInfo, elapsed);
        }
    };
    // clang-format on

    // Get current timestamp
    const auto& now = faceFinderTimeLogger.getTime();
    const bool doDetection = needsDetection(now);

    GLuint outputTexture = 0;
    ScenePrimitives outputScene;
    if (impl->doOptimizedPipeline)
    {
        std::tie(outputTexture, outputScene) = runFast(frame1, doDetection);
    }
    else
    {
        std::tie(outputTexture, outputScene) = runSimple(frame1, doDetection);
    }

    if (impl->imageLogger && outputScene.faces().size() && !outputScene.image().empty())
    {
        (impl->imageLogger)(outputScene.image());
    }

    impl->frameIndex++; // increment frame index

    if (impl->scenePrimitives.size() >= 2)
    {
        if (impl->scenePrimitives[0].faces().size() && impl->scenePrimitives[1].faces().size())
        {
            const auto& face0 = impl->scenePrimitives[0].faces()[0];
            const auto& face1 = impl->scenePrimitives[1].faces()[0];
            const cv::Point3f delta = (*face0.eyesCenter - *face1.eyesCenter);
            impl->faceMotion = delta; // active face motion
        }
    }

    try
    {
        notifyListeners(outputScene, now, impl->fifo->isFull(), outputTexture);
    }
    catch (...)
    {
        // noop
    }

    return outputTexture;
}

/**
 * Provide per frame scene descriptor callbacks.
 * Notably, if a face requet listener is registered,
 * then a response is expected on a per frame basis.
 *
 * Maintain a history for last N textures and scenes.
 * Note that with the current optimized pipeline (i.e., runFast) we introduce a latency of T=2
 * so when we the texture for time T=2 to the OpenGL FIFO we wil push the most recent available
 * scene for T-2 (T=0) to our scene buffer.
 *
 * IMAGE : { image[n-0], image[n-1], image[n-2] }
 * SCENE : { __________, __________, scene[n-2], ... }
 */

void FaceFinder::notifyListeners
(
    const ScenePrimitives& scene,
    const TimePoint& now,
    bool isInit,
    std::uint32_t tex
)
{
    // Perform optional frame grabbing
    // NOTE: This must occur in the main OpenGL thread:
    std::vector<FaceMonitor::FaceImage> frames;

    // Build a list of active requests:
    std::vector<FaceMonitor::Request> requests(impl->faceMonitorCallback.size());

    FaceMonitor::Request request{0,false,false,false,false};

    // Check each listener's request() methods to see if any images are requested
    // and format a request as the union of all individual requests:
    for (int i = 0; i < impl->faceMonitorCallback.size(); i++)
    {
        auto& callback = impl->faceMonitorCallback[i];
        requests[i] = callback->request(scene.faces(), now, tex);
        request |= requests[i]; // accumulate requests
    }

    // Clip request to available history.  The ignoreLatestFramesInMonitor
    // state flag is specific to the optimized pipeline (w/ built-in latency)
    // and indicates that the user is not interested in viewing the latest
    // frames (i.e., first two) from the FIFO that have no associated metadata.
    // When we are computing the global request (from all registered listeners)
    // we need to take this into account, as it will impact the Request::n
    // parameter.  For example, if the full frame FIFO contains 5 frames,
    // and our GPU latency is 2 frames, then the most a user can request is
    // just 3 frames.
    
    int skipFrames = 0;
    int metadataOffset = impl->latency;
    auto availableFrameCount = static_cast<int>(impl->fifo->getBufferCount());
    if(impl->ignoreLatestFramesInMonitor)
    {
        skipFrames = impl->latency;
        metadataOffset = 0;
        availableFrameCount -= skipFrames;
    }
    
    if(request.n > availableFrameCount)
    {
        request.n = availableFrameCount;
    }
    
    // If a frame was requested we'll have to pull down data from the GPU here:
    // * eye buffer: eye crops may be requested
    // * face buffer: face crops may be requested
    
    // We have 4 separate buffers that need to synchronized and merged
    // at each time step.
    //
    // Sizes:
    //   N1 = impl->history + impl->latency
    //   N2 = impl->history
    //
    // Buffers: impl-> ...
    //   fifo                            ; [1][2][3][4][5] ; N1
    //   scenePrimitives                 ; [1][2][3]       ; N2
    //   eyeFilter->m_impl->fifoProc     ; [1][2][3]       ; N2
    //   eyeFilter->m_impl->m_eyeHistory ; [1][2][3]       ; N2
    
    if (request.n > 0)
    {
        frames.resize(request.n);
        if (request.getFrames)
        {
            // ### collect face images ###
            std::vector<core::ImageView> faces;
            dumpFaces(faces, request.n, request.getImage, skipFrames);
            if (faces.size() && request.getFrames)
            {
                const int length = static_cast<int>(std::min(faces.size(), frames.size()));
                for (int i = 0; i < length; i++)
                {
                    frames[i].image = faces[i];
                    if(i >= metadataOffset)
                    {
                        const int index = i - impl->latency;
                        frames[i].faceModels = impl->scenePrimitives[index].faces();
                    }
                }
            }
        }
        
        if (request.getEyes)
        {
            // ### collect eye images ###
            std::vector<core::ImageView> eyes;
            std::vector<std::array<eye::EyeModel, 2>> eyePairs;
            const int numberOfSynchronizedEyes = std::max(request.n - impl->latency + skipFrames, 0);
            if(numberOfSynchronizedEyes)
            {
                dumpEyes(eyes, eyePairs, numberOfSynchronizedEyes, request.getImage);
                for (int i = 0; i < static_cast<int>(std::min(eyes.size(), frames.size())); i++)
                {
                    frames[i + metadataOffset].eyes = eyes[i];
                    frames[i + metadataOffset].eyeModels = eyePairs[i];
                }
            }
        }
    }

    // Pass down all of the populated face images pointers w/ associated models
    for (int i = 0; i < impl->faceMonitorCallback.size(); i++)
    {
        if (requests[i].n)
        {
            impl->faceMonitorCallback[i]->grab(frames, isInit);
        }
        else
        {
            impl->faceMonitorCallback[i]->grab({}, isInit);
        }
    }
}

GLuint FaceFinder::paint(const ScenePrimitives& scene, GLuint inputTexture)
{
    return inputTexture;
}

void FaceFinder::initColormap()
{
    // ##### Create a colormap ######
    cv::Mat1b colorsU8(1, 360);
    cv::Mat3b colorsU8C3;
    cv::Mat3f colors32FC3;
    for (int i = 0; i < 360; i++)
    {
        colorsU8(0, i) = uint8_t(255.f * float(i) / (colorsU8.cols - 1) + 0.5f);
    }

    cv::applyColorMap(colorsU8, colorsU8C3, cv::COLORMAP_HSV);
    colorsU8C3.convertTo(impl->colors32FC3, CV_32FC3, 1.0 / 255.0);
}

void FaceFinder::computeAcf(const FrameInput& frame, bool doLuv, bool doDetection)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDepthMask(GL_FALSE);

    impl->acf->setDoLuvTransfer(doLuv);
    impl->acf->setDoAcfTransfer(doDetection);

    (*impl->acf)(frame);
}

/*
 * Create full ACF pyramid on GPU
 */

std::shared_ptr<acf::Detector::Pyramid> FaceFinder::createAcfGpu(const FrameInput& frame, bool doDetection)
{
    computeAcf(frame, false, doDetection);

    std::shared_ptr<decltype(impl->P)> P;

    // Here we always trigger channel processing
    // to ensure grayscale images will be available
    // for regression, even if we won't be using ACF detection.
    cv::Mat acf = impl->acf->getChannels();

    if (doDetection)
    {
        assert(acf.type() == CV_8UC1);
        assert(acf.channels() == 1);

        if (impl->acf->getChannelStatus())
        {
            P = std::make_shared<decltype(impl->P)>();
            fill(*P);

#if DRISHTI_HCI_FACEFINDER_DEBUG_PYRAMIDS
            std::string home = getenv("HOME");
            home += "/Documents/";

            cv::Mat channels = impl->acf->getChannels();
            cv::imwrite(home + "/acf_gpu.png", channels);
            logPyramid(home + "/Pgpu.png", *P);
#endif
        }
    }

    return P;
}

/*
 * Create LUV images in OpenGL ES and uses this as input for
 * CPU based ACF pyramid construction.
 */

std::shared_ptr<acf::Detector::Pyramid> FaceFinder::createAcfCpu(const FrameInput& frame, bool doDetection)
{
    computeAcf(frame, true, doDetection);

    std::shared_ptr<decltype(impl->P)> P;
    if (doDetection)
    {
        cv::Mat acf = impl->acf->getChannels();
        assert(acf.type() == CV_8UC1);
        assert(acf.channels() == 1);

        P = std::make_shared<decltype(impl->P)>();

        MatP LUVp = impl->acf->getLuvPlanar();
        impl->detector->setIsLuv(true);
        impl->detector->setIsTranspose(true);
        impl->detector->computePyramid(LUVp, *P);

#if DRISHTI_HCI_FACEFINDER_DEBUG_PYRAMIDS
        std::string home = getenv("HOME");
        home += "/Documents/";
        logPyramid(home + "/Pcpu.png", *P);
#endif
    }

    return P;
}

/**
 * GPU preprocessing:
 * (1) FrameInpute -> texture -> ACF output image (P pyramid)
 * (2) Harris/Shi-Tomasi corners
 * (3) Resized grayscale image for face landmarks
 */

void FaceFinder::preprocess(const FrameInput& frame, ScenePrimitives& scene, bool doDetection)
{
    const auto tag = DRISHTI_LOCATION_SIMPLE;
    std::stringstream ss;
    core::ScopeTimeLogger scopeTimeLogger = [&](double t) { impl->logger->info("TIMING:{}{}total={}", tag, ss.str(), t); };

    if (impl->doCpuACF)
    {
        scene.m_P = createAcfCpu(frame, doDetection);
    }
    else
    {
        core::ScopeTimeLogger scopeTimeLogger = [&](double t) { ss << "acf=" << t << ";"; };
        scene.m_P = createAcfGpu(frame, doDetection);
    }

    // ### Grayscale image ###
    if (impl->doLandmarks)
    {
        core::ScopeTimeLogger scopeTimeLogger = [&](double t) { ss << "gray=" << t << ";"; };
        scene.image() = impl->acf->getGrayscale();
    }
}

void FaceFinder::fill(acf::Detector::Pyramid& P)
{
    impl->acf->fill(P, impl->P);
}

int FaceFinder::detectOnly(ScenePrimitives& scene, bool doDetection)
{
    // Fill in ACF Pyramid structure.
    // Check to see if detection was already computed
    if (doDetection)
    {
        core::ScopeTimeLogger scopeTimeLogger = [this](double t) { impl->timerInfo.detectionTimeLogger(t); };
        std::vector<double> scores;
        (*impl->detector)(*scene.m_P, scene.objects(), &scores);
        if (impl->doSingleFace)
        {
            chooseBest(scene.objects(), scores);
        }
        impl->objects = std::make_pair(HighResolutionClock::now(), scene.objects());
    }
    else
    {
        scene.objects() = impl->objects.second;
    }

    return scene.objects().size();
}

void FaceFinder::scaleToFullResolution(std::vector<drishti::face::FaceModel>& faces)
{
    const float Srf = 1.0f / impl->acf->getGrayscaleScale();
    const cv::Matx33f Hrf = transformation::scale(Srf);
    for (auto& f : faces)
    {
        f = Hrf * f;

        // Tag each face w/ approximate distance:
        if (impl->faceEstimator)
        {
            if (f.eyeFullL.has && f.eyeFullR.has)
            {
                f.eyesCenter = (*impl->faceEstimator)(f);
            }
        }
    }
}

int FaceFinder::detect(const FrameInput& frame, ScenePrimitives& scene, bool doDetection)
{
    //impl->logger->set_level(spdlog::level::off);
    core::ScopeTimeLogger scopeTimeLogger = [this](double t) {
        impl->logger->info("FULL_CPU_PATH: {}", t);
    };

    // Start with empty face detections:
    std::vector<drishti::face::FaceModel> faces;
    drishti::face::FaceDetector::PaddedImage Ib(scene.image(), { { 0, 0 }, scene.image().size() });

    if (impl->detector && (!doDetection || scene.m_P))
    {
        if (!scene.objects().size())
        {
            detectOnly(scene, doDetection);
        }
        if (impl->doLandmarks && scene.objects().size())
        {
            const auto& objects = scene.objects();
            faces.resize(objects.size());
            for (int i = 0; i < faces.size(); i++)
            {
                faces[i].roi = objects[i];
            }

            //impl->imageLogger(gray);
            const bool isDetection = true;
            const float Sdr = impl->ACFScale * impl->acf->getGrayscaleScale();
            const cv::Matx33f Hdr = transformation::scale(Sdr);
            impl->faceDetector->setDoIrisRefinement(true);
            impl->faceDetector->refine(Ib, faces, Hdr, isDetection);

            // Scale faces from regression to level 0.
            // The configuration sizes used in the ACF stacked channel image
            // are all upright, but the output texture used for the display
            // is still in the native (potentially rotated) coordinate system,
            // so we need to perform scaling wrt that.

            scaleToFullResolution(faces);
        }
    }

    {
        // Perform simple prediction on every frame.  This occurs on full resolution
        // FaceModel objects, for which approximate location is known.  In some cases
        // we may need to update landmarks for a track prediction which had no corresponding
        // detection assignment for this frame, in which case we must:
        //   1) map to regression image resolution
        //   2) refine the face model
        //   3) map back to the full resolution image
        
        const float Sfr = impl->acf->getGrayscaleScale(); // full->regression
        const cv::Matx33f Hfr = transformation::scale(Sfr);
        drishti::face::FaceTracker::FaceTrackVec tracksOut;
        (*impl->faceTracker)(faces, tracksOut);
        
        x = (*impl->faceTracker).x;
        y = (*impl->faceTracker).y;

        // Clear the face vector and use this to store tracks that don't receive
        // detection assignments -- we will need to refine the landmarks separately
        faces.clear();
        for (auto& f : tracksOut)
        {
            if (f.second.misses > 0)
            {
                // Here we map tracks that received no detection assignment to
                // the regression image resolution for subsequent refinement.
                faces.push_back(Hfr * f.first);
            }
            else
            {
                // Each track that was assigned to a detection will have valid
                // landmarks and can simply be copied to the face vector.
                scene.faces().emplace_back(f.first);
            }
        }

        // Finally we run the landmark refinement at the operative resolution
        // for all the unassigned tracks.
        if(faces.size())
        {
            impl->faceDetector->refine(Ib, faces, cv::Matx33f::eye(), false);
            scaleToFullResolution(faces);
            for (auto& f : faces) 
            {
                scene.faces().emplace_back(f);
            }
        }

        // Sort near to far:
        std::sort(scene.faces().begin(), scene.faces().end(), [](const face::FaceModel& a, const face::FaceModel& b) {
            return (a.eyesCenter->z < b.eyesCenter->z);
        });
    }

    return 0;
}

void FaceFinder::updateEyes(GLuint inputTexId, const ScenePrimitives& scene)
{
    core::ScopeTimeLogger updateEyesLoge = [this](double t) { impl->logger->info("FaceFinder::updateEyes={}", t); };

    if (scene.faces().size())
    {
        for (const auto& f : scene.faces())
        {
            impl->eyeFilter->addFace(f);
        }
    }
    else
    {
        impl->eyeFilter->clearFaces();
    }

    // Trigger eye enhancer, triggers blob filter:
    impl->eyeFilter->process(inputTexId, 1, GL_TEXTURE_2D);
    
    if (scene.faces().size())
    {
        // Limit to points on iris:
        const auto& eyeWarps = impl->eyeFilter->getEyeWarps();

        if (impl->doEyeFlow)
        { // Grab optical flow results:
            const auto flowSize = impl->eyeFlowBgraInterface->getOutFrameSize();
            cv::Mat4b ayxb(flowSize.height, flowSize.width);
            impl->eyeFlowBgraInterface->getResultData(ayxb.ptr());

            // Extract corners in bottom half of image
            cv::Mat1b corners;
            cv::extractChannel(ayxb, corners, 0);
            std::vector<FeaturePoint> features;
            extractPoints(corners, features, 1.f);

            impl->eyeFlowField.clear();
            if (features.size())
            {
                cv::Matx33f Heye[2] = {
                    eyeWarps[0].H.inv() * transformation::normalize(ayxb.size()),
                    eyeWarps[1].H.inv() * transformation::normalize(ayxb.size())
                };

                std::vector<cv::Point2f> flow;
                for (const auto& f : features)
                {
                    // Extract flow:
                    const cv::Vec4b& pixel = ayxb(f.point.y, f.point.x);
                    cv::Point2f p(pixel[2], pixel[1]);
                    cv::Point2f d = (p * (2.0f / 255.0f)) - cv::Point2f(1.0f, 1.0f);
                    flow.push_back(d);

                    cv::Point3f q3 = Heye[p.x > ayxb.cols / 2] * f.point;
                    cv::Point2f q2(q3.x / q3.z, q3.y / q3.z);
                    impl->eyeFlowField.emplace_back(q2.x, q2.y, d.x * 100.f, d.y * 100.f);
                }
                impl->eyeMotion = -drishti::geometry::pointMedian(flow);
            }
        }

        if (impl->blobFilter)
        { // Grab reflection points for eye tracking etc:
            core::ScopeTimeLogger scopeTimeLogger = [this](double t) { this->impl->timerInfo.blobExtractionTimeLogger(t); };

            const cv::Size filteredEyeSize(impl->blobFilter->getOutFrameW(), impl->blobFilter->getOutFrameH());
            EyeBlobJob single(filteredEyeSize, eyeWarps);
            impl->blobFilter->getHessianPeaks()->getResultData(single.filtered.ptr());
            single.run();
            impl->eyePoints = single.eyePoints;

            computeGazePoints();
        }
    }
}

void FaceFinder::computeGazePoints()
{
    // Convert points to polar coordinates:
    impl->gazePoints.clear();

    const auto& eyeWarps = impl->eyeFilter->getEyeWarps();

    float total = 0.f;
    cv::Point2f mu;
    for (int i = 0; i < 2; i++)
    {
        for (const auto& p : impl->eyePoints[i])
        {
            // Find iris center relative to specular reflection:
            const auto& iris = eyeWarps[i].eye.irisEllipse;
            const cv::Point2f q = (iris.center - p.point);

            // Transform to unit circle:
            const float rho = cv::norm(q) / (iris.size.width * 0.5f);
            const cv::Point2f qi = cv::normalize(cv::Vec2f(q)) * rho;

            // Project points to unit circle:
            FeaturePoint gaze(qi, p.radius);
            impl->gazePoints.push_back(gaze);

            // Compute mean point:
            const float weight = p.radius;
            mu += qi * weight;
            total += weight;
        }
    }
    if (total > 0.f)
    {
        mu *= (1.0 / total);
        impl->logger->info("GAZE: {:f}", mu);
    }
}

void FaceFinder::initTimeLoggers()
{
    // clang-format off
    impl->timerInfo.init();
    // clang-format on
}

// #### init2 ####

void FaceFinder::init2(drishti::face::FaceDetectorFactory& resources)
{
    using drishti::face::FaceSpecification;
    
    impl->logger->info("FaceFinder::init2() {}", sBar);
    impl->logger->info("{}", resources);

    initTimeLoggers();

#if DRISHTI_HCI_FACEFINDER_DO_TRACKING
    // Insntiate a face detector w/ a tracking component:
    auto faceDetectorAndTracker = drishti::core::make_unique<drishti::face::FaceDetectorAndTracker>(resources);
    faceDetectorAndTracker->setMaxTrackAge(2.0);
    impl->faceDetector = std::move(faceDetectorAndTracker);
#else
    impl->faceDetector = drishti::core::make_unique<drishti::face::FaceDetector>(resources);
#endif
    
    impl->faceDetector->setLandmarkFormat( impl->factory->inner ? FaceSpecification::kibug68_inner : FaceSpecification::kibug68);    
    impl->faceDetector->setDoNMSGlobal(impl->doSingleFace); // single detection only
    impl->faceDetector->setDoNMS(true);
    impl->faceDetector->setInits(1);

    // Get weak ref to underlying ACF detector
    auto *detector = dynamic_cast<ml::ObjectDetectorACF *>(impl->faceDetector->getDetector());
    impl->detector = dynamic_cast<acf::Detector*>(detector->getDetector());
    
    if (impl->detector)
    {
        if (impl->acfCalibration != 0.f)
        {
            // Perform modification
            acf::Detector::Modify dflt;
            dflt.cascThr = { "cascThr", -1.0 };
            dflt.cascCal = { "cascCal", impl->acfCalibration };
            impl->detector->acfModify(dflt);
        }
    }

    impl->faceDetector->setDetectionTimeLogger(impl->timerInfo.detectionTimeLogger);
    impl->faceDetector->setRegressionTimeLogger(impl->timerInfo.regressionTimeLogger);
    impl->faceDetector->setEyeRegressionTimeLogger(impl->timerInfo.eyeRegressionTimeLogger);

    {
        // FaceDetection mean:
        drishti::face::FaceModel faceDetectorMean = impl->factory->getMeanFace();

        // We can change the regressor crop padding by doing a centered scaling of face features:
        if (impl->regressorCropScale > 0.f)
        {
            // clang-format off
            std::vector<cv::Point2f> centers
            {
                faceDetectorMean.getEyeRightCenter(),
                faceDetectorMean.getEyeLeftCenter(),
                *faceDetectorMean.noseTip,
            };
            
            if(!impl->factory->inner)
            {
                centers.push_back(*faceDetectorMean.mouthCornerRight);
                centers.push_back(*faceDetectorMean.mouthCornerLeft);
            }

            // clang-format on
            const cv::Point2f center = core::centroid(centers);
            const cv::Matx33f H = transformation::scale(impl->regressorCropScale, impl->regressorCropScale, center);
            faceDetectorMean = H * faceDetectorMean;
        }

        impl->faceDetector->setFaceDetectorMean(faceDetectorMean);
    }

    impl->faceTracker = core::make_unique<face::FaceTracker>(
        impl->minFaceSeparation,
        impl->minTrackHits,
        impl->maxTrackMisses);
}

static void smooth(double& t0, double t1, double alpha = 0.95)
{
    t0 = (t0 != 0.0) ? t1 : ((t0 * alpha) + (t1 * (1.0 - alpha)));
}

void FaceFinder::TimerInfo::init()
{
    // clang-format off
    detectionTimeLogger = [this](double seconds) { smooth(detectionTime, seconds); };
    regressionTimeLogger = [this](double seconds) { smooth(regressionTime, seconds); };
    eyeRegressionTimeLogger = [this](double seconds) { smooth(eyeRegressionTime, seconds); };
    acfProcessingTimeLogger = [this](double seconds) { smooth(acfProcessingTime, seconds); };
    blobExtractionTimeLogger = [this](double seconds) { smooth(blobExtractionTime, seconds); };
    renderSceneTimeLogger = [this](double seconds) { smooth(renderSceneTime, seconds); };
    // clang-format on
}

// #### utilty: ####

static void chooseBest(std::vector<cv::Rect>& objects, std::vector<double>& scores)
{
    if (objects.size() > 1)
    {
        int best = 0;
        for (int i = 1; i < objects.size(); i++)
        {
            if (scores[i] > scores[best])
            {
                best = i;
            }
        }
        objects = { objects[best] };
        scores = { scores[best] };
    }
}

std::ostream& operator<<(std::ostream& os, const FaceFinder::TimerInfo& info)
{
    double total = info.detectionTime + info.regressionTime + info.eyeRegressionTime;
    os << " acf+=" << info.acfProcessingTime
       << " fd=" << info.detectionTime
       << " fr=" << info.regressionTime
       << " er=" << info.eyeRegressionTime
       << " blob=" << info.blobExtractionTime
       << " gl=" << info.renderSceneTime
       << " total=" << total;
    return os;
}

static int
getDetectionImageWidth(float objectWidthMeters, float fxPixels, float zMeters, float winSizePixels, float imageWidthPixels)
{
    // objectWidthPixels / fxPixels = objectWidthMeters / zMeters
    // objectWidthPixels = (fxPixels * objectWidthMeters) / zMeters
    const float objectWidthPixels = (fxPixels * objectWidthMeters) / zMeters;

    // objectWidthPixels / imageWidthPixels = winSizePixels / N
    // N = winSizePixels / (objectWidthPixels / imageWidthPixels);
    return winSizePixels / (objectWidthPixels / imageWidthPixels);
}

#if DRISHTI_HCI_FACEFINDER_DEBUG_PYRAMIDS

static cv::Mat draw(const acf::Detector::Pyramid& pyramid)
{
    cv::Mat canvas;
    std::vector<cv::Mat> levels;
    for (int i = 0; i < pyramid.nScales; i++)
    {
        // Concatenate the transposed faces, so they are compatible with the GPU layout
        cv::Mat Ccpu;
        std::vector<cv::Mat> images;
        for (const auto& image : pyramid.data[i][0].get())
        {
            images.push_back(image.t());
        }
        cv::vconcat(images, Ccpu);

        // Instead of upright:
        //cv::vconcat(pyramid.data[i][0].get(), Ccpu);

        if (levels.size())
        {
            cv::copyMakeBorder(Ccpu, Ccpu, 0, levels.front().rows - Ccpu.rows, 0, 0, cv::BORDER_CONSTANT);
        }

        levels.push_back(Ccpu);
    }
    cv::hconcat(levels, canvas);
    return canvas;
}

static void logPyramid(const std::string& filename, const acf::Detector::Pyramid& P)
{
    cv::Mat canvas = draw(P);
    cv::imwrite(filename, canvas);
}
#endif // DRISHTI_HCI_FACEFINDER_DEBUG_PYRAMIDS

DRISHTI_HCI_NAMESPACE_END
