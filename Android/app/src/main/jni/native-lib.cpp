#include <jni.h>
#include <string>
#include "zinnia.h"
#include "caffe_classifier.h"

#include <android/log.h>

#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO, "droidpad", __VA_ARGS__)

#define _DEBUG

using namespace zinnia;

struct CaffeCVMatState {
    CaffeCVMatState(int width, int height) : prePoint(0, 0)
#ifdef _DEBUG
            , _index(0)
#endif
    {
        mat = new cv::Mat(width, height, CV_32FC1, cv::Scalar(0));
    }

    ~CaffeCVMatState() { delete mat; }

    cv::Mat *mat;
    cv::Point prePoint;
#ifdef _DEBUG
    unsigned int _index;
#endif
};

static Recognizer *gRecognizer = NULL;
static Character *gCharacter = NULL;
static bool zinniaInited = false;

static Classifier *gClassifer = NULL;
static bool caffeInited = false;

static std::map<jobject, CaffeCVMatState *> gMatCache;
//typedef std::map<jobject, CaffeCVMatState *>::iterator MatCacheIterator;


/////////////////////////////////////////////////////////////////////////
static void throwException(JNIEnv *env, const char *clz, const char *msg);
/////////////////////////////////////////////////////////////////////////

extern "C"
{

JNIEXPORT jboolean JNICALL
Java_com_droidpad_ZinniaRecognizer_zinniaSetup(JNIEnv *env, jobject instance, jstring model_file_,
                                               jint width, jint height) {
    const char *model_file = env->GetStringUTFChars(model_file_, 0);

    if (zinniaInited) {
        throwException(env, "java/lang/Exception", "Duplicate init of zinnia.");
        return JNI_FALSE;
    }

    if (!gRecognizer) {
        gRecognizer = Recognizer::create();
    }

    if (!gCharacter) {
        gCharacter = Character::create();
        gCharacter->set_width(width);
        gCharacter->set_height(height);
        gCharacter->clear();
    }

    zinniaInited = gRecognizer->open(model_file);

    env->ReleaseStringUTFChars(model_file_, model_file);
    return zinniaInited ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT void JNICALL
Java_com_droidpad_ZinniaRecognizer_znAddCoordinate(JNIEnv *env, jobject instance, jlong index,
                                                   jint x,
                                                   jint y) {
    if (gCharacter)
        gCharacter->add(index, x, y);
}


JNIEXPORT jstring JNICALL
Java_com_droidpad_ZinniaRecognizer_znGetResult(JNIEnv *env, jobject instance, jint max_num) {

    Result *r = gRecognizer->classify(*gCharacter, max_num);
    if (!r) {
        //throwException(env, "java/lang/Exception", gRecognizer->what());
        //return NULL;
        return env->NewStringUTF("");
    }
    size_t N = r->size();
    char *utf8_buff = new char[N * 6];
    memset(utf8_buff, 0, N * 6);
    size_t offset = 0;
    for (int i = 0; i < N; i++) {
        size_t len = strlen(r->value(i)) * sizeof(char);
        strncpy(utf8_buff + offset, r->value(i), len);
        offset += len;
    }
    jstring str = env->NewStringUTF(utf8_buff);
    delete[] utf8_buff;
    return str;
}

JNIEXPORT void JNICALL
Java_com_droidpad_ZinniaRecognizer_zinniaTearDown(JNIEnv *env, jobject instance) {

    if (gRecognizer) {
        gRecognizer->close();
        delete gRecognizer;
    }
    if (gCharacter)
        delete gCharacter;
    gRecognizer = NULL;
    gCharacter = NULL;
    zinniaInited = false;
}

JNIEXPORT void JNICALL
Java_com_droidpad_ZinniaRecognizer_znReset(JNIEnv *env, jobject instance) {

    if (gCharacter)
        gCharacter->clear();
}

//////////////////////////////////////////////////////////////////////////

}

static void throwException(JNIEnv *env, const char *clz, const char *msg) {
    jclass _cls = env->FindClass(clz);
    if (!_cls)
        return;
    env->ThrowNew(_cls, msg);
    env->DeleteLocalRef(_cls);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CaffeCVMatState *_GetCVMatFromCache(JNIEnv *env, jobject obj) {
    CaffeCVMatState *state = NULL;
    auto it = gMatCache.begin();
    for (; it != gMatCache.end(); it++) {
        if (env->IsSameObject(it->first, obj))
            break;
    }

    if (it != gMatCache.end())
        state = it->second;
    return state;
}

CaffeCVMatState *_GetCVMatFromObject(JNIEnv *env, jobject obj) {
    CaffeCVMatState *state = NULL;
    jclass clazz = env->GetObjectClass(obj);
    jfieldID jfield = env->GetFieldID(clazz, "mMat", "J");
    if (jfield == NULL) {
        throwException(env, "java/lang/Exception",
                       "Cannot find mMat field in jobject instance");
    }
    state = reinterpret_cast<CaffeCVMatState *>(env->GetLongField(obj, jfield));
    if (state == NULL) {
        throwException(env, "java/lang/Exception",
                       "The values of mMat field in jobject instance is illegal");
    }
    return state;
}

CaffeCVMatState *_RetrieveCVMat(JNIEnv *env, jobject obj) {
    CaffeCVMatState *state = _GetCVMatFromCache(env, obj);
    if (!state)
        state = _GetCVMatFromObject(env, obj);
    return state;
}

CaffeCVMatState *_RetrieveCVMatAndCache(JNIEnv *env, jobject obj) {
    CaffeCVMatState *state = _GetCVMatFromCache(env, obj);
    if (!state) {
        state = _GetCVMatFromObject(env, obj);
#ifdef _DEBUG
        state->_index = gMatCache.size();
#endif
        jobject ref = env->NewGlobalRef(obj);
        gMatCache[ref] = state;
    }
    return state;
}

void _CVMatDestroyCachedItem(JNIEnv *env, jobject obj) {
    auto it = gMatCache.begin();
    for (; it != gMatCache.end();) {
        if (env->IsSameObject(it->first, obj)) {
            env->DeleteGlobalRef(it->first);
            delete it->second;
            gMatCache.erase(it);
            it = gMatCache.begin();
            continue;
        }
        it++;
    }
}

//---------------------------------------------------------------------------//

extern "C"
{

JNIEXPORT jboolean JNICALL
Java_com_droidpad_CaffeEngine_caffeSetup(JNIEnv *env, jobject instance, jstring deploy_file_,
                                         jstring model_file_, jstring mean_file_,
                                         jstring lable_file_) {
    const char *deploy_file = env->GetStringUTFChars(deploy_file_, 0);
    const char *model_file = env->GetStringUTFChars(model_file_, 0);
    const char *mean_file = env->GetStringUTFChars(mean_file_, 0);
    const char *lable_file = env->GetStringUTFChars(lable_file_, 0);

    if (caffeInited) {
        throwException(env, "java/lang/Exception", "Duplicate init of caffe.");
        return JNI_FALSE;
    }

    if (!gClassifer)
        gClassifer = new Classifier();
    caffeInited = gClassifer->init(deploy_file, model_file, mean_file, lable_file);

    env->ReleaseStringUTFChars(deploy_file_, deploy_file);
    env->ReleaseStringUTFChars(model_file_, model_file);
    env->ReleaseStringUTFChars(mean_file_, mean_file);
    env->ReleaseStringUTFChars(lable_file_, lable_file);

    return caffeInited ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_droidpad_CaffeEngine_cfPredic(JNIEnv *env, jobject instance, jobject mat, jint max_num) {

    CaffeCVMatState *state = _RetrieveCVMat(env, mat);
#ifdef _DEBUG
    LOG_I("cfPredic CVMatState (index %d)", state->_index);
#endif
    std::vector<Prediction> predictions = gClassifer->Classify(*state->mat, max_num);
    size_t N = predictions.size();
    auto *array = new jchar[N];
    for (size_t i = 0; i < N; ++i) {
        array[i] = (jchar) strtol(predictions[i].first.c_str(), NULL, 16);
    }
    jstring ret = env->NewString(array, N);
    delete[] array;
    return ret;
}

JNIEXPORT void JNICALL
Java_com_droidpad_CaffeEngine_caffeTearDown(JNIEnv *env, jobject instance) {

    if (gClassifer)
        delete gClassifer;
    gClassifer = NULL;
    caffeInited = false;
}

/////////////////////////////////////////////////////////////////////////



JNIEXPORT void JNICALL
Java_com_droidpad_CaffeCVMat_cvMatCreate(JNIEnv *env, jobject instance, jint width, jint height) {

    jclass clazz = env->GetObjectClass(instance);
    jfieldID jfield = env->GetFieldID(clazz, "mMat", "J");
    if (jfield == NULL) {
        throwException(env, "java/lang/Exception", "Cannot find mMat field in jobject instance");
        return;
    }
    auto *state = new CaffeCVMatState(width, height);
    env->SetLongField(instance, jfield, reinterpret_cast<jlong>(state));
}

JNIEXPORT void JNICALL
Java_com_droidpad_CaffeCVMat_cvMatAddPathPoint(JNIEnv *env, jobject instance, jint x, jint y,
                                               jboolean newPath) {

    CaffeCVMatState *state = _RetrieveCVMatAndCache(env, instance);
#ifdef _DEBUG
    LOG_I("cvMatAddPathPoint CVMatState (index %d)", state->_index);
#endif

    if (newPath)
        state->prePoint = cv::Point(x, y);
    else {
        cv::Point _cur(x, y);
        cv::line(*state->mat, state->prePoint, _cur, cv::Scalar(255), 4);
        state->prePoint = _cur;
    }
}

JNIEXPORT void JNICALL
Java_com_droidpad_CaffeCVMat_cvMatReset(JNIEnv *env, jobject instance) {

    CaffeCVMatState *state = _RetrieveCVMatAndCache(env, instance);
#ifdef _DEBUG
    LOG_I("cvMatReset CVMatState (index %d)", state->_index);
#endif
    IplImage tmp(*state->mat);
    cvSet(&tmp, cvScalar(0));
}

JNIEXPORT void JNICALL
Java_com_droidpad_CaffeCVMat_cvMatDestroy(JNIEnv *env, jobject instance) {
#ifdef _DEBUG
    CaffeCVMatState *state = _RetrieveCVMat(env, instance);
    LOG_I("cvMatDestroy CVMatState (index %d)", state->_index);
#endif
    _CVMatDestroyCachedItem(env, instance);
}

}// extern "C"