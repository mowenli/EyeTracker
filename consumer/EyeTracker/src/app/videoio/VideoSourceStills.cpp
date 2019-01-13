/*! -*-c++-*-
 @file   videoio/VideoSourceStills.cpp
 @author David Hirvonen
 @brief  Simple implementation of a list-of-files VideoSource.

 \copyright Copyright 2017 Elucideye, Inc. All rights reserved.
 \license{This project is released under the 3 Clause BSD License.}

 */

#include "videoio/VideoSourceStills.h"

#include "drishti/core/make_unique.h"
#include "drishti/testlib/drishti_cli.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <utility>

DRISHTI_VIDEOIO_NAMESPACE_BEGIN

class VideoSourceStills::Impl
{
public:
    explicit Impl(std::string  filename)
        : m_filename(std::move(filename))
    {
        m_filenames = drishti::cli::expand(m_filename);
    }

    explicit Impl(std::vector<std::string>  filenames)
        : m_filenames(std::move(filenames))
    {
    }

    ~Impl() = default;

    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) = delete;

    std::size_t count() const
    {
        return m_filenames.size();
    }

    VideoSourceCV::Frame operator()(int i = -1)
    {
    if (!((0 <= i) && (i < m_filenames.size())))
    {
      return {};
    }
        return VideoSourceCV::Frame(cv::imread(m_filenames[i]), i, m_filenames[i]);
    }

    std::string m_filename;
    std::vector<std::string> m_filenames;
};

VideoSourceStills::VideoSourceStills(const std::string& filename)
{
    m_impl = drishti::core::make_unique<Impl>(filename);
}

VideoSourceStills::VideoSourceStills(const std::vector<std::string>& filenames)
{
    m_impl = drishti::core::make_unique<Impl>(filenames);
}

VideoSourceStills::~VideoSourceStills()
= default;

std::size_t VideoSourceStills::count() const
{
    return m_impl->count();
}

VideoSourceCV::Frame VideoSourceStills::operator()(int i)
{
    return (*m_impl)(i);
}

DRISHTI_VIDEOIO_NAMESPACE_END
