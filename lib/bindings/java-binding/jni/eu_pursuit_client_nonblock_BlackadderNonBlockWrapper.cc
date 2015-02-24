/*
 * Copyright (C) 2013  Christos Tsilopoulos, Mobile Multimedia Laboratory, 
 * Athens University of Economics and Business 
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#include "eu_pursuit_client_nonblock_BlackadderNonBlockWrapper.h"
#include <nb_blackadder.hpp>
#include <string>
#include <stdio.h>

JavaVM *baJVM;
jobject baWrapper;
long events_arrived = 0;

void print_array_contents(char *data_ptr, int length){
	for(int i=0; i<length; i++){
		printf("%d ", data_ptr[i]);	
	}
	printf("\n");
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved){
	//printf("on load\n");
	baJVM = jvm;
	
	JNIEnv *env;
	baJVM->GetEnv((void **)&env, JNI_VERSION_1_6);
	return JNI_VERSION_1_6;
}

void onEventReceived(Event *ev) {
	//printf("onEventReceived type %d events %u event_pointer %u\n", ev->type, ++events_arrived, (long)ev);	
	JNIEnv *jnienv;
	int status = baJVM->GetEnv((void **)&jnienv, JNI_VERSION_1_6);	
	int must_detach = ev->type == END_EVENT;
	if (status < 0) {
		//printf("Failed to get JNI environment, assuming native thread %d\n", status);
		//printf("try to get thread\n");
		status = baJVM->AttachCurrentThread(&jnienv, NULL);
		if (status < 0) {
			printf("Failed to attach current thread.\n");
			printf("Cannot call Java method (JNIEnv failed).\n");
			return;
		}
	}	
	
	const char *id_ptr = ev->id.c_str();
	//printf("got message with type %d data_len: %d id is:\n", ev->type, ev->data_len);
	//print_array_contents(id_ptr, ev->id.length());
	
	//printf("data is:\n");
	//print_array_contents(ev->data, ev->data_len);
		
	jbyteArray idArray = jnienv->NewByteArray(ev->id.length());
	jnienv->SetByteArrayRegion(idArray, 0, ev->id.length(), (jbyte *) id_ptr);
	
	// create ByteBuffer and let Java read c++ memory about the data directly
	jobject jbuff = jnienv->NewDirectByteBuffer((void*)ev->data, (jlong) ev->data_len);
	
	static jclass cls = jnienv->GetObjectClass(baWrapper);
	static jmethodID mid = jnienv->GetMethodID(cls, "onEventArrived", "(J[BBLjava/nio/ByteBuffer;)V");
	if (mid == NULL) {
		printf("Cannot call method onEventArrived: not found.\n");
		
		if(must_detach){
			printf("detaching native (worker) thread from JVM\n");
			static jmethodID detachMethod = jnienv->GetMethodID(cls, "detachNativeThead", "()V");
			if(detachMethod == NULL){
				printf("Cannot call method detachNativeThead: not found.\n");
			}else{
				jnienv->CallVoidMethod(baWrapper, detachMethod);
			}			
			status = baJVM->DetachCurrentThread();
			if(status < 0){
				printf("could not detach thread from jvm: %d\n", status);
			}
		}		
		return; /* method not found */
	}	
		
	jnienv->CallVoidMethod(baWrapper, mid, (jlong)ev, idArray, (jbyte) ev->type, jbuff);
	
	jnienv->DeleteLocalRef(idArray);
	jnienv->DeleteLocalRef(jbuff);	
	//printf("onEventReceived exit\n");
	
	if(must_detach){
		printf("detaching native (worker) thread from JVM\n");
		static jmethodID detachMethod = jnienv->GetMethodID(cls, "detachNativeThead", "()V");
		if(detachMethod == NULL){
			printf("Cannot call method detachNativeThead: not found.\n");
		}else{
			jnienv->CallVoidMethod(baWrapper, detachMethod);
		}			
		status = baJVM->DetachCurrentThread();
		if(status < 0){
			printf("could not detach thread from jvm: %d\n", status);
		}
	}		
}

void deleteAllEvents(Event *ev) {
	printf("delete all event %lu\n", (long)ev);
	delete ev;
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    create_new_ba
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_create_1new_1ba
  (JNIEnv *env, jobject wrapper, jint userspace){
	bool user = userspace? true : false;	
	NB_Blackadder *ba_ptr = NB_Blackadder::Instance(user);
	//printf("ba pointer %ul\n", (long)ba_ptr);
	fflush(stdout);
	
	ba_ptr->setCallback(*onEventReceived);
	baWrapper = (env)->NewGlobalRef(wrapper);
	return (jlong) ba_ptr;
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    close_and_delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_close_1and_1delete
  (JNIEnv *env, jobject, jlong ba_ptr){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;	
	//printf("BA call to join\n");
	//ba->join();
	//printf("BA joined\n");
	
	printf("BA call to disconnect\n");
	ba->disconnect();
	printf("BA disconnected\n");
	env->DeleteGlobalRef(baWrapper);
	delete ba;
	printf("ba pointer deleted\n");	
  }

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_publish_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1publish_1scope
  (JNIEnv *env, jobject obj, jlong ba_ptr, jbyteArray scope, jbyteArray scope_prefix, jbyte strategy, jbyteArray jstr_opt){

	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * scope_ptr = (*env).GetByteArrayElements(scope, &copy);
	unsigned int scope_length = (*env).GetArrayLength(scope);
        const string scope_str ((char *)scope_ptr, scope_length);

	jbyte *scope_prefix_ptr = (*env).GetByteArrayElements(scope_prefix, &copy);
	unsigned int scope_prefix_length = (*env).GetArrayLength(scope_prefix);
        const string scope_prefix_str ((char *)scope_prefix_ptr, scope_prefix_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}

	/*
	 * the call to blackadder 
	 */
	ba->publish_scope(scope_str, scope_prefix_str, (char)strategy, str_opt, str_opt_len);

        /*
         * XXX: Because the ID strings are expected to be constant,
         *      ReleaseByteArrayElements() could be called with
         *      JNI_ABORT instead of 0.
         */
	(*env).ReleaseByteArrayElements(scope, scope_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(scope_prefix, scope_prefix_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_publish_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1publish_1item
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray rid, jbyteArray sid, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * rid_ptr = (*env).GetByteArrayElements(rid, &copy);
	int rid_length = (*env).GetArrayLength(rid);	
        const string rid_str ((char *)rid_ptr, rid_length);

	jbyte *sid_ptr = (*env).GetByteArrayElements(sid, &copy);
	int sid_length = (*env).GetArrayLength(sid);
        const string sid_str ((char *)sid_ptr, sid_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}

	/*
	 * the call to blackadder 
	 */
	ba->publish_info(rid_str, sid_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(rid, rid_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(sid, sid_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_unpublish_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1unpublish_1scope
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray scope, jbyteArray scope_prefix, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * scope_ptr = (*env).GetByteArrayElements(scope, &copy);
	int scope_length = (*env).GetArrayLength(scope);
        const string scope_str ((char *)scope_ptr, scope_length);

	jbyte *scope_prefix_ptr = (*env).GetByteArrayElements(scope_prefix, &copy);
	int scope_prefix_length = (*env).GetArrayLength(scope_prefix);
        const string scope_prefix_str ((char *)scope_prefix_ptr, scope_prefix_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}
	
	/*
	 * the call to blackadder 
	 */
	ba->unpublish_scope(scope_str, scope_prefix_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(scope, scope_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(scope_prefix, scope_prefix_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_unpublish_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1unpublish_1item
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray rid, jbyteArray sid, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * rid_ptr = (*env).GetByteArrayElements(rid, &copy);
	int rid_length = (*env).GetArrayLength(rid);
	//string id = rid_length == 0? "" : std::string((char *)rid_ptr, rid_length);
        const string rid_str ((char *)rid_ptr, rid_length);

	jbyte *sid_ptr = (*env).GetByteArrayElements(sid, &copy);
	int sid_length = (*env).GetArrayLength(sid);
	//string prefix = sid_length == 0? "" : std::string((char *)sid_ptr, sid_length);
        const string sid_str ((char *)sid_ptr, sid_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}

	/*
	 * the call to blackadder 
	 */
	ba->unpublish_info(rid_str, sid_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(rid, rid_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(sid, sid_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_subscribe_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1subscribe_1scope
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray scope, jbyteArray scope_prefix, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * scope_ptr = (*env).GetByteArrayElements(scope, &copy);
	int scope_length = (*env).GetArrayLength(scope);
        const string scope_str ((char *)scope_ptr, scope_length);

	jbyte *scope_prefix_ptr = (*env).GetByteArrayElements(scope_prefix, &copy);
	int scope_prefix_length = (*env).GetArrayLength(scope_prefix);
        const string scope_prefix_str ((char *)scope_prefix_ptr, scope_prefix_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}
	
	/*
	 * the call to blackadder 
	 */
	ba->subscribe_scope(scope_str, scope_prefix_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(scope, scope_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(scope_prefix, scope_prefix_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_subscribe_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1subscribe_1item
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray rid, jbyteArray sid, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * rid_ptr = (*env).GetByteArrayElements(rid, &copy);
	int rid_length = (*env).GetArrayLength(rid);
        const string rid_str ((char *)rid_ptr, rid_length);

	jbyte *sid_ptr = (*env).GetByteArrayElements(sid, &copy);
	int sid_length = (*env).GetArrayLength(sid);
        const string sid_str ((char *)sid_ptr, sid_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}

	/*
	 * the call to blackadder 
	 */
	ba->subscribe_info(rid_str, sid_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(rid, rid_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(sid, sid_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_unsubscribe_scope
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1unsubscribe_1scope
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray scope, jbyteArray scope_prefix, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * scope_ptr = (*env).GetByteArrayElements(scope, &copy);
	int scope_length = (*env).GetArrayLength(scope);
        const string scope_str ((char *)scope_ptr, scope_length);

	jbyte *scope_prefix_ptr = (*env).GetByteArrayElements(scope_prefix, &copy);
	int scope_prefix_length = (*env).GetArrayLength(scope_prefix);
        const string scope_prefix_str ((char *)scope_prefix_ptr, scope_prefix_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}
	
	/*
	 * the call to blackadder 
	 */
	ba->unsubscribe_scope(scope_str, scope_prefix_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(scope, scope_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(scope_prefix, scope_prefix_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_unsubscribe_item
 * Signature: (J[B[BB[B)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1unsubscribe_1item
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray rid, jbyteArray sid, jbyte strategy, jbyteArray jstr_opt){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

	jbyte * rid_ptr = (*env).GetByteArrayElements(rid, &copy);
	int rid_length = (*env).GetArrayLength(rid);
	//string id = rid_length == 0? "" : std::string((char *)rid_ptr, rid_length);
        const string rid_str ((char *)rid_ptr, rid_length);

	jbyte *sid_ptr = (*env).GetByteArrayElements(sid, &copy);
	int sid_length = (*env).GetArrayLength(sid);
	//string prefix = sid_length == 0? "" : std::string((char *)sid_ptr, sid_length);
        const string sid_str ((char *)sid_ptr, sid_length);

	void *str_opt;
	str_opt = 0;
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}

	/*
	 * the call to blackadder 
	 */
	ba->unsubscribe_info(rid_str, sid_str, (char)strategy, str_opt, str_opt_len);

	(*env).ReleaseByteArrayElements(rid, rid_ptr, (jint)0);
	(*env).ReleaseByteArrayElements(sid, sid_ptr, (jint)0);	
	
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_publish_data
 * Signature: (J[BB[B[BI)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1publish_1data
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray name, jbyte strategy, jbyteArray jstr_opt, jbyteArray data, jint datalen){
    //printf("publish data []\n");
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;

	jboolean copy = (jboolean)false;

    //printf("publish data: get id\n");
	jbyte *name_ptr = (*env).GetByteArrayElements(name, &copy);
	int name_length = (*env).GetArrayLength(name);
        const string name_str ((char *)name_ptr, name_length);
	
	void *str_opt;
	void *str_opt_native = 0;
	str_opt = 0;
	unsigned int str_opt_len = 0;
	

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);				
	}

    //printf("publish data: get data:\n");
	jbyte *data_ptr = (*env).GetByteArrayElements(data, &copy);
	//printf("publish data: data ptr %u\n", (long)data_ptr);
	//print_array_contents((char *)data_ptr, (int)datalen);	
	
	char *data_native_ptr = (char *)calloc((int)datalen, sizeof(char));
	if(data_native_ptr != 0){
		memcpy( (void *)data_native_ptr, (void *)data_ptr, (int)datalen);
		//printf("publish data: call publish data\n");
		ba->publish_data(name_str, (char)strategy, str_opt, str_opt_len, (char *)data_native_ptr, (int)datalen);
		//printf("publish data: called publish data\n");
	}else{
		printf("failed to allocate memory for published data\n");
	}
    	
	(*env).ReleaseByteArrayElements(name, name_ptr, (jint)0);	
	(*env).ReleaseByteArrayElements(data, data_ptr, (jint)0);
	
    if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}	
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_publish_data_direct
 * Signature: (J[BB[BLjava/nio/ByteBuffer;I)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1publish_1data_1direct
  (JNIEnv *env, jobject, jlong ba_ptr, jbyteArray name, jbyte strategy, jbyteArray jstr_opt, jobject jbytebuffer, jint length){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;


	jboolean copy = (jboolean)false;
	jbyte *name_ptr = (*env).GetByteArrayElements(name, &copy);
	int name_length = (*env).GetArrayLength(name);
        const string name_str ((char *)name_ptr, name_length);
	
	void *str_opt;	
	str_opt = 0;
	
	unsigned int str_opt_len = 0;

	if(jstr_opt != NULL && (str_opt_len = (*env).GetArrayLength(jstr_opt)) > 0) {
		str_opt = (void *) (*env).GetByteArrayElements(jstr_opt, &copy);		
	}

	char *data_ptr = (char *)(*env).GetDirectBufferAddress(jbytebuffer);	
	char *data_copy_ptr = (char *)malloc((int)length);	
	if(data_copy_ptr != 0){		
		memcpy((void *)data_copy_ptr, (void *)data_ptr, (int)length);	
		ba->publish_data(name_str, (char)strategy, str_opt, str_opt_len, (char *)data_copy_ptr, (int)length);
	}else{
		printf("failed to allocate memory for published data\n");
	}
		
	(*env).ReleaseByteArrayElements(name, name_ptr, (jint)0);
	if(jstr_opt != 0){	
		(*env).ReleaseByteArrayElements(jstr_opt, (jbyte *)str_opt, (jint)0);		
	}
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    c_delete_event
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_c_1delete_1event  
  (JNIEnv *, jobject, jlong ev_ptr){
	Event *event = (Event *)ev_ptr;
	//printf("delete event %lu\n", (long)event);
	delete event;
}

/*
 * Class:     eu_pursuit_client_nonblock_BlackadderNonBlockWrapper
 * Method:    end
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_eu_pursuit_client_nonblock_BlackadderNonBlockWrapper_end
  (JNIEnv *, jobject, jlong ba_ptr){
	NB_Blackadder *ba;
	ba = (NB_Blackadder *)ba_ptr;	
	ba->end();	
}
