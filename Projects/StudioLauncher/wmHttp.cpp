/*
=================================================================================================
wmHttp.cpp
A simple libcurl wrapper to make your life easier.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#include "wmHttp.h"
#include "studio.h"
#include <process.h>

/*
* HTTP Variables (Global Scope)
*/

/*
=================================================================================================
wmHttp Method Definitions
=================================================================================================
*/

//
// Make an HTTP request in a synchronous fashion. "responseData" is a pointer
// to a wmHttpResponse struct that will receive the results of the request.
//
void wmHttpRequest(LPCSTR szMethod, LPCSTR szUrl, LPSTR szPostData, wmHttpResponse * responseData) {
	wmHttpRequest(szMethod, szUrl, szPostData, responseData, NULL);
}

//
// Make an HTTP request in an asynchronous fashion. A wmHttpResponse struct will
// be passed to the "callback" as an argument.
//
void wmHttpRequest(LPCSTR szMethod, LPCSTR szUrl, LPSTR szPostData, void *callback) {
	wmHttpRequest(szMethod, szUrl, szPostData, NULL, callback);
}

//
// The internal master method. The overloaded functions above all call this.
//
void wmHttpRequest(LPCSTR szMethod, LPCSTR szUrl, LPSTR szPostData, wmHttpResponse *responseData, void *callback)
{
	bool bAsync = true;	if (callback == NULL) bAsync = false;

	if (bAsync) {
		//
		// Allocate memory for a wmHttpReqArgs pointer.
		// This'll be sent as the "arg" in pthread_create.
		// Important: _wmHttpCurlRequest MUST free this memory.
		//
		wmHttpReqArgs *args = (wmHttpReqArgs*)malloc(sizeof *args);
		args->callback = callback;
		args->url = szUrl;
		args->method = szMethod;

		// Create a mutex with no initial owner
		// ghMutex = CreateMutex(NULL, FALSE, NULL);

		// Call _wmHttpCurlRequest on a new thread
		DWORD ThreadID;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_wmHttpCurlRequest, args, 0, &ThreadID);

		//pthread_t newThread;
		//pthread_create(&newThread, NULL, &_wmHttpCurlRequest, (void *)args);
	}
}

//
// The internal writer method.
//
static int wmHttpWriter(char *data, size_t size, size_t nmemb, std::string *writerData)
{	
	if (writerData == NULL)
		return 0;

	writerData->append(data, size*nmemb);
	return size * nmemb;
}

//
// Internal threaded method that actually makes the HTTP request.
//
DWORD _wmHttpCurlRequest(LPVOID arguments) {
	// WaitForSingleObject(ghMutex, INFINITE);
	// Retrieve the arguments sent by CreateThread
	wmHttpReqArgs *args = (wmHttpReqArgs*)arguments;

	std::string responseBody;
	long statusCode = 0;

	// Make a cURL request
	CURL *curl;
	curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, args->method); //set method
	curl_easy_setopt(curl, CURLOPT_URL, args->url);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wmHttpWriter);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, STUDIO_USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	// Set our custom set of headers	

	string userHeaderStr;
	userHeaderStr = "Username: ";
	userHeaderStr += Studio.Username;

	string passHeaderStr;
	passHeaderStr = "Password: ";
	passHeaderStr += Studio.Password;

	struct curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, userHeaderStr.c_str());
	chunk = curl_slist_append(chunk, passHeaderStr.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	// Fire away!
	curl_easy_perform(curl);

	// Get status code
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

	// Clean up!
	curl_easy_cleanup(curl);

	// Create an HTTP_REQUEST struct and fill it
	// TODO: What happens to this object?
	wmHttpResponse callbackData;
	callbackData.status = statusCode;
	callbackData.text = responseBody.c_str();

	// Call the callback and pass the HTTP_REQUEST
	((void(*)(wmHttpResponse response)) args->callback)(callbackData);

	responseBody.clear();

	// Free the allocated memory!
	free(args);
	//ReleaseMutex(ghMutex);

	return 0;
}

//
// Pass this function when you don't want to define a callback
//
void wmHttpNoCallback(wmHttpResponse response) {
}
