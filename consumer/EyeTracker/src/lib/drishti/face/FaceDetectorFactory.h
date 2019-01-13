/*! -*-c++-*-
  @file   FaceDetectorFactory.h
  @author David Hirvonen
  @brief  Internal declaration of a factory for FaceDetector modules

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __drishti_face_FaceDetectorFactory_h__
#define __drishti_face_FaceDetectorFactory_h__

#include "drishti/face/drishti_face.h"
#include "drishti/face/Face.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <map>

// clang-format off
namespace drishti { namespace ml { class ObjectDetector; } };
namespace drishti { namespace ml { class ShapeEstimator; } };
namespace drishti { namespace eye { class EyeModelEstimator; } };
// clang-format on

DRISHTI_FACE_NAMESPACE_BEGIN

class FaceDetectorFactory
{
public:
    FaceDetectorFactory(bool inner=false) : inner(inner) {}

    FaceDetectorFactory(
        std::string  sFaceDetector,
        std::string  sFaceRegressor,
        std::string  sEyeRegressor,
        std::string  sFaceDetectorMean, bool inner=false)
        : sFaceDetector(std::move(sFaceDetector))
        , sFaceRegressor(std::move(sFaceRegressor))
        , sEyeRegressor(std::move(sEyeRegressor))
        , sFaceDetectorMean(std::move(sFaceDetectorMean))
        , inner(inner)
    {
    }
    
    virtual std::unique_ptr<drishti::ml::ObjectDetector> getFaceDetector();
    virtual std::unique_ptr<drishti::ml::ShapeEstimator> getFaceEstimator();
    virtual std::unique_ptr<drishti::eye::EyeModelEstimator> getEyeEstimator();
    virtual drishti::face::FaceModel getMeanFace();

    virtual bool isInner(drishti::ml::ShapeEstimator &estimator);
    virtual bool isInner();

    std::string sFaceDetector;
    std::string sFaceRegressor;
    std::string sEyeRegressor;
    std::string sFaceDetectorMean;
    
    bool inner = false;

    std::map<std::string, std::string> sModelBindings;
};

class FaceDetectorFactoryStream : public FaceDetectorFactory
{
public:
    FaceDetectorFactoryStream() = default;

    FaceDetectorFactoryStream(
        std::istream* iFaceDetector,
        std::istream* iFaceRegressor,
        std::istream* iEyeRegressor,
        std::istream* iFaceDetectorMean, bool inner = false)
        : FaceDetectorFactory(inner)
        , iFaceDetector(iFaceDetector)
        , iFaceRegressor(iFaceRegressor)
        , iEyeRegressor(iEyeRegressor)
        , iFaceDetectorMean(iFaceDetectorMean)
    {
    }

    std::unique_ptr<drishti::ml::ObjectDetector> getFaceDetector() override;
    std::unique_ptr<drishti::ml::ShapeEstimator> getFaceEstimator() override;
    std::unique_ptr<drishti::eye::EyeModelEstimator> getEyeEstimator() override;
    drishti::face::FaceModel getMeanFace() override;

    std::istream* iFaceDetector = nullptr;
    std::istream* iFaceRegressor = nullptr;
    std::istream* iEyeRegressor = nullptr;
    std::istream* iFaceDetectorMean = nullptr;
};

std::ostream& operator<<(std::ostream& os, const FaceDetectorFactory& factory);

DRISHTI_FACE_NAMESPACE_END

#endif // __drishti_face_FaceDetectorFactory_h__
