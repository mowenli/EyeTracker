/*! -*-c++-*-
  @file   test-FaceTracker.cpp
  @author David Hirvonen
  @brief  Google test for public drishti API FaceTracker interface.

  \copyright Copyright 2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <gtest/gtest.h>

#include "drishti/drishti/FaceTracker.hpp"
#include "drishti/drishti/Context.hpp"
#include "drishti/drishti/drishti_cv.hpp"
#include "drishti/core/Logger.h"
#include "drishti/core/ThreadPool.h"
#include "drishti/drishti/Sensor.hpp"

// clang-format off
#if defined(DRISHTI_DO_GPU_TESTING)
#  include "aglet/GLContext.h"
#endif
// clang-format on

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <memory>
#include <fstream>

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

extern const char* sFaceDetector;
extern const char* sFaceDetectorMean;
extern const char* sFaceRegressor;
extern const char* sEyeRegressor;
extern const char* sFaceImageFilename;

// clang-format off
#define BEGIN_EMPTY_NAMESPACE namespace {
#define END_EMPTY_NAMESPACE }
// clang-format on

BEGIN_EMPTY_NAMESPACE

class FaceTest : public ::testing::Test
{
public:

    FaceTest(const FaceTest&) = delete;
    FaceTest(FaceTest&&) = delete;
    FaceTest& operator=(const FaceTest&) = delete;
    FaceTest& operator=(FaceTest&&) = delete;

protected:
    bool m_hasTranspose = false;

    // Setup
    FaceTest()
    {
        m_logger = drishti::core::Logger::create("test-drishti-drishti-face");
        m_logger->set_level(spdlog::level::off); // by default...

        // Load the ground truth data:
        image = loadImage(sFaceImageFilename);

#if defined(DRISHTI_DO_GPU_TESTING)
        m_context = aglet::GLContext::create(aglet::GLContext::kAuto);
#if defined(_WIN32) || defined(_WIN64)
        CV_Assert(!glewInit());
#endif
#endif
        // TODO: we need to load ground truth output for each shader
        // (some combinations could be tested, but that is probably excessive!)
        //truth = loadImage(truthFilename);
    }

    // Cleanup
    ~FaceTest() override
    {
        drishti::core::Logger::drop("test-drishti-drishti-face");
    }

    /*
     * FaceFinder configuration can be achieved by modifying the input
     * configurations stored as member variables in the test fixture prior
     * to calling create()
     * 
     * @param size  : size of input frames for detector
     * @orientation : orientation of input frames
     * @doThreads   : support testing with and without threadpool
     */
    std::shared_ptr<drishti::sdk::FaceTracker> create(const cv::Size& size, int orientation, bool doThreads)
    {
        const float fx = size.width;
        const drishti::sdk::Vec2f p(image.cols / 2, image.rows / 2);
        drishti::sdk::SensorModel::Intrinsic intrinsic(p, fx, { size.width, size.height });
        drishti::sdk::Matrix33f I = drishti::sdk::Matrix33f::eye();
        drishti::sdk::SensorModel::Extrinsic extrinsic(I);
        drishti::sdk::SensorModel sensor(intrinsic, extrinsic);

        drishti::sdk::Context context(sensor);

        /*
         * Lazy construction requires that streams are in scope at the time of construction:
         */

        std::ifstream iFaceDetector(sFaceDetector, std::ios_base::binary);
        if (!iFaceDetector)
        {
            throw std::runtime_error("FaceTest::create() failed to open face detector");
        }

        std::ifstream iFaceRegressor(sFaceRegressor, std::ios_base::binary);
        if (!iFaceRegressor)
        {
            throw std::runtime_error("FaceTest::create() failed to open face regressor");
        }

        std::ifstream iEyeRegressor(sEyeRegressor, std::ios_base::binary);
        if (!iEyeRegressor)
        {
            throw std::runtime_error("FaceTest::create() failed to open eye regressor");
        }

        std::ifstream iFaceDetectorMean(sFaceDetectorMean, std::ios_base::binary);
        if (!iFaceDetectorMean)
        {
            throw std::runtime_error("FaceTest::create() failed to open face mean");
        }

        assert(iFaceDetector.good());
        assert(iFaceRegressor.good());
        assert(iEyeRegressor.good());
        assert(iFaceDetectorMean.good());

        drishti::sdk::FaceTracker::Resources factory;
        factory.sFaceDetector = &iFaceDetector;
        factory.sFaceRegressor = &iFaceRegressor;
        factory.sEyeRegressor = &iEyeRegressor;
        factory.sFaceModel = &iFaceDetectorMean;

        return std::make_shared<drishti::sdk::FaceTracker>(&context, factory);
    }

#if defined(DRISHTI_DO_GPU_TESTING)
    void runTest(bool doCpu, bool doAsync)
    {
        // Instantiate a face finder and register a callback:
        auto tracker = create(image.size(), 0, doAsync);

        ASSERT_TRUE(tracker->good());

        drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);

        const int iterations = 3;
        for (int i = 0; i < iterations; i++)
        {
            (*tracker)(frame);
        }
    }
#endif

#if defined(DRISHTI_BUILD_C_INTERFACE)
    std::shared_ptr<drishti::sdk::FaceTracker> createC(const cv::Size& size, int orientation, bool doThreads)
    {
        const float fx = size.width;
        const drishti::sdk::Vec2f p(image.cols / 2, image.rows / 2);
        drishti::sdk::SensorModel::Intrinsic intrinsic(p, fx, { size.width, size.height });

        drishti::sdk::Matrix33f R;
        drishti::sdk::SensorModel::Extrinsic extrinsic(R);
        drishti::sdk::SensorModel sensor(intrinsic, extrinsic);

        drishti::sdk::Context context(sensor);
        context.setMinDetectionDistance(0.0);
        context.setMaxDetectionDistance(1.0);

        std::ifstream iFaceDetector(sFaceDetector, std::ios_base::binary);
        std::ifstream iFaceRegressor(sFaceRegressor, std::ios_base::binary);
        std::ifstream iEyeRegressor(sEyeRegressor, std::ios_base::binary);
        std::ifstream iFaceDetectorMean(sFaceDetectorMean, std::ios_base::binary);

        drishti::sdk::FaceTracker::Resources factory;
        factory.sFaceDetector = &iFaceDetector;
        factory.sFaceRegressor = &iFaceRegressor;
        factory.sEyeRegressor = &iEyeRegressor;
        factory.sFaceModel = &iFaceDetectorMean;

        auto tracker = drishti_face_tracker_create_from_streams(&context, factory);

        if (tracker)
        {
            if (tracker->good())
            {
                return std::shared_ptr<drishti::sdk::FaceTracker>(tracker, drishti_face_tracker_destroy);
            }
            else
            {
                delete tracker;
            }
            return nullptr;
        }
        else
        {
            m_logger->error("Unable to instantiate face tracker");
            return nullptr;
        }
    }

    int callback(drishti_face_tracker_results_t& results)
    {
        m_logger->info("callback: Received results");
        return 0;
    }

    drishti_request_t trigger(const drishti_face_tracker_result_t& faces, double timestamp)
    {
        m_logger->info("trigger: Received results at time {}}", timestamp);
        // if(some_condition_is_true(faces)) {
        return { 3, true, true, true, true }; // request last 3 images and textures
        // }
    }

    int allocator(const drishti_image_t& spec, drishti::sdk::Image4b& image)
    {
        m_logger->info("allocator: {} {}", spec.width, spec.height);
        return 0;
    }

    /*
     * C API:
     */

    static int callbackFunc(void* context, drishti_face_tracker_results_t& results)
    {
        if (FaceTest* ft = static_cast<FaceTest*>(context))
        {
            return ft->callback(results);
        }
        return -1;
    }

    static drishti_request_t triggerFunc(void* context, const drishti_face_tracker_result_t& faces, double timestamp)
    {
        if (FaceTest* ft = static_cast<FaceTest*>(context))
        {
            return ft->trigger(faces, timestamp);
        }
        return { 0, false, false };
    }

    static int allocatorFunc(void* context, const drishti_image_t& spec, drishti::sdk::Image4b& image)
    {
        if (FaceTest* ft = static_cast<FaceTest*>(context))
        {
            return ft->allocator(spec, image);
        }
        return -1;
    }

    void runTestC(bool doCpu, bool doAsync)
    {
        // Instantiate a face finder and register a callback:
        auto tracker = createC(image.size(), 0, doAsync);

        // clang-format off
        drishti_face_tracker_t table
        {
            this,
            triggerFunc,
            callbackFunc,
            allocatorFunc
        };
        // clang-format on

        drishti_face_tracker_callback(tracker.get(), table);

        drishti::sdk::VideoFrame frame({ image.cols, image.rows }, image.ptr(), true, 0, DFLT_TEXTURE_FORMAT);

        const int iterations = 3;
        for (int i = 0; i < iterations; i++)
        {
            drishti_face_tracker_track(tracker.get(), frame);
        }
    }
#endif // DRISHTI_BUILD_C_INTERFACE

    // Called after constructor for each test
    void SetUp() override {}

    // Called after destructor for each test
    void TearDown() override {}

    static cv::Mat loadImage(const std::string& filename)
    {
        assert(!filename.empty());
        cv::Mat image = cv::imread(filename, cv::IMREAD_COLOR);

        assert(!image.empty() && image.type() == CV_8UC3);
        cv::Mat tmp;
        cv::cvtColor(image, tmp, cv::COLOR_BGR2BGRA);
        cv::swap(image, tmp);
        return image;
    }

#if defined(DRISHTI_DO_GPU_TESTING)
    std::shared_ptr<aglet::GLContext> m_context;
#endif

    void* m_glContext = nullptr;
    std::shared_ptr<spdlog::logger> m_logger;

    // Test images:
    cv::Mat image, truth;
};

#if defined(DRISHTI_DO_GPU_TESTING)
TEST_F(FaceTest, RunSimpleTest) // NOLINT (TODO)
{
    static const bool doCpu = false;
    static const bool doAsync = true;
    runTest(doCpu, doAsync);
}
#endif

#if defined(DRISHTI_DO_GPU_TESTING) && defined(DRISHTI_BUILD_C_INTERFACE)
TEST_F(FaceTest, RunSimpleTestC)
{
    static const bool doCpu = true;
    static const bool doAsync = true;
    runTestC(doCpu, doAsync);
}
#endif // DRISHTI_BUILD_C_INTERFACE

END_EMPTY_NAMESPACE
