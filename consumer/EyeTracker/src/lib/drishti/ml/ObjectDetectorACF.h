/*! -*-c++-*-
  @file   ObjectDetectorACF.h
  @author David Hirvonen
  @brief  Internal ObjectDetectorACF abstract API declaration file.

  \copyright Copyright 2017 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __drishti_ml_ObjectDetectorACF_h__
#define __drishti_ml_ObjectDetectorACF_h__

#include "drishti/ml/ObjectDetector.h"

#include <acf/ACF.h>

#include <vector>

DRISHTI_ML_NAMESPACE_BEGIN

class ObjectDetectorACF : public ObjectDetector
{
public:

    ObjectDetectorACF();
    ObjectDetectorACF(const std::string& filename);
    ObjectDetectorACF(std::istream& is, const std::string& hint = {});
    ~ObjectDetectorACF() override;

    ObjectDetectorACF(const ObjectDetectorACF&) = delete;
    ObjectDetectorACF(ObjectDetectorACF&&) = delete;
    ObjectDetectorACF& operator=(const ObjectDetectorACF&) = delete;
    ObjectDetectorACF& operator=(ObjectDetectorACF&&) = delete;

    bool good() const;
    explicit operator bool() const;

    int operator()(const cv::Mat& image, std::vector<cv::Rect>& objects, std::vector<double>* scores = nullptr) override;
    int operator()(const MatP& image, std::vector<cv::Rect>& objects, std::vector<double>* scores = nullptr) override;
    cv::Size getWindowSize() const override;
    void setDoNonMaximaSuppression(bool flag) override;
    bool getDoNonMaximaSuppression() const override;

    acf::Detector* getDetector() const { return m_impl.get(); }

protected:

    std::unique_ptr<acf::Detector> m_impl;

};

DRISHTI_ML_NAMESPACE_END

#endif
