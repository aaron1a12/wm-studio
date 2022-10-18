/*
=================================================================================================
schedule.h
Setup notifiers for the schedule.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#ifndef   SCHEDULE_H
#define   SCHEDULE_H

//
// Headers
//

#include "standard.h"

/*
=================================================================================================
Global Scope
=================================================================================================
*/

HANDLE hScheduleTimerQueue;
HANDLE hSchedule_WorkTimer;

/*
=================================================================================================
Method Definitions
=================================================================================================
*/

//void 

DWORD _notify(LPVOID arguments) {
	return 0;
}

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	//PHANDLE selfHandle = (PHANDLE)lpParam;

	wmNotify("schedule TIMER");

	DWORD ThreadID;
//	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_notify, args, 0, &ThreadID);

	//DeleteTimerQueueTimer(hScheduleTimerQueue, selfHandle, NULL);
}

void addScheduleTimer(PHANDLE phNewTimer, int hour, int min, int sec, char* message)
{
	time_t rightNow;
	struct tm workTime;

	time(&rightNow);

	workTime = *localtime(&rightNow);
	workTime.tm_hour = hour;
	workTime.tm_min = min;
	workTime.tm_sec = sec;

	int untilWorkTime = (int)floor(difftime(mktime(&workTime), rightNow));

	//if (untilWorkTime > 0) {
		/*char buffer[20];
		_snprintf(buffer, 20, "%d", untilWorkTime * 1000);
		MessageBoxA(NULL, (const char*)buffer, "Time left", MB_OK | MB_ICONINFORMATION);*/

		CreateTimerQueueTimer(&hSchedule_WorkTimer, NULL, (WAITORTIMERCALLBACK)TimerRoutine, phNewTimer, 1000, 0, WT_EXECUTEONLYONCE);
	//}
}

//
// Setup the schedule notifiers
//
void setupSchedule()
{
	//wmNotify("schedule TIMER");
	addScheduleTimer(&hSchedule_WorkTimer, 7,43,0, "Work time is here!");	



	/*char buffer[20];
	_snprintf(buffer, 20, "%d", untilWorkTime * 1000);
	MessageBoxA(NULL, (const char*)buffer, "Time left", MB_OK | MB_ICONINFORMATION);*/

	//SetTimer(hMainWnd, (UINT_PTR)TIMER_WORK, 2000, (TIMERPROC)TimerProc);
	/*
	HANDLE hTimer = NULL;
	HANDLE hTimerQueue = NULL;
	int arg = 123;


	hTimerQueue = CreateTimerQueue();

	CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)TimerRoutine, &arg, 5000, 0, 0);*/
	//wmNotify("????");
}

//
// Reset the schedule (for another day)
//
void resetSchedule()
{
	// Delete all timers in the timer queue.
	DeleteTimerQueue(hScheduleTimerQueue);
}

#endif