/*

 MIT License

 Copyright (c) 2017 Leonid Keselman

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 https://github.com/leonidk/imshow/blob/master/LICENSE

 */

#pragma once

#if defined(IMSHOW_WITH_OPENCV)
#  include <opencv2/core.hpp>
#endif

namespace glfw
{
enum ImType
{
    IM_8U,
    IM_8I,
    IM_16U,
    IM_16I,
    IM_32U,
    IM_32I,
    IM_32F,
    IM_64F,
    IM_INVALID
};

struct Image
{
    void* data;
    int type, width, height, channels, stride;
};

void imshow(const char * name, const Image &image);
char getKey(bool wait = false);
void destroyWindow(const char *name);
void destroyAllWindows();
    
inline ImType getImType(const cv::Mat &image)
{
    switch (image.depth())
    {
        case CV_8U:
            return IM_8U;
        case CV_8S:
            return IM_8I;
        case CV_16U:
            return IM_16U;
        case CV_16S:
            return IM_16I;
        case CV_32S:
            return IM_32I;
        case CV_32F:
            return IM_32F;
        case CV_64F:
            return IM_64F;
        default:
            return IM_INVALID;
    }
}

#if defined(IMSHOW_WITH_OPENCV)
inline void imshow(const char * name, const cv::Mat &image)
{
    CV_Assert(image.isContinuous());
    imshow(name, {const_cast<void*>(image.ptr<void>()), getImType(image), image.cols, image.rows, image.channels(), static_cast<int>(image.step1())});
}

inline char waitKey(int value = 0)
{
    return getKey(value <= 0);
}
#endif 
    
}
