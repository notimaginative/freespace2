// HelpEdLine.h: interface for the HelpEdLine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HELPEDLINE_H__159085D7_C1B5_11D2_9904_00A0CC39C0BE__INCLUDED_)
#define AFX_HELPEDLINE_H__159085D7_C1B5_11D2_9904_00A0CC39C0BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class HelpEdLine : public CObject  
{
public:
	void Draw(CDC *pDC);
	HelpEdLine(CPoint point_from, CPoint point_to);
	HelpEdLine();
	virtual ~HelpEdLine();

private:
	CPoint line_end;
	CPoint line_start;
};

#endif // !defined(AFX_HELPEDLINE_H__159085D7_C1B5_11D2_9904_00A0CC39C0BE__INCLUDED_)

