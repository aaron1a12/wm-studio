/*
=================================================================================================
studio.h
Global studio struct.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   STUDIO_H
#define   STUDIO_H

#include "version.h"
#include <string>
#include <windows.h>

using namespace std;

//
// This struct contains most of Studio's global variables.
//

extern struct StudioStruct {
	string Username;
	string Password;
	bool RememberUser;
	bool LoggedIn;
	int Score;
	bool RenderMode;

	// For use by launcher

	HWND hStudioView;
	POINT MousePos;
} Studio;

#endif