/*
=================================================================================================
wmHttp.h
A simple libcurl wrapper to make your life easier.

Example Use:

void onReq(HTTP_REQUEST response) {
// response.text
}

...

wmHttpInit();
wmHttpConnect(L"studio.wildmontage.com");

// Sync
wmHttpResponse response;
wmHttpRequest("GET", "http://studio.wildmontage.com/app/foo.php", NULL, &response);

// Async
wmHttpRequest("GET", "http://studio.wildmontage.com/app/foo.php", NULL, &onReq);

wmHttpClose();

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   WMHTTP_H
#define   WMHTTP_H

/*
* User agent
*/
#define WMHTTP_USERAGENT "Studio/0.1.0 (+http://www.wildmontage.com)"

/*
* Standard headers
*/

#include "standard.h"
#include "version.h"
#include "curl.h"

/*
* HTTP Variables (Global Scope)
*/

//
// This struct is given to _wmHttpCurlRequest() and will allow us to pass
// more than one argument to it since threaded functions only allow one.
//
struct wmHttpReqArgs {  // OLD CODE: typedef struct wmHttpReqArgs
	//int id;
	const char *method;
	const char *url;
	void *callback;
};

//
// This struct will contain the results of the request.
//
struct wmHttpResponse {
	int status;
	const char* text;
};

/*
=================================================================================================
wmHttp Method Declarations
=================================================================================================
*/

//
// Make an HTTP request in a synchronous fashion. "responseData" is a pointer
// to an HTTP_REQUEST struct that will receive the results of the request.
//
void wmHttpRequest(LPCSTR szMethod, LPCSTR szUrl, LPSTR szPostData, wmHttpResponse * responseData);

//
// Make an HTTP request in an asynchronous fashion. An HTTP_REQUEST struct will
// be passed to the "callback" as an argument.
//
void wmHttpRequest(LPCSTR szMethod, LPCSTR szUrl, LPSTR szPostData, void *callback);

//
// The internal master method. Don't use this method.
//
void wmHttpRequest(LPCSTR szMethod, LPCSTR szUrl, LPSTR szPostData, wmHttpResponse *responseData, void *callback);

//
// The internal writer method. Don't use this method.
//
static int wmHttpWriter(char *data, size_t size, size_t nmemb, std::string *writerData);

//
// Internal threaded method that actually makes the HTTP request. Don't use this method.
//
DWORD _wmHttpCurlRequest(LPVOID arguments);

//
// Pass this function when you don't want to define a callback
//
void wmHttpNoCallback(wmHttpResponse response);

#endif