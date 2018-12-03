#include <android/bitmap.h>
#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

// ncnn
#include "net.h"

#include <sys/time.h>
#include <unistd.h>

static struct timeval tv_begin;
static struct timeval tv_end;
static double elasped;

static void bench_start() {
    gettimeofday(&tv_begin, NULL);
}

static void bench_end(const char* comment) {
    gettimeofday(&tv_end, NULL);
    elasped = ((tv_end.tv_sec - tv_begin.tv_sec) * 1000000.0f + tv_end.tv_usec - tv_begin.tv_usec) / 1000.0f;
//     fprintf(stderr, "%.2fms   %s\n", elasped, comment);
    __android_log_print(ANDROID_LOG_DEBUG, "EyeTracker", "%.2fms   %s", elasped, comment);
}

static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
static ncnn::PoolAllocator g_workspace_pool_allocator;

static ncnn::Mat eyetracker_param;
static ncnn::Mat eyetracker_bin;
static ncnn::Net eyetracker;

static std::vector<std::string> split_string(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}

extern "C" {
    // public native boolean Init(byte[] param, byte[] bin);
JNIEXPORT jboolean JNICALL Java_com_example_stellasong_eyetracker_EyetrackerNcnn_Init(JNIEnv* env, jobject thiz, jbyteArray param, jbyteArray bin) {
    // init param
    {
        int len = env->GetArrayLength(param);
        eyetracker_param.create(len, (size_t)1u);
        env->GetByteArrayRegion(param, 0, len, (jbyte*)eyetracker_param);
        int ret = eyetracker.load_param((const unsigned char*)eyetracker_param);
        __android_log_print(ANDROID_LOG_DEBUG, "EyeTracker", "load_param %d %d", ret, len);
    }

    // init bin
    {
        int len = env->GetArrayLength(bin);
        eyetracker_bin.create(len, (size_t)1u);
        env->GetByteArrayRegion(bin, 0, len, (jbyte*)eyetracker_bin);
        int ret = eyetracker.load_model((const unsigned char*)eyetracker_bin);
        __android_log_print(ANDROID_LOG_DEBUG, "EyeTracker", "load_model %d %d", ret, len);
    }

    ncnn::Option opt;
    opt.lightmode = true;
    opt.num_threads = 4;
    opt.blob_allocator = &g_blob_pool_allocator;
    opt.workspace_allocator = &g_workspace_pool_allocator;

    ncnn::set_default_option(opt);

    return JNI_TRUE;
}


// public native String Detect(Bitmap image_left, Bitmap image_right, Bitmap image_face, byte[] face_grid);
JNIEXPORT jstring JNICALL Java_com_example_stellasong_eyetracker_EyetrackerNcnn_Detect(JNIEnv* env, jobject thiz, jobject image_left, jobject image_right, jobject image_face, jobject face_grid) {
    bench_start();

    // ncnn from bitmap
    ncnn::Mat in_left, in_right, in_face, in_grad;
    {
        AndroidBitmapInfo info;
        AndroidBitmap_getInfo(env, image_left, &info);
        int width = info.width;
        int height = info.height;
        if (width != 224 || height != 224)
            return NULL;
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
            return NULL;

        void* indata;
        AndroidBitmap_lockPixels(env, image_left, &indata);

        in_left = ncnn::Mat::from_pixels((const unsigned char*)indata, ncnn::Mat::PIXEL_RGBA2BGR, width, height);

        AndroidBitmap_unlockPixels(env, image_left);


        AndroidBitmap_getInfo(env, image_right, &info);
        width = info.width;
        height = info.height;
        if (width != 224 || height != 224)
            return NULL;
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
            return NULL;

        AndroidBitmap_lockPixels(env, image_right, &indata);

        in_right = ncnn::Mat::from_pixels((const unsigned char*)indata, ncnn::Mat::PIXEL_RGBA2BGR, width, height);

        AndroidBitmap_unlockPixels(env, image_right);


        AndroidBitmap_getInfo(env, image_face, &info);
        width = info.width;
        height = info.height;
        if (width != 224 || height != 224)
            return NULL;
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
            return NULL;

        AndroidBitmap_lockPixels(env, image_face, &indata);

        in_face = ncnn::Mat::from_pixels((const unsigned char*)indata, ncnn::Mat::PIXEL_RGBA2BGR, width, height);

        AndroidBitmap_unlockPixels(env, image_face);


        bool tmp[25][25];
        for (int i = 0; i < 625; i++) {
            if (i / 25 > 5 && i / 25 < 15 && i % 25 > 10 && i % 25 < 24) {
                tmp[i / 25][i % 25] = 1;
            } else {
                tmp[i / 25][i % 25] = 0;
            }
        }
        in_grad = ncnn::Mat(625, 1, 1, tmp);
    }

    // eyetracker
    std::vector<float> cls_scores;
    {
        ncnn::Extractor ex = eyetracker.create_extractor();

        ex.input("image_left", in_left);
        ex.input("image_right", in_right);
        ex.input("image_face", in_face);
        ex.input("facegrid", in_grad);

        ncnn::Mat out;
        ex.extract("fc3", out);

        cls_scores.resize(out.w);
        for (int j=0; j<out.w; j++) {
            cls_scores[j] = out[j];
        }
    }
    char tmp1[32], tmp2[32];
    sprintf(tmp1, "%.3f", cls_scores[0]);
    sprintf(tmp2, "%.3f", cls_scores[1]);
    std::string result_str = std::string("x = ") + tmp1 + ", y = " + tmp2;
    jstring result = env->NewStringUTF(result_str.c_str());
    return result;
}

}