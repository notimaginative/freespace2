/*
 * $Logfile: /Freespace2/code/Math/Fix.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * Code to deal with 16.16 fixed point numbers.
 *
 * $Log$
 * Revision 1.2  2002/05/07 03:16:46  theoddone33
 * The Great Newline Fix
 *
 * Revision 1.1.1.1  2002/05/03 03:28:09  root
 * Initial import.
 *
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:49a Dave
 * 
 * 2     2/17/97 5:18p John
 * Added a bunch of RCS headers to a bunch of old files that don't have
 * them.
 *
 * $NoKeywords: $
 */


#ifndef PLAT_UNIX
#include <windows.h>
#endif

#include "pstypes.h"
#include "fix.h"

fix fixmul(fix a, fix b)
{
	longlong tmp;
	tmp = (longlong)a * (longlong)b;
	return (fix)(tmp>>16);
}

fix fixdiv(fix a, fix b)
{
	return MulDiv(a,65536,b);
}

fix fixmuldiv(fix a, fix b,fix c)
{
	return MulDiv(a,b,c);
}

