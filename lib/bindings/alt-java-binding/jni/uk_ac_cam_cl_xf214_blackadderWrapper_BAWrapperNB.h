/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB */

#ifndef _Included_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
#define _Included_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
#ifdef __cplusplus
extern "C" {
#endif
#undef uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_DEFAULT_SCOPE_ID_LENGTH
#define uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_DEFAULT_SCOPE_ID_LENGTH 8L
/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_create_new_ba
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1create_1new_1ba
  (JNIEnv *, jobject, jint);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_delete_ba
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1delete_1ba
  (JNIEnv *, jobject, jlong);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_publish_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1publish_1scope
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_publish_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1publish_1item
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_unpublish_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1unpublish_1scope
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_unpublish_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1unpublish_1item
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_subscribe_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1subscribe_1scope
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_subscribe_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1subscribe_1item
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_unsubscribe_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1unsubscribe_1scope
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_unsubscribe_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1unsubscribe_1item
  (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray, jbyte, jbyteArray);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_publish_data
 * Signature: (J[BB[B[BI)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1publish_1data
  (JNIEnv *, jobject, jlong, jbyteArray, jbyte, jbyteArray, jbyteArray, jint);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_publish_data_direct
 * Signature: (J[BB[BLjava/nio/ByteBuffer;I)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1publish_1data_1direct
  (JNIEnv *, jobject, jlong, jbyteArray, jbyte, jbyteArray, jobject, jint);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_register_event_receiver
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1register_1event_1receiver
  (JNIEnv *, jobject, jlong);

/*
 * Class:     uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB
 * Method:    c_delete_event
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_uk_ac_cam_cl_xf214_blackadderWrapper_BAWrapperNB_c_1delete_1event
  (JNIEnv *, jobject, jlong, jlong);
#ifdef __cplusplus
}
#endif
#endif