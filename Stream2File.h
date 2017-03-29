#ifndef __STREAM_TO_FILE_H__
#define __STREAM_TO_FILE_H__

#include <stdlib.h>
#include <string>

#include "common.h"
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include "libcurl/curl.h"

using namespace std;

#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "lib/librtmp.lib")
#else					//LINUX
#pragma comment(lib, "libstdc++.a")
#pragma comment(lib, "lib/libcurl.a")
#endif

// starts sockets
//非windows平台如何启动socket的?
inline int InitSockets() {
#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))
	WORD version;
	WSADATA wsaData;

	version = MAKEWORD(1, 1);
	return (WSAStartup(version, &wsaData) == 0);
#else
	return TRUE;
#endif
}

inline void	CleanupSockets() {
#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))
	WSACleanup();
#endif
}

#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
#include <windows.h>
#define LOAD_CURLDLL(path)      LoadLibraryExA(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH)
#define GET_CURLFUNC(h, f)      GetProcAddress(h, #f)
#define UNLOAD_CURLDLL          FreeLibrary
#define DLL_HANDLE				HINSTANCE
#define DLL_NAME				"libcurl.dll"
#elif (defined(ANDROID) || defined(__ANDROID__) || defined(OS_ANDROID))   // android
#include <dlfcn.h>
#define LOAD_CURLDLL(path)      dlopen(path, RTLD_LAZY)
#define GET_CURLFUNC(h, f)      dlsym(h, #f)
#define UNLOAD_CURLDLL          dlclose
#define DLL_HANDLE				void *
#define DLL_NAME				"libcurl.so"
#elif (defined (__APPLE__) || defined(OS_IOS))    // ios/mac
#define LOAD_CURLDLL(path)      0x01
#define GET_CURLFUNC(h, f)      f
#define UNLOAD_CURLDLL          //0x0
#define DLL_HANDLE				int
#define DLL_NAME				"libcurl"
#else       // others
#define LOAD_CURLDLL(path)      0x01
#define GET_CURLFUNC(h, f)      f
#define UNLOAD_CURLDLL          0x0
#define DLL_HANDLE				int
#define DLL_NAME				"libcurl"
#endif
//function pointer (easy interface)
typedef CURLcode (*curl_global_init_p)(long flags);
typedef void     (*curl_global_cleanup_p)(void);
typedef CURL*    (*curl_easy_init_p)(void);
typedef void     (*curl_easy_cleanup_p)(CURL *curl);
typedef CURLcode (*curl_easy_setopt_p)(CURL *curl, CURLoption option, ...);
typedef CURLcode (*curl_easy_perform_p)(CURL *curl);
typedef CURLcode (*curl_easy_getinfo_p)(CURL *curl, CURLINFO info, ...);
typedef struct curl_slist* (*curl_slist_append_p)(struct curl_slist *, const char *);
typedef void     (*curl_slist_free_all_p)(struct curl_slist *);
typedef CURLcode (*curl_easy_send_p)(CURL *curl, const void *buffer, size_t buflen, size_t *n);
typedef CURLcode (*curl_easy_recv_p)(CURL *curl, void *buffer, size_t buflen, size_t *n);
typedef CURLMsg* (*curl_multi_info_read_p)(CURLM *multi_handle, int *msgs_in_queue);
typedef CURLcode (*curl_easy_pause_p)(CURL *handle, int bitmask);
typedef void     (*curl_easy_reset_p)(CURL *curl);
typedef CURL*    (*curl_easy_duphandle_p)(CURL *curl);
typedef const char* (*curl_easy_strerror_p)(CURLcode);

//mulit interface
typedef CURLM*       (*curl_multi_init_p)(void);
typedef CURLMcode    (*curl_multi_add_handle_p)(CURLM *multi_handle, CURL *curl_handle);
typedef CURLMcode    (*curl_multi_remove_handle_p)(CURLM *multi_handle, CURL *curl_handle);
typedef CURLMcode    (*curl_multi_fdset_p)(CURLM *multi_handle, fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd);
typedef CURLMcode    (*curl_multi_wait_p)(CURLM *multi_handle,struct curl_waitfd extra_fds[],unsigned int extra_nfds,int timeout_ms,int *ret);
typedef CURLMcode    (*curl_multi_perform_p)(CURLM *multi_handle, int *running_handles);
typedef CURLMcode    (*curl_multi_cleanup_p)(CURLM *multi_handle);
typedef const char*  (*curl_multi_strerror_p)(CURLMcode);
typedef CURLMcode    (*curl_multi_timeout_p)(CURLM *multi_handle, long *milliseconds);

//other interface
typedef void (*curl_free_p)(void *p);
typedef char* (*curl_easy_escape_p)(CURL *handle, const char *string,  int length);
typedef char* (*curl_easy_unescape_p)(CURL *handle,const char *string,int length, int *outlength);
typedef char* (*curl_getenv_p)(const char *variable);
typedef char* (*curl_version_p)(void);
typedef time_t (*curl_getdate_p)(const char *p, const time_t *unused);
typedef CURLFORMcode (*curl_formadd_p)(struct curl_httppost **httppost,struct curl_httppost **last_post,...);
typedef void (*curl_formfree_p)(struct curl_httppost *form);

unsigned int RTMPStreamToFlv(FILE* outFile, char* streamUrl, int model = 0, uint32_t param = 2 * 1024);
unsigned int HTTPStreamToFlv(FILE* outFile, char* streamUrl, int model = 0, uint32_t param = 2 * 1024);
size_t StreamWriteCallback(char* buffer, size_t size, size_t nmemb, void *outFile);

#endif
