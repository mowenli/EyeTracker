/**
  @file   drishti/Context.hpp
  @author David Hirvonen
  @brief  Public API for continuous face filter.

  \copyright Copyright 2014-2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <drishti/Context.hpp>
#include <drishti/ContextImpl.h>
#include <drishti/SensorImpl.h>

#define DRISHTI_LOGGER_NAME "drishti"

_DRISHTI_SDK_BEGIN

Context::Impl::Impl(drishti::sdk::SensorModel& sensor)
    : sensor(sensor.getImpl()->sensor)
    , logger(drishti::core::Logger::create(DRISHTI_LOGGER_NAME))
    , threads(std::make_shared<tp::ThreadPool<>>()) // thread-pool
{
}

Context::Context(drishti::sdk::SensorModel& sensor)
{
    impl = drishti::core::make_unique<Impl>(sensor);
    updateGL();
}

Context::~Context() = default;

void Context::updateGL()
{
#if defined(_WIN32) || defined(_WIN64)
    // A windows DLL must call glewInit() from within the library (not the application layer)
    CV_Assert(wglGetCurrentContext() != NULL && GLEW_OK == glewInit());
#endif  
}

void Context::setDoSingleFace(bool flag)
{
    impl->doSingleFace = flag;
}

bool Context::getDoSingleFace() const
{
    return impl->doSingleFace;
}

void Context::setMinDetectionDistance(float value)
{
    impl->minDetectionDistance = value;
}

float Context::getMinDetectionDistance() const
{
    return impl->minDetectionDistance;
}

void Context::setMaxDetectionDistance(float value)
{
    impl->maxDetectionDistance = value;
}

float Context::getMaxDetectionDistance() const
{
    return impl->maxDetectionDistance;
}

void Context::setFaceFinderInterval(float value)
{
    impl->faceFinderInterval = value;
}

float Context::getFaceFinderInterval() const
{
    return impl->faceFinderInterval;
}

void Context::setAcfCalibration(float value)
{
    impl->acfCalibration = value;
}

float Context::getAcfCalibration() const
{
    return impl->acfCalibration;
}

void Context::setRegressorCropScale(float value)
{
    impl->regressorCropScale = value;
}

float Context::getRegressorCropScale() const
{
    return impl->regressorCropScale;
}

void Context::setMinTrackHits(int hits)
{
    impl->minTrackHits = hits;
}

int Context::getMinTrackHits() const
{
    return impl->minTrackHits;
}

void Context::setMaxTrackMisses(int hits)
{
    impl->maxTrackMisses = hits;
}

int Context::getMaxTrackMisses() const
{
    return impl->maxTrackMisses;
}

void Context::setMinFaceSeparation(float value)
{
    impl->minFaceSeparation = value;
}

float Context::getMinFaceSeparation() const
{
    return impl->minFaceSeparation;
}

void Context::setDoOptimizedPipeline(bool flag)
{
    impl->doOptimizedPipeline = flag;
}

bool Context::getDoOptimizedPipeline() const
{
    return impl->doOptimizedPipeline;
}

void Context::setDoCpuACF(bool flag)
{
    impl->doCpuAcf = flag;
}

bool Context::getDoCpuACF() const
{
    return impl->doCpuAcf;
}

void Context::setDoAnnotation(bool flag)
{
    impl->doAnnotation = flag;
}

bool Context::getDoAnnotation() const
{
    return impl->doAnnotation;
}

void Context::setGLContext(void *context)
{
    impl->glContext = context;
}

void* Context::getGLContext() const
{
    return impl->glContext;
}

_DRISHTI_SDK_END
