%{Cpp:LicenseTemplate}\
#include <jni.h>

#include <string>

extern "C" JNIEXPORT jstring JNICALL %{JniFunction}(JNIEnv *env, jclass)
{
    const std::string greeting = "Hello World from C++!";
    return env->NewStringUTF(greeting.c_str());
}
