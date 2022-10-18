/*
=================================================================================================
studioLauncher.h
Header for studioLauncher.cpp

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   STUDIOLAUNCHER_H
#define   STUDIOLAUNCHER_H

#include "wmHttp.h"

//
// Global tray icon
//

extern NOTIFYICONDATA niData;

//
// Handle to the login window
//
extern HWND hLoginWnd;

//
// Handle to Studio's process
//
extern HANDLE hStudioProcess;

//
// Used by analyticsUpdate(). Its value is used to count up to a timeout.
//
//extern int currTimeOut;

//
// Invoked when a WM_LOGGEDIN message is sent from the login dialog.
//
void onLogIn();

//
// Invoked when a WM_LOGOUT message is sent from Studio.
//
void onLogOut(bool bDroppedCon = false);

//
// You know what this does.
//
void launchStudio();

//
// Loads preferences from the registry into the Studio struct.
//
void loadPreferences();

//
// Saves preferences from the Studio struct into the registry.
//
void savePreferences();

//
// Check for updates.
//
void checkForUpdates();

//
// Tray icon
//
void createTrayIcon();

//
// After fetching the original score from the server.
//
//void onDownloadScore(wmHttpResponse response);

//
// Enable productivity scoring
//
void enableAnalytics();

//
// Disable productivity scoring
//
void disableAnalytics();

//
// Checks to see whether the user is using a productive program and then updates his 
// or her score accordingly. Runs, by default, every 5 seconds.
//
void analyticsUpdate();

//
// Makes an HTTP POST request, containing user score, to a php script which saves
// the scores in the MySQL database.
//
void analyticsSave();

//
// Called after saving score. Will check to see if we still have an internet connection.
//
void analyticsSaveCallback(wmHttpResponse response);

//
// Given information about the active program, test against a hardcoded list to see
// whether the activated app is a "productive" app.
//
bool analyticsValidateApp(string windowTitle, string windowClass, string windowProcess);

//
// Same as analyticsValidateApp except only for MIDI-capable music programs.
//
bool analyticsValidateMidiApp(string windowTitle, string windowClass, string windowProcess);

//
// Validation for educational apps. (Rosetta Stone)
//
bool analyticsValidateEduApp(string windowTitle, string windowClass, string windowProcess);

//
// Fires when a WMS_KEYUP is received by the main window.
//
void analyticsOnKeyUp();

//
// Turns on render mode for 10 mins.
//
void enableRenderMode();

//
// Disables render mode and kills timer.
//
void disableRenderMode();

//
// Returns, in seconds, the time left in Render Mode.
//
int getRenderTime();

//
// Show a notification by its id.
//
void showNotification(int notificationId);
#endif