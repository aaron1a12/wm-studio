/*
=================================================================================================
studioDefines.h
Solution-wide definitions and configurations.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   STUDIODEFINES_H
#define   STUDIODEFINES_H

//
// Shared memory path. Must
//
#define STUDIO_MEMORY_PATH "Local\\WildMontageStudio"

//
// Registry Paths
//

#define REGKEY_COMPANY "Software\\Wild Montage"
#define REGKEY_APP REGKEY_COMPANY "\\Studio v0.1.0"
#define REGKEY_PREF REGKEY_APP "\\Preferences"

//
// Defined messages
//

// Open Studio or active its window
#define WMS_LAUNCHSTUDIO	(WM_USER + 0x5100)

// Sent by loginWindow to notify that the user has logged in and
// that his credentials are available.
#define WMS_LOGGEDIN		(WM_USER + 0x5101)

// Send this to the launcher's main window to request a logout.
#define WMS_LOGOUT			(WM_USER + 0x5102)

// Callback used by the tray icon for events.
#define WMS_ICONNOTIFY		(WM_USER + 0x5103)

// Unused at the moment.
#define WMS_HIDESPLASH		(WM_USER + 0x5104)

// Sent by Studio to the launcher. wParam contains the hWnd to Studio.
#define WMS_NEWHWND			(WM_USER + 0x5105)

// Sent by Studio to the launcher. Will return user's productivity score.
#define WMS_GETSCORE		(WM_USER + 0x5106)

// Launcher's timer for scoring analytical points. Runs every second.
#define WMS_SCORETIMER		(WM_USER + 0x5107)

// Launcher's timer for updating our score on the server. Runs, by default, every 30 sec.
#define WMS_SCORESAVETIMER	(WM_USER + 0x5108)

// Message sent by the keyboard hook proc to the main window when a keyboard key is
// lifted after a key press. (Used for analytics)
#define WMS_KEYUP			(WM_USER + 0x5109)

// Activate rendermode
#define WMS_RENDERON		(WM_USER + 0x5110)

// Launcher's timer for switching off render mode.
#define WMS_RENDERTIMER		(WM_USER + 0x5111)

// Sent by Studio to the launcher. Will return seconds left until render mode finishes.
#define WMS_GETRENDERTIME	(WM_USER + 0x5112)

// A signal for the launcher to check for updates.
#define WMS_CHECK4UPDATES	(WM_USER + 0x5113)

// Sent by the launcher to indicate that we just finished checking for updates.
#define WMS_UPDATECHECKED	(WM_USER + 0x5114)

// Sent by StudioClient.dll to display an alarm notification
#define WMS_NOTIFY			(WM_USER + 0x5115)

// Sent by StudioClient.dll to reset a score
#define WMS_RESETSCORE		(WM_USER + 0x5116)

// Send this to the launcher to indicate the connection was dropped.
#define WMS_DROPPEDCON		(WM_USER + 0x5117)

// When the cursor hovers over a part of the titlebar (Studio)
#define WMS_ENTERCAPTION	(WM_USER + 0x5118)

// When the cursor leaves a part of the titlebar (Studio)
#define WMS_LEAVECAPTION	(WM_USER + 0x5119)

// StudioClient.dll, Timer to shutdown PC.
#define WMS_SHUTDOWNTIMER	(WM_USER + 0x5120)

// StudioClient.dll, Timer to shutdown PC.
#define WMS_SHUTDOWNTIMER2	(WM_USER + 0x5121)

// StudioClient.dll, Sound timer
#define WMS_NOTIFYSNDTIMER	(WM_USER + 0x5122)

//
// Notifications
//
#define NOTIFY_PREWORK		1
#define NOTIFY_WORKSTART	2
#define NOTIFY_WORKEND		3
#define NOTIFY_PRESHUTDOWN1	4
#define NOTIFY_PRESHUTDOWN2	5
#define NOTIFY_SHUTDOWN1	6
#define NOTIFY_SHUTDOWN2	7
//
// Misc.
//

// How quickly we score. In seconds.
#define SCORE_INTERVAL		5

// The timeout for when the user has been inactive for too long. In seconds.
#define SCORE_TIMEOUT		20

// How often will we "HTTP-POST" the score to the server? In seconds.
#define SCORE_SAVE			30

// Render mode duration in MINUTES. Default is 10.
// This value is hardcoded into studio.js
#define SCORE_RENDERTIME	10

// How often will we check the user activity? Don't change this.
#define SCORE_INTERVAL_MILL	SCORE_INTERVAL * 1000

// Menu command id for exit in tray icon.
#define CMD_EXITAPP 1

// Schedule timers
#define TIMER_WORK			(WM_USER + 0x6100)

#endif