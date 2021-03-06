// CTL.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <stdext.h>
#include "ctlapp.h"
//#include <vld.h>


//namespace qi = boost::spirit::qi;
//namespace ascii = boost::spirit::ascii;
//using namespace boost::phoenix;
//using namespace boost::spirit;
namespace prop_tree = boost::property_tree;

LOCAL_LOGGER_ENABLE(_T("ctl"), LOGGER_LEVEL_DEBUGINFO);

const TCHAR CCTLApp::LOG_CONFIG_FN[] = _T("ctl.cfg");
typedef jcvos::CJCApp<CCTLApp>	CApplication;
CApplication _app;

#define _class_name_	CApplication
BEGIN_ARGU_DEF_TABLE()
	ARGU_DEF(_T("kripke"), 'k', m_kripke_fn,	_T("input file of kripke") )
	ARGU_DEF(_T("exp"), 'e', m_expression,	_T("expression to calculate") )
END_ARGU_DEF_TABLE()

CCTLApp::CCTLApp(void)
{
}

int CCTLApp::Initialize(void)
{
	EnableDstFileParam(_T('o'));
	EnableSrcFileParam(_T('i'));
	return 0;
}

void CCTLApp::CleanUp(void)
{
	RELEASE(m_kripke);
#ifdef _DEBUG
	getc(stdin);
#endif
}

int CCTLApp::Run(void)
{
	// read a Kripke from file
	std::string str_fn;
	jcvos::UnicodeToUtf8(str_fn, m_kripke_fn);
	prop_tree::wptree kripke_pt, pt;
	prop_tree::read_xml(str_fn, pt);
	kripke_pt=pt.get_child(L"Kripke");
	
	//jcvos::auto_interface<CKripke> kripke;
	ReadKripkeFromFile(m_kripke, kripke_pt);

	wprintf_s(L"\n");
	m_kripke->PrintStatus();
	wprintf_s(L"\n");

	// calculate CTL
	jcvos::auto_interface<CPropLabel> sat;
	wprintf_s(L"calculating...   %s\n", m_expression.c_str());
	m_exp_buf = m_expression.c_str();
	m_exp_ptr=0;
	ParseLogicExp(sat);

	wprintf_s(L"\n");
	wprintf_s(L"result: SatM(%s) = ", m_expression.c_str());
	sat->PrintSat(m_kripke);
	wprintf_s(L"\n");
	return 0;
}

void CCTLApp::ReadStrings(CKripke * kripke, std::wstring & str, CKripke::STR_TYPE type)
{
	trim(str);
	wchar_t _str[256];
	wcscpy_s(_str, 255, str.c_str());

	wchar_t * ss=NULL;
	wchar_t * next_token=NULL;

	ss=wcstok_s(_str, L",", &next_token);

	while (ss)
	{
		if (type==CKripke::STATUS) kripke->AddStatus(ss/*, type*/);
		else kripke->AddProposition(ss);
		ss=wcstok_s(NULL, L",", &next_token);
	}
}

bool CCTLApp::ReadKripkeFromFile(CKripke * & kripke, prop_tree::wptree pt)
{
	JCASSERT(kripke==NULL);
	kripke = new jcvos::CDynamicInstance<CKripke>;

	// read status
	std::wstring & str_status=pt.get<std::wstring>(L"status");
	ReadStrings(kripke, str_status, CKripke::STATUS);
	kripke->Initialize();

	std::wstring & str_prop=pt.get<std::wstring>(L"proposition");
	ReadStrings(kripke, str_prop, CKripke::PROPSITION);

	// read connection
	prop_tree::wptree connect_pt=pt.get_child(L"relation");
	kripke->ReadTransition(connect_pt);

	prop_tree::wptree label_pt=pt.get_child(L"label");
	kripke->ReadLabels(label_pt);

	return true;
}



bool CCTLApp::ParseLogicExp(CPropLabel * & res)
{	// logical_exp := <not_exp> and | or | -> | <not_exp> ... 
	JCASSERT(res == NULL);
	std::wstring ap;

	ParseLogicNotExp(res);
	int sym=LookPhrase();
	while (sym != 'O' )
	{
		if (sym != '&' && sym != '|' && sym != 'I' )	return true;
		int op = NextPhrase(ap);	// achieve & or |
		jcvos::auto_interface<CPropLabel> p1(res);	res=NULL;
		jcvos::auto_interface<CPropLabel> p2;
		ParseLogicNotExp(p2);
		switch (op)
		{
		case '&':	CalculateAnd(res, p1, p2, m_kripke->GetStatusNum() );	
			res->SetName(p1->GetName() + L" & " + p2->GetName() );
			break;
		case '|':	CalculateOr(res, p1, p2, m_kripke->GetStatusNum() );	
			res->SetName(p1->GetName() + L" | " + p2->GetName() );
			break;
		case 'I':	CalculateImply(res, p1, p2, m_kripke->GetStatusNum() );
			res->SetName(p1->GetName() + L" -> " + p2->GetName() );
			break;
		}
		sym = LookPhrase();
	}
	res->DebugPrintLabel(m_kripke);
	return true;
}

bool CCTLApp::ParseLogicNotExp(CPropLabel * & res)
{	// not_exp := !<state_exp> | <state_exp>
	JCASSERT(res == NULL);
	int sym=LookPhrase();
	if (sym == '!')
	{
		std::wstring ap;
		sym=NextPhrase(ap);
		jcvos::auto_interface<CPropLabel> p1;
		ParseStateExp(p1);
		CalculateNot(res, p1, m_kripke->GetStatusNum());
		res->SetName(std::wstring(L"!") + p1->GetName());
		res->DebugPrintLabel(m_kripke);
	}
	else
	{
		ParseStateExp(res);
	}
	return true;
}

bool CCTLApp::ParseStateExp(CPropLabel * & res)
{	// state_exp := A/E <path_exp>	| ( <logic_exp> ) | id
	JCASSERT(res == NULL);

	std::wstring ap;
	int sym=NextPhrase(ap);
	switch (sym)
	{
	case '(': {
		ParseLogicExp(res);
		sym=NextPhrase(ap);
		if ( sym!=')' ) THROW_ERROR(ERR_PARAMETER, L"missing ) at char %d", m_exp_ptr);
		res->SetName(std::wstring(L"(") + res->GetName() + L")");
		return true;
			  }

	case 'A':	{
		jcvos::auto_interface<IPathExpression> path_exp;
		ParsePathExp(path_exp);		JCASSERT(path_exp);
		path_exp->CalculateAExp(res, m_kripke);
		res->DebugPrintLabel(m_kripke);
		return true;
				}

	case 'E':	{
		jcvos::auto_interface<IPathExpression> path_exp;
		ParsePathExp(path_exp);		JCASSERT(path_exp);
		path_exp->CalculateEExp(res, m_kripke);
		res->DebugPrintLabel(m_kripke);
		return true;	
				}

	case 'P':	{
		bool br=m_kripke->GetAp(res, ap);
		if (!br || !res) THROW_ERROR(ERR_PARAMETER, L"ap: %s is not defined. char %d", ap.c_str(), m_exp_ptr);
		//res->DebugPrintLabel(m_kripke);
		return true;
				}
	default: {
		THROW_ERROR(ERR_PARAMETER, L"Unknown phrase at char %d", m_exp_ptr);
			 }
	}
	return false;
}

bool CCTLApp::ParsePathExp(IPathExpression * & exp)
{
	JCASSERT(exp==NULL);
	std::wstring ap;
	int sym=NextPhrase(ap);
	switch (sym)
	{	// Unary operator
	case 'X':	case 'F': case 'G': {
		jcvos::auto_interface<CPropLabel> p1;
		ParseLogicExp(p1);
		switch (sym)
		{
		case 'X': exp=new jcvos::CDynamicInstance<CPathExpX>; break;
		case 'F': exp=new jcvos::CDynamicInstance<CPathExpF>; break;
		case 'G': exp=new jcvos::CDynamicInstance<CPathExpG>; break;
		}
		exp->SetOperator(0, p1);
		return true;
				}
	// Binary operator, Must start from (
	case '(':	{
		jcvos::auto_interface<CPropLabel> p1, p2;
		ParseLogicExp(p1);
		sym = NextPhrase(ap);
		switch (sym)
		{
		case 'U': exp=new jcvos::CDynamicInstance<CPathExpU>;	break;
		case 'W': exp=new jcvos::CDynamicInstance<CPathExpW>;	break;
		case 'R': exp=new jcvos::CDynamicInstance<CPathExpR>;	break;
		default: THROW_ERROR(ERR_APP, L"illeagle path expression at char %d", m_exp_ptr);
		}
		ParseLogicExp(p2);
		exp->SetOperator(0, p1);
		exp->SetOperator(1, p2);
		sym = NextPhrase(ap);
		if ( sym!=')' ) THROW_ERROR(ERR_PARAMETER, L"missing ) at char %d", m_exp_ptr);
		return true;
				}
	default:
		THROW_ERROR(ERR_APP, L"illeagle path expression at char %d", m_exp_ptr);
	}
}

inline bool IsSpace(wchar_t ch)
{
	return (ch==' ')|| (ch=='\t') || (ch=='\n') || (ch=='\r');
}

inline bool IsIdentify(wchar_t ch)
{
	return ( (('a'<=ch) && (ch<='z')) || (('0'<=ch) && (ch<='9')) );
}

int CCTLApp::LookPhrase(void)
{
	while ( IsSpace(m_exp_buf[m_exp_ptr]) && 
		m_exp_ptr < MAX_EXPRESS_LEN ) {m_exp_ptr ++;}
	if (m_exp_ptr>=MAX_EXPRESS_LEN || m_exp_buf[m_exp_ptr]==0) return 'O';	// end of string
	wchar_t ch=m_exp_buf[m_exp_ptr];
	if (ch=='A' || ch=='E' || ch=='(' || ch==')' 
		|| ch=='X' || ch=='U' || ch=='F' || ch=='G'
		|| ch=='!'|| ch=='&'|| ch=='|')
	{ return ch; }
	else if (ch=='-' && m_exp_buf[m_exp_ptr+1]=='>')	{ return 'I'; }
	else if (IsIdentify(ch)) return 'P';
	else return 0;
}

int CCTLApp::NextPhrase(std::wstring & ap)
{
	while ( IsSpace(m_exp_buf[m_exp_ptr]) && 
		m_exp_ptr < MAX_EXPRESS_LEN ) {m_exp_ptr ++;}
	if (m_exp_ptr>=MAX_EXPRESS_LEN || m_exp_buf[m_exp_ptr]==0) return 'O';	// end of string

	wchar_t ch=m_exp_buf[m_exp_ptr];
	switch (ch)
	{
	case 'A': case 'E': case '(': case ')': case 'X': case 'U': case 'F': case 'G':
	case '!': case '&': case '|':
		m_exp_ptr++;
		return ch;
	case '-':	{
		if (m_exp_buf[m_exp_ptr+1]=='>') {m_exp_ptr+=2; return 'I';}
		else THROW_ERROR(ERR_PARAMETER, L"Missing > after -, char %d", m_exp_ptr);
		break;	}
	default: {// atomix proposition
		if ( !IsIdentify(ch) ) return 0;	// error
		wchar_t str[64];
		memset(str, 0, 64);
		int ii=0;
		while ( IsIdentify(ch) )
		{
			str[ii++]=ch;
			m_exp_ptr++;
			ch=m_exp_buf[m_exp_ptr];
		}
		ap=str;
		return 'P';		// atomix proposition
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	return jcvos::local_main(argc, argv);
}

