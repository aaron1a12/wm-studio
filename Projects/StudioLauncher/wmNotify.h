/*
=================================================================================================
wmNotify.h
Show a notification on the lower-right corner of the screen.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   WMNOTIFY_H
#define   WMNOTIFY_H

//
// Headers
//

#include "standard.h"
#include <studioDefines.h>
#include <Mmsystem.h>

/*
=================================================================================================
Global Scope
=================================================================================================
*/

extern HWND hWndNotify;
extern char* notificationText;

/*
=================================================================================================
Method Definitions
=================================================================================================
*/

//
// Show the notification
//
void wmNotify(char* notifyMsg);

#endif