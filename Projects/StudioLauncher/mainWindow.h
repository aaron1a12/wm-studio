/*
=================================================================================================
mainWindow.h
Here we create the main hidden window for the message loop and timer operations.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#pragma once

#ifndef   MAINWINDOW_H
#define   MAINWINDOW_H

#include <standard.h>

#define MAX_LOADSTRING 100

// Global Variables:
extern HINSTANCE hInst;								// current instance
extern HWND hMainWnd;
extern TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
extern TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

												// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#endif