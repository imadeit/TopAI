#include "stdafx.h"

#include "kripke.h"
//namespace qi = boost::spirit::qi;
//namespace ascii = boost::spirit::ascii;
//using namespace boost::phoenix;
//using namespace boost::spirit;
namespace prop_tree = boost::property_tree;


CKripke::CKripke(void) : m_status_num(0), m_transition(NULL)
{
}

CKripke::~CKripke(void)
{
	delete [] m_transition;
	PROP_MAP_ITERATOR it = m_prop_map.begin(), endit=m_prop_map.end();
	for (;it!=endit; ++it)	{ RELEASE(it->second); }
}

void CKripke::ReadStringsForRelation(std::wstring & str, size_t s1, CKripke::STR_TYPE type)
{
	JCASSERT(s1 < m_status_num);
	trim(str);
	wchar_t _str[256];
	wcscpy_s(_str, 255, str.c_str());

	wchar_t * ss=NULL;
	wchar_t * next_token=NULL;

	ss=wcstok_s(_str, L",", &next_token);

	while (ss)
	{
		if (type==TRANSITION)
		{	// add a connection
			STR_ITERATOR it = m_status_map.find(ss);
			if (it==m_status_map.end()) THROW_ERROR(ERR_PARAMETER, L"status '%s' is undefined", ss);
			Connect(s1, it->second);
		}
		else if (type == LABEL)
		{
			PROP_MAP_ITERATOR it = m_prop_map.find(ss);
			if (it==m_prop_map.end()) THROW_ERROR(ERR_PARAMETER, L"prop '%s' is undefined", ss);
			CPropLabel * prop=it->second;	JCASSERT(prop);
			char * label=prop->GetLabel();	JCASSERT(label);
			label[s1] = 1;
		}
		else JCASSERT(0);
		ss=wcstok_s(NULL, L",", &next_token);
	}
}

void CKripke::ReadTransition(boost::property_tree::wptree & pt)
{
	for (size_t ss=0; ss<m_status_num; ++ss)
	{
		std::wstring str_status=m_status_names[ss];
		std::wstring & str_tran=pt.get<std::wstring>(str_status);
		ReadStringsForRelation(str_tran, ss, TRANSITION);
	}
}

void CKripke::ReadLabels(boost::property_tree::wptree & pt)
{
	for (size_t ss=0; ss<m_status_num; ++ss)
	{
		std::wstring str_status=m_status_names[ss];
		boost::optional<std::wstring> opt_prop = pt.get_optional<std::wstring>(str_status);
		if (opt_prop) ReadStringsForRelation(*opt_prop, ss, LABEL);
	}
}

void CKripke::AddStatus(const std::wstring & ss)
{
	std::pair<STR_ITERATOR, bool> ir = m_status_map.insert(std::pair<std::wstring, size_t>(ss, m_status_num));
	if (ir.second == false)		THROW_ERROR(ERR_PARAMETER, L"status %s was double defined", ss.c_str());
	m_status_names.push_back(ss);
	m_status_num++;
}

void CKripke::PrintStatus(void)
{
	wprintf_s(L"status & relations: (%d)\n", m_status_num);
	for (size_t ss=0; ss<m_status_num; ++ss)
	{
		//wprintf_s(L"\t%d: %s -> ", ss, m_status_names[ss].c_str());
		wprintf_s(L"\t%s -> ", m_status_names[ss].c_str());
		for (size_t s2=0; s2<m_status_num; ++s2)
		{
			if ( GetTransition(ss, s2) )
			{
				wprintf_s(L"%s,", m_status_names[s2].c_str());
			}
		}
		wprintf_s(L"\n");
	}

	wprintf_s(L"propsition: (%d)\n", m_prop_map.size());
	PROP_MAP_ITERATOR it = m_prop_map.begin(), endit=m_prop_map.end();
	for (;it!=endit; ++it)
	{
		CPropLabel * prop = it->second;	JCASSERT(prop);
		wprintf_s(L"\t%s: ", (it->first).c_str());
		prop->PrintSat(this);
		wprintf_s(L"\n");
	}
}

void CKripke::Initialize(void)
{
	JCASSERT(m_transition==NULL);
	m_tran_size=m_status_num * m_status_num;
	m_transition=new char[m_tran_size];
	memset(m_transition, 0, m_tran_size);
}

void CKripke::Connect(size_t s1, size_t s2)
{
	size_t ii=s1*m_status_num +s2;
	JCASSERT(ii<m_tran_size);
	m_transition[ii]=1;
}

char CKripke::GetTransition(size_t s1, size_t s2)
{
	size_t ii=s1*m_status_num +s2;
	JCASSERT(ii<m_tran_size);
	return (m_transition[ii]);
}

void CKripke::AddProposition(const std::wstring & ap_name)
{
	JCASSERT(m_status_num > 0);
	CPropLabel * prop = new jcvos::CDynamicInstance<CPropLabel>;
	prop->CreateLabel(m_status_num);
	prop->SetName(ap_name);

	std::pair<PROP_MAP_ITERATOR, bool> ir=m_prop_map.insert(std::pair<std::wstring, CPropLabel*>(ap_name, prop));
	if (ir.second == false)		THROW_ERROR(ERR_PARAMETER, L"proposition %s was double defined", ap_name.c_str());
}

bool CKripke::GetAp(CPropLabel * & prop, const std::wstring & ap_name)
{
	JCASSERT(prop==NULL);
	PROP_MAP_ITERATOR it=m_prop_map.find(ap_name);
	if (it==m_prop_map.end()) return false;
	prop=(it->second);	JCASSERT(prop);
	prop->AddRef();
	return true;
}

STATUS_TYPE CKripke::GetFirstRelation(STATUS_TYPE s0)
{
	JCASSERT(s0 < m_status_num);
	JCASSERT(m_transition);

	char * relation=m_transition + s0 * m_status_num;
	STATUS_TYPE ss=0;
	for (; ss< m_status_num; ++ss)	if (relation[ss]) return ss;
	return 0xFFFFFFFF;
}

bool CKripke::GetNextRelation(STATUS_TYPE s0, STATUS_TYPE & r)
{
	JCASSERT(s0 < m_status_num && r<m_status_num);
	JCASSERT(m_transition);

	char * relation=m_transition + s0 * m_status_num;
	r++;
	for (;r < m_status_num; ++r) if (relation[r]) return true;
	r=0xFFFFFFFF;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- CPropLabel
CPropLabel::CPropLabel(void) : m_labels(NULL), m_length(0)
{
}

CPropLabel::~CPropLabel(void)
{
	delete [] m_labels;
}

void CPropLabel::CreateLabel(size_t size)
{
	JCASSERT(m_labels==NULL);
	m_labels=new char[size];
	memset(m_labels, 0, size);
	m_length=size;
}


void CPropLabel::PrintSat(CKripke * kripke)
{
	JCASSERT(kripke);
	wprintf_s(L"{");
	for (size_t ss=0; ss<m_length; ++ss)
	{
		if (m_labels[ss])	wprintf_s(L"%s,", kripke->GetStatusName(ss).c_str());
	}
	wprintf_s(L"}");
}

void CPropLabel::DebugPrintLabel(CKripke * kripke)
{
	JCASSERT(kripke);
	wprintf_s(L"    SatM(%s)={", m_prop_name.c_str() );
	for (size_t ss=0; ss<m_length; ++ss)
	{
		if (m_labels[ss])	wprintf_s(L"%s,", kripke->GetStatusName(ss).c_str());
	}
	wprintf_s(L"}\n");
}

void CreatePropLabel(CPropLabel *& prop, size_t len)
{
	JCASSERT(prop==NULL && len > 0);
	prop = new jcvos::CDynamicInstance<CPropLabel>;	JCASSERT(prop);
	prop->CreateLabel(len);
}

