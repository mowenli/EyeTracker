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

static ncnn::Mat squeezenet_param;
static ncnn::Mat squeezenet_bin;
static std::vector<std::string> squeezenet_words;
static ncnn::Net squeezenet;

extern "C" {
    // public native boolean Init(byte[] param, byte[] bin, byte[] words);
JNIEXPORT jboolean JNICALL Java_com_tencent_squeezencnn_SqueezeNcnn_Init(JNIEnv* env, jobject thiz, jbyteArray param, jbyteArray bin, jbyteArray words) {
    // init param
    {
        int len = env->GetArrayLength(param);
        squeezenet_param.create(len, (size_t)1u);
        env->GetByteArrayRegion(param, 0, len, (jbyte*)squeezenet_param);
        int ret = squeezenet.load_param((const unsigned char*)squeezenet_param);
        __android_log_print(ANDROID_LOG_DEBUG, "EyeTracker", "load_param %d %d", ret, len);
    }

    // init bin
    {
        int len = env->GetArrayLength(bin);
        squeezenet_bin.create(len, (size_t)1u);
        env->GetByteArrayRegion(bin, 0, len, (jbyte*)squeezenet_bin);
        int ret = squeezenet.load_model((const unsigned char*)squeezenet_bin);
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


}