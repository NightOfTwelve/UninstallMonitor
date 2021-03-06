/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <signal.h>

/* 宏定义begin */
//清0宏
#define MEM_ZERO(pDest, destSize) memset(pDest, 0, destSize)

//LOG宏定义
#define LOG_INFO(tag, msg) __android_log_write(ANDROID_LOG_INFO, tag, msg)
#define LOG_DEBUG(tag, msg) __android_log_write(ANDROID_LOG_DEBUG, tag, msg)
#define LOG_WARN(tag, msg) __android_log_write(ANDROID_LOG_WARN, tag, msg)
#define LOG_ERROR(tag, msg) __android_log_write(ANDROID_LOG_ERROR, tag, msg)

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "uninstall_monitor", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "uninstall_monitor", __VA_ARGS__))

/* 内全局变量begin */
static char c_TAG[] = "uninstall_monitor";
static jboolean b_IS_COPY = JNI_TRUE;

void Java_com_demo_uninstallmonitor_util_AppUtil_beginMonitor(JNIEnv* env,
		jobject thiz, jstring userSerial, jstring url, jstring packageName,
		jstring apkPah) {
	jstring tag = (*env)->NewStringUTF(env, c_TAG);

	// convert Java string to UTF-8
	const char *webUrl = (*env)->GetStringUTFChars(env, url, NULL);
	const char *pkgName = (*env)->GetStringUTFChars(env, packageName, NULL);
	const char *apkDirectory = (*env)->GetStringUTFChars(env, apkPah, NULL);

	LOGI("url:%s", webUrl);
	LOGI("package data directory:%s", pkgName);
	LOGI("apk path:%s", apkDirectory);

	//fork child process to do the monitoring
	pid_t pid = fork();
	//child_pid=pid;
	LOGI("forked child process pid:%d", pid);
	if (pid < 0) {
		//出错log
		LOGE("fork failed!");

	} else if (pid >= 0) {
		/**************************************start observer data directory***********************************************************/
	while(1){
		//开始监听
		LOGI("start observer data directory");

		//子进程目录监听器 "/data/data/com.demo.uninstallmonitor"
		int fileDescriptor = inotify_init();
		if (fileDescriptor < 0) {

			LOGE("inotify_init failed !");

			exit(1);
		}

		//add watch
		int watchDescriptor = inotify_add_watch(fileDescriptor, pkgName,
				IN_DELETE);

		if (watchDescriptor < 0) {
			LOGE("inotify_add_watch failed !");
			exit(1);
		}


		void *event_buffer = malloc(sizeof(struct inotify_event));

		if (event_buffer == NULL) {

			LOGE("malloc failed  !");
			exit(1);
		}

		//If no events have occurred so far, then read() blocks until an event occurs
		 read(fileDescriptor, event_buffer,
				sizeof(struct inotify_event));


		free(event_buffer);

		//remove watch
		inotify_rm_watch(fileDescriptor, IN_DELETE);

		LOGI("directory moved action");
		//apk file is not deleted immediately,so we sleep for a few seconds
		sleep(5);

		/**************************************start apk file check***********************************************************/
		LOGI("start apk file check");
		//use apk to determine if uninstalled
	int	apk_file_descriptor = inotify_init();

		//On success, inotify_add_watch() returns a nonnegative watch descriptor. On error -1 is returned
		int apkDescriptor = inotify_add_watch(apk_file_descriptor, apkDirectory,
				IN_MODIFY);
		LOGI("apk descriptor:%d", apkDescriptor);

		inotify_rm_watch(apk_file_descriptor, IN_MODIFY);

		//apk file was truly deleted
		if (apkDescriptor == -1) {

			//执行命令am start -a android.intent.action.VIEW -d http://www.baidu.com/
			//4.2以上的系统由于用户权限管理更严格，需要加上 --user 0

			LOGI("before loading url");
			if (userSerial == NULL) {

				// 执行命令am start -a android.intent.action.VIEW -d $(url)
				execlp("am", "am", "start", "-a", "android.intent.action.VIEW",
						"-d", webUrl, (char *) NULL);
			} else {
				// 执行命令am start --user userSerial -a android.intent.action.VIEW -d $(url)
				execlp("am", "am", "start", "--user",
						(*env)->GetStringUTFChars(env, userSerial, &b_IS_COPY),
						"-a", "android.intent.action.VIEW", "-d", webUrl,
						(char *) NULL);
			}

			break;
		}

		else if (apkDescriptor >= 1) { //apk file was not deleted

			LOGI("apk exists,go to directory observer");

		}

	}

	} else {
		//父进程直接退出，使子进程被init进程领养，以避免子进程僵死
	}

	// release the Java string and UTF-8
	(*env)->ReleaseStringUTFChars(env, url, webUrl);
	(*env)->ReleaseStringUTFChars(env, packageName, pkgName);
	(*env)->ReleaseStringUTFChars(env, apkPah, apkDirectory);

	LOGI("finish");

	exit(0);

}
