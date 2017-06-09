#pragma once

#include <jcapp.h>
#include "kripke.h"
#include "path_expression.h"

#include <map>
#include <vector>

#define MAX_EXPRESS_LEN 512

class CCTLApp : public jcvos::CJCAppSupport<jcvos::AppArguSupport>
{
protected:
	typedef jcvos::CJCAppSupport<jcvos::AppArguSupport> BaseAppClass;

public:
	static const TCHAR LOG_CONFIG_FN[];
	CCTLApp(void);

public:
	virtual int Initialize(void);
	virtual int Run(void);
	virtual void CleanUp(void);
	virtual LPCTSTR AppDescription(void) const {
		return _T("CTL Calculator, by Jingcheng Yuan\n");
	};

protected:
	void ReadStrings(CKripke * kripke, std::wstring & str, CKripke::STR_TYPE type);
	bool ReadKripkeFromFile(CKripke * & kripke, boost::property_tree::wptree pt);

	// pass next phrase, if it is an atomix proposition, return it to ap;
	int NextPhrase(std::wstring & ap);
	int LookPhrase(void);
	bool ParseStateExp(CPropLabel * & result);
	bool ParsePathExp(IPathExpression * & exp);
	bool ParseLogicExp(CPropLabel * & result);
	bool ParseLogicNotExp(CPropLabel * & res);

protected:
	//wchar_t m_exp_buf[MAX_EXPRESS_LEN];	// buffer of expression
	const wchar_t * m_exp_buf;
	size_t m_exp_ptr;		// current point in expression
	CKripke * m_kripke;

public:
	std::wstring m_kripke_fn;
	std::wstring m_expression;
};
