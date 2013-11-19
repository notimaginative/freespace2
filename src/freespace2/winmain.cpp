/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <windows.h>
#include <process.h>
#include <direct.h>
#include <io.h>


extern int game_main(const char *szCmdLine);


int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow)
{
	int result = -1;

	__try
	{
		result = game_main(szCmdLine);
	}
	__except(RecordExceptionInfo(GetExceptionInformation(), "Freespace 2 Main Thread"))
	{
		// Do nothing here - RecordExceptionInfo() has already done
		// everything that is needed. Actually this code won't even
		// get called unless you return EXCEPTION_EXECUTE_HANDLER from
		// the __except clause.
	}

	return result;
}
