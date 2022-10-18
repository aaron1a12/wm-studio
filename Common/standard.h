/*
=================================================================================================
standard.h
When you get tired of including sh*t :P

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#pragma once
// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN 
// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <string>
//#include <mbstring.h>

// TODO: useless
#include <stdio.h>

#include "Commctrl.h"

#include "Shellapi.h"
#include "Shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

/*
* Enable "XP" themes (and common controls)
*/

#pragma comment(linker, "\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls'   \
	version='6.0.0.0'                          \
	processorArchitecture='*'            \
	publicKeyToken='6595b64144ccf1df'          \
	language='*'\"")