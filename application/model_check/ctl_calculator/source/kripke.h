#pragma once

#include <stdext.h>
#include <map>
#include <vector>

typedef size_t	STATUS_TYPE;
typedef size_t	PROP;

class CKripke;

///////////////////////////////////////////////////////////////////////////////
// -- CPropLabel

class CPropLabel : public IJCInterface
{
public:
	CPropLabel(void);
	~CPropLabel(void);
public:
	void CreateLabel(size_t size);
	void SetName(const std::wstring name) {m_prop_name = name;}; 
	const std::wstring & GetName(void) const {return m_prop_name;};
	char * GetLabel(void) {return m_labels;}
	void PrintSat(CKripke * kripke);
	void DebugPrintLabel(CKripke * kripke);

protected:
	std::wstring m_prop_name;
	char * m_labels;
	size_t m_length;
};

void CreatePropLabel(CPropLabel *& prop, size_t len);


///////////////////////////////////////////////////////////////////////////////
// -- Kripke
class CKripke : public IJCInterface
{
public:
	CKripke(void);
	~CKripke(void);

	enum STR_TYPE {STATUS, PROPSITION, TRANSITION, LABEL};
public:
	size_t GetStatusNum(void) {return m_status_num;}
	// initialize Kripe with status number and proposition number
	//void Initialize(size_t state_num, size_t prop_num);
	void Initialize(void);
	// Add a status or proposition to Kripe
	void AddStatus(const std::wstring & ss/*, STR_TYPE type*/);
	// Add a connection from s1 to s2
	void Connect(size_t s1, size_t s2);
	char GetTransition(size_t s1, size_t s2);

	void ReadStringsForRelation(std::wstring & str, size_t status, CKripke::STR_TYPE type);
	void ReadTransition(boost::property_tree::wptree & pt);
	void ReadLabels(boost::property_tree::wptree & pt);

	// propositon operations
	void AddProposition(const std::wstring & prop);
	bool GetAp(CPropLabel * & prop, const std::wstring & ap_name);

	STATUS_TYPE GetFirstRelation(STATUS_TYPE s0);
	bool GetNextRelation(STATUS_TYPE s0, STATUS_TYPE & r);

	const std::wstring & GetStatusName(STATUS_TYPE ss) const
	{
		JCASSERT(ss < m_status_num);
		return m_status_names[ss];
	}

	// for debug
	void PrintStatus(void);


protected:
	size_t m_status_num;		// Using int to indicate status. status from 0 to m_status_num-1;
	// 2D array (m_status_num * m_status_num) to indicate a transition relation
	//	if there is a connection from sa to sb then m_transition[sa][sb]=1
	size_t m_tran_size;
	char * m_transition;	

	typedef std::map<const std::wstring, size_t> STRING_MAP;
	typedef STRING_MAP::iterator STR_ITERATOR;

	STRING_MAP m_status_map;
	std::vector<const std::wstring> m_status_names;

	typedef std::map<std::wstring, CPropLabel *>::iterator PROP_MAP_ITERATOR;
	std::map<std::wstring, CPropLabel *> m_prop_map;
};

// -- CKripke
inline void ltrim(std::wstring &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
inline void rtrim(std::wstring &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::wstring &s) {
    ltrim(s);
    rtrim(s);
}

void CalculateNot(CPropLabel * & res, CPropLabel *p1, size_t len);
void CalculateAnd(CPropLabel * & res,CPropLabel * p1, CPropLabel *p2, size_t len);
void CalculateOr(CPropLabel * & res, CPropLabel *p1, CPropLabel *p2, size_t len);
void CalculateImply(CPropLabel * & res, CPropLabel *p1, CPropLabel *p2, size_t len);
