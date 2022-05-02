/*
 * Copyright (c) 2003, 2022, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "jvmti.h"
#include "jvmti_common.h"

extern "C" {


#define PASSED 0
#define STATUS_FAILED 2

typedef struct {
  jfieldID fid;
  char *m_cls;
  char *m_name;
  char *m_sig;
  jlocation loc;
  char *f_cls;
  char *f_name;
  char *f_sig;
  jboolean is_static;
  jvalue val;
} writable_watch_info;

typedef struct {
  jfieldID fid;
  const char *m_cls;
  const char *m_name;
  const char *m_sig;
  jlocation loc;
  const char *f_cls;
  const char *f_name;
  const char *f_sig;
  jboolean is_static;
  jvalue val;
} watch_info;

static jvmtiEnv *jvmti;
static jvmtiEventCallbacks callbacks;
static jvmtiCapabilities caps;
static jint result = PASSED;
static volatile jboolean isVirtualExpected = JNI_FALSE;
static int eventsExpected = 0;
static int eventsCount = 0;
static watch_info watches[] = {
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticBoolean", "Z", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticByte", "B", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticShort", "S", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticInt", "I", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticLong", "J", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticFloat", "F", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticDouble", "D", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticChar", "C", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticObject", "Ljava/lang/Object;", JNI_TRUE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "staticArrInt", "[I", JNI_TRUE, {} },

    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceBoolean", "Z", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceByte", "B", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceShort", "S", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceInt", "I", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceLong", "J", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceFloat", "F", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceDouble", "D", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceChar", "C", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceObject", "Ljava/lang/Object;", JNI_FALSE, {} },
    { NULL, "Lfieldmod02;", "check", "(Ljava/lang/Object;)I", 0,
        "Lfieldmod02a;", "instanceArrInt", "[I", JNI_FALSE, {} }
};

void printValue(jvalue val, char *sig) {
  switch (*sig) {
    case 'J':
      LOG("0x%x%08x", (jint)(val.j >> 32), (jint)val.j);
      break;
    case 'F':
      LOG("%.3f", (double)val.f);
      break;
    case 'D':
      LOG("%f", (double)val.d);
      break;
    case 'L':
    case '[':
      LOG("0x%p", val.l);
      break;
    case 'Z':
      LOG("0x%x", val.z);
      break;
    case 'B':
      LOG("%d", val.b);
      break;
    case 'S':
      LOG("%d", val.s);
      break;
    case 'C':
      LOG("0x%x", val.c);
      break;
    case 'I':
      LOG("%d", val.i);
      break;
    default:
      LOG("0x%x%08x", (jint)(val.j >> 32), (jint)val.j);
      break;
  }
}

int isEqual(JNIEnv *jni, char *sig, jvalue v1, jvalue v2) {
  switch (*sig) {
    case 'J':
      return (v1.j == v2.j);
    case 'F':
      return (v1.f == v2.f);
    case 'D':
      return (v1.d == v2.d);
    case 'L':
    case '[':
      return jni->IsSameObject(v1.l, v2.l);
    case 'Z':
      return (v1.z == v2.z);
    case 'B':
      return (v1.b == v2.b);
    case 'S':
      return (v1.s == v2.s);
    case 'C':
      return (v1.c == v2.c);
    case 'I':
      return (v1.i == v2.i);
    default:
      return (1);
  }
}

void JNICALL FieldModification(jvmtiEnv *jvmti, JNIEnv *jni,
                               jthread thr, jmethodID method, jlocation location,
                               jclass field_klass, jobject obj,
                               jfieldID field, char sig, jvalue new_value) {
  jvmtiError err;
  jclass cls;
  writable_watch_info watch;
  char *generic;

  eventsCount++;

  LOG(">>> retrieving modification watch info ...\n");

  watch.fid = field;
  watch.loc = location;
  watch.is_static = (obj == NULL) ? JNI_TRUE : JNI_FALSE;
  watch.val = new_value;
  err = jvmti->GetMethodDeclaringClass(method, &cls);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetMethodDeclaringClass) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    result = STATUS_FAILED;
    return;
  }
  err = jvmti->GetClassSignature(cls,
                                     &watch.m_cls, &generic);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetClassSignature) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    result = STATUS_FAILED;
    return;
  }
  err = jvmti->GetMethodName(method, &watch.m_name, &watch.m_sig, &generic);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetMethodName) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    result = STATUS_FAILED;
    return;
  }
  err = jvmti->GetClassSignature(field_klass,
                                     &watch.f_cls, &generic);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetClassSignature) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    result = STATUS_FAILED;
    return;
  }
  err = jvmti->GetFieldName(field_klass, field,
                                &watch.f_name, &watch.f_sig, &generic);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetFieldName) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    result = STATUS_FAILED;
    return;
  }

  LOG(">>>      class: \"%s\"\n", watch.m_cls);
  LOG(">>>     method: \"%s%s\"\n", watch.m_name, watch.m_sig);
  LOG(">>>   location: 0x%x%08x\n",
         (jint)(watch.loc >> 32), (jint)watch.loc);
  LOG(">>>  field cls: \"%s\"\n", watch.f_cls);
  LOG(">>>      field: \"%s:%s\"\n", watch.f_name, watch.f_sig);
  LOG(">>>     object: 0x%p\n", obj);
  LOG(">>>  new value: ");
  printValue(watch.val, watch.f_sig);
  LOG("\n");

  for (size_t i = 0; i < sizeof(watches)/sizeof(watch_info); i++) {
    if (watch.fid == watches[i].fid) {
      if (watch.m_cls == NULL ||
          strcmp(watch.m_cls, watches[i].m_cls) != 0) {
        LOG("(watch#%" PRIuPTR ") wrong class: \"%s\", expected: \"%s\"\n",
               i, watch.m_cls, watches[i].m_cls);
        result = STATUS_FAILED;
      }
      if (watch.m_name == NULL ||
          strcmp(watch.m_name, watches[i].m_name) != 0) {
        LOG("(watch#%" PRIuPTR ") wrong method name: \"%s\"",
               i, watch.m_name);
        LOG(", expected: \"%s\"\n", watches[i].m_name);
        result = STATUS_FAILED;
      }
      if (watch.m_sig == NULL ||
          strcmp(watch.m_sig, watches[i].m_sig) != 0) {
        LOG("(watch#%" PRIuPTR ") wrong method sig: \"%s\"",
               i, watch.m_sig);
        LOG(", expected: \"%s\"\n", watches[i].m_sig);
        result = STATUS_FAILED;
      }
      if (watch.loc != watches[i].loc) {
        LOG("(watch#%" PRIuPTR ") wrong location: 0x%x%08x",
               i, (jint)(watch.loc >> 32), (jint)watch.loc);
        LOG(", expected: 0x%x%08x\n",
               (jint)(watches[i].loc >> 32), (jint)watches[i].loc);
        result = STATUS_FAILED;
      }
      if (watch.f_name == NULL ||
          strcmp(watch.f_name, watches[i].f_name) != 0) {
        LOG("(watch#%" PRIuPTR ") wrong field name: \"%s\"",
               i, watch.f_name);
        LOG(", expected: \"%s\"\n", watches[i].f_name);
        result = STATUS_FAILED;
      }
      if (watch.f_sig == NULL ||
          strcmp(watch.f_sig, watches[i].f_sig) != 0) {
        LOG("(watch#%" PRIuPTR ") wrong field sig: \"%s\"",
               i, watch.f_sig);
        LOG(", expected: \"%s\"\n", watches[i].f_sig);
        result = STATUS_FAILED;
      }
      if (watch.is_static != watches[i].is_static) {
        LOG("(watch#%" PRIuPTR ") wrong field type: %s", i,
               (watch.is_static == JNI_TRUE) ? "static" : "instance");
        LOG(", expected: %s\n",
               (watches[i].is_static == JNI_TRUE) ? "static" : "instance");
        result = STATUS_FAILED;
      }
      if (!isEqual((JNIEnv *)jni, watch.f_sig, watch.val, watches[i].val)) {
        LOG("(watch#%" PRIuPTR ") wrong new value: ", i);
        printValue(watch.val, watch.f_sig);
        LOG(", expected: ");
        printValue(watches[i].val, watch.f_sig);
        LOG("\n");
        result = STATUS_FAILED;
      }
      jboolean isVirtual = jni->IsVirtualThread(thr);
      if (isVirtualExpected != isVirtual) {
        LOG("The thread IsVirtualThread %d differs from expected %d.\n", isVirtual, isVirtualExpected);
        result = STATUS_FAILED;
      }
      return;
    }
  }
  LOG("Unexpected field modification catched: 0x%p\n", watch.fid);
  result = STATUS_FAILED;
}

#ifdef STATIC_BUILD
JNIEXPORT jint JNICALL Agent_OnLoad_fieldmod02(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNICALL Agent_OnAttach_fieldmod02(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNI_OnLoad_fieldmod02(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif
jint Agent_Initialize(JavaVM *jvm, char *options, void *reserved) {
  jvmtiCapabilities caps;
  jvmtiError err;
  jint res;

  res = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_1);
  if (res != JNI_OK || jvmti == NULL) {
    LOG("Wrong result of a valid call to GetEnv!\n");
    return JNI_ERR;
  }


  memset(&caps, 0, sizeof(jvmtiCapabilities));
  caps.can_generate_field_modification_events = 1;
  caps.can_support_virtual_threads = 1;

  err = jvmti->AddCapabilities(&caps);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(AddCapabilities) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    return JNI_ERR;
  }

  err = jvmti->GetCapabilities(&caps);
  if (err != JVMTI_ERROR_NONE) {
    LOG("(GetCapabilities) unexpected error: %s (%d)\n",
           TranslateError(err), err);
    return JNI_ERR;
  }

  if (caps.can_generate_field_modification_events) {
    callbacks.FieldModification = &FieldModification;
    err = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
      LOG("(SetEventCallbacks) unexpected error: %s (%d)\n",
             TranslateError(err), err);
      return JNI_ERR;
    }

    err = jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                                          JVMTI_EVENT_FIELD_MODIFICATION, NULL);
    if (err != JVMTI_ERROR_NONE) {
      LOG("Failed to enable JVMTI_EVENT_FIELD_MODIFICATION: %s (%d)\n",
             TranslateError(err), err);
      return JNI_ERR;
    }
  } else {
    LOG("Warning: FieldModification watch is not implemented\n");
  }

  return JNI_OK;
}

JNIEXPORT void JNICALL
Java_fieldmod02_getReady(JNIEnv *jni, jclass clz) {
  jvmtiError err;
  jclass cls;
  jmethodID ctor;
  jintArray arr1, arr2;
  jobject obj1, obj2;
  jthread thread;

  err = jvmti->GetCurrentThread(&thread);
  if (err != JVMTI_ERROR_NONE) {
    LOG("Failed to get current thread: %s (%d)\n", TranslateError(err), err);
    result = STATUS_FAILED;
    return;
  }

  eventsCount = 0;
  eventsExpected = 0;
  isVirtualExpected = jni->IsVirtualThread(thread);

  LOG(">>> setting field modification watches ...\n");

  cls = jni->FindClass("fieldmod02a");
  if (cls == NULL) {
    LOG("Cannot find fieldmod02a class!\n");
    result = STATUS_FAILED;
    return;
  }
  for (size_t i = 0; i < sizeof(watches)/sizeof(watch_info); i++) {
    if (watches[i].is_static == JNI_TRUE) {
      watches[i].fid = jni->GetStaticFieldID(
          cls, watches[i].f_name, watches[i].f_sig);
    } else {
      watches[i].fid = jni->GetFieldID(
          cls, watches[i].f_name, watches[i].f_sig);
    }
    if (watches[i].fid == NULL) {
      LOG("Cannot get field ID for \"%s:%s\"\n",
             watches[i].f_name, watches[i].f_sig);
      result = STATUS_FAILED;
      return;
    }
    err = jvmti->SetFieldModificationWatch(cls, watches[i].fid);
    if (err == JVMTI_ERROR_NONE) {
      eventsExpected++;
    } else {
      LOG("(SetFieldModificationWatch#%" PRIuPTR ") unexpected error: %s (%d)\n",
             i, TranslateError(err), err);
      result = STATUS_FAILED;
    }
  }

  ctor = jni->GetMethodID(cls, "<init>", "()V");
  obj1 = jni->NewGlobalRef(jni->NewObject(cls, ctor));
  obj2 = jni->NewGlobalRef(jni->NewObject(cls, ctor));
  arr1 = (jintArray) jni->NewGlobalRef(jni->NewIntArray((jsize) 1));
  arr2 = (jintArray) jni->NewGlobalRef(jni->NewIntArray((jsize) 1));

  watches[0].val.z = JNI_TRUE;
  watches[1].val.b = 1;
  watches[2].val.s = 2;
  watches[3].val.i = 3;
  watches[4].val.j = 4;
  watches[5].val.f = 0.5F;
  watches[6].val.d = 0.6;
  watches[7].val.c = 0x7;
  watches[8].val.l = obj1;
  watches[9].val.l = arr1;

  watches[10].val.z = JNI_FALSE;
  watches[11].val.b = 10;
  watches[12].val.s = 20;
  watches[13].val.i = 30;
  watches[14].val.j = 40;
  watches[15].val.f = 0.05F;
  watches[16].val.d = 0.06;
  watches[17].val.c = 0x70;
  watches[18].val.l = obj2;
  watches[19].val.l = arr2;

  LOG(">>> ... done\n");

}

JNIEXPORT jint JNICALL
Java_fieldmod02_check(JNIEnv *jni, jclass clz, jobject obj) {
  jclass cls;

  LOG(">>> modifying fields ...\n");


  cls = jni->FindClass("fieldmod02a");
  if (cls == NULL) {
    LOG("Cannot find fieldmod02a class!\n");
    return STATUS_FAILED;
  }

  jni->SetStaticBooleanField(cls, watches[0].fid, watches[0].val.z);
  jni->SetStaticByteField(cls, watches[1].fid, watches[1].val.b);
  jni->SetStaticShortField(cls, watches[2].fid, watches[2].val.s);
  jni->SetStaticIntField(cls, watches[3].fid, watches[3].val.i);
  jni->SetStaticLongField(cls, watches[4].fid, watches[4].val.j);
  jni->SetStaticFloatField(cls, watches[5].fid, watches[5].val.f);
  jni->SetStaticDoubleField(cls, watches[6].fid, watches[6].val.d);
  jni->SetStaticCharField(cls, watches[7].fid, watches[7].val.c);
  jni->SetStaticObjectField(cls, watches[8].fid, watches[8].val.l);
  jni->SetStaticObjectField(cls, watches[9].fid, watches[9].val.l);

  jni->SetBooleanField(obj, watches[10].fid, watches[10].val.z);
  jni->SetByteField(obj, watches[11].fid, watches[11].val.b);
  jni->SetShortField(obj, watches[12].fid, watches[12].val.s);
  jni->SetIntField(obj, watches[13].fid, watches[13].val.i);
  jni->SetLongField(obj, watches[14].fid, watches[14].val.j);
  jni->SetFloatField(obj, watches[15].fid, watches[15].val.f);
  jni->SetDoubleField(obj, watches[16].fid, watches[16].val.d);
  jni->SetCharField(obj, watches[17].fid, watches[17].val.c);
  jni->SetObjectField(obj, watches[18].fid, watches[18].val.l);
  jni->SetObjectField(obj, watches[19].fid, watches[19].val.l);

  LOG(">>> ... done\n");

  if (eventsCount != eventsExpected) {
    LOG("Wrong number of field modification events: %d, expected: %d\n",
           eventsCount, eventsExpected);
    result = STATUS_FAILED;
  }

  for (size_t i = 0; i < sizeof(watches)/sizeof(watch_info); i++) {
    jvmtiError err = jvmti->ClearFieldModificationWatch(cls, watches[i].fid);
    if (err == JVMTI_ERROR_NONE) {
      eventsExpected++;
    } else {
      LOG("(ClearFieldModificationWatch#%" PRIuPTR ") unexpected error: %s (%d)\n",
             i, TranslateError(err), err);
      result = STATUS_FAILED;
    }
  }
  return result;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
  return Agent_Initialize(jvm, options, reserved);
}

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *jvm, char *options, void *reserved) {
  return Agent_Initialize(jvm, options, reserved);
}

}
