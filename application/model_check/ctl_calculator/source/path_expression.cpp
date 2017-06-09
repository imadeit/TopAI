#include "stdafx.h"

#include "path_expression.h"
namespace prop_tree = boost::property_tree;

///////////////////////////////////////////////////////////////////////////////
// -- Path Expressions

static char * MakeTrueLabel(CPropLabel * & res, size_t len)
{
	CreatePropLabel(res, len);			JCASSERT(res);
	char * __true_ = res->GetLabel();	JCASSERT(__true_);
	memset(__true_, 1, len);	
	return __true_;
}

// -- AG and EG
void CPathExpG::CalculateAExp(CPropLabel * & res, CKripke * kripke)
{	// AGp = !EF(!p) = !(E(true U !p))
	JCASSERT(res==NULL && kripke);	JCASSERT(m_op[0]);
	size_t len = kripke->GetStatusNum();
	// not p
	jcvos::auto_interface<CPropLabel> not_p;
	CalculateNot(not_p, m_op[0], len);
	// make true
	jcvos::auto_interface<CPropLabel> _true_;
	char * __true_=MakeTrueLabel(_true_, len);
	// E(true U !p)
	jcvos::auto_interface<CPropLabel> tmp;	// E(true U !p)
	CreatePropLabel(tmp, len);			JCASSERT(tmp);
	char * _tmp=tmp->GetLabel();		JCASSERT(_tmp);
	char * _not_p = not_p->GetLabel();	JCASSERT(_not_p);
	CPathExpU::SatEU(_tmp, __true_, _not_p, kripke);
	// !E(true U !p)
	CalculateNot(res, tmp, len);
	res->SetName(std::wstring(L"AG(")+m_op[0]->GetName()+L")");
}

void CPathExpG::CalculateEExp(CPropLabel * & res, CKripke * kripke)
{	// EGp = !AF(!p) = !(A(true U !p))
	JCASSERT(res==NULL && kripke);	JCASSERT(m_op[0]);
	size_t len = kripke->GetStatusNum();
	// not p
	jcvos::auto_interface<CPropLabel> not_p;
	CalculateNot(not_p, m_op[0], len);
	// make true
	jcvos::auto_interface<CPropLabel> _true_;
	char * __true_=MakeTrueLabel(_true_, len);
	// E(true U !p)
	jcvos::auto_interface<CPropLabel> tmp;	// A(true U !p)
	CreatePropLabel(tmp, len);			JCASSERT(tmp);
	char * _tmp=tmp->GetLabel();		JCASSERT(_tmp);
	char * _not_p = not_p->GetLabel();	JCASSERT(_not_p);
	CPathExpU::SatAU(_tmp, __true_, _not_p, kripke);
	// !A(true U !p)
	CalculateNot(res, tmp, len);
	res->SetName(std::wstring(L"EG(")+m_op[0]->GetName()+L")");
}

// -- AF and EF
void CPathExpF::CalculateAExp(CPropLabel * & res, CKripke * kripke)
{	// AFp = A(true U p)
	JCASSERT(res==NULL && kripke);	JCASSERT(m_op[0]);
	size_t len = kripke->GetStatusNum();
	// make true
	jcvos::auto_interface<CPropLabel> _true_;
	char * __true_=MakeTrueLabel(_true_, len);
	
	CreatePropLabel(res, len);				JCASSERT(res);

	char * _res = res->GetLabel();		JCASSERT(_res);
	char * _q = m_op[0]->GetLabel();	JCASSERT(_q);
	CPathExpU::SatAU(_res, __true_, _q, kripke);	
	res->SetName(std::wstring(L"AF(")+m_op[0]->GetName()+L")");
}

void CPathExpF::CalculateEExp(CPropLabel * & res, CKripke * kripke)
{	// EFp = E(true U p)
	JCASSERT(res==NULL && kripke);	JCASSERT(m_op[0]);

	size_t len = kripke->GetStatusNum();
	// make true
	jcvos::auto_interface<CPropLabel> _true_;
	char * __true_=MakeTrueLabel(_true_, len);

	CreatePropLabel(res, len);				JCASSERT(res);

	char * _res = res->GetLabel();		JCASSERT(_res);
	char * _q = m_op[0]->GetLabel();	JCASSERT(_q);
	CPathExpU::SatEU(_res, __true_, _q, kripke);	
	res->SetName(std::wstring(L"EF(")+m_op[0]->GetName()+L")");
}

//-----------------------------------------------------------------------------
// -- AX and EX
CPathExpX::CPathExpX(void)
{
}

CPathExpX::~CPathExpX(void)
{
}

void CPathExpX::CalculateAExp(CPropLabel * & res, CKripke * kripke)
{	// AXp = !EX(!p)
	JCASSERT(res==NULL && kripke);
	JCASSERT(m_op[0]);

	size_t status_num = kripke->GetStatusNum();
	jcvos::auto_interface<CPropLabel> not_p;
	CalculateNot(not_p, m_op[0], status_num);
	jcvos::auto_interface<CPropLabel> ex_not_p;
	SatEX(ex_not_p, not_p, kripke);
	CalculateNot(res, ex_not_p, status_num);
	res->SetName(std::wstring(L"AX(")+m_op[0]->GetName()+L")");
}

void CPathExpX::CalculateEExp(CPropLabel * & res, CKripke * kripke)
{	// EX
	JCASSERT(res==NULL && kripke);
	JCASSERT(m_op[0]);
	SatEX(res, m_op[0], kripke);
	res->SetName(std::wstring(L"EX(")+m_op[0]->GetName()+L")");
}

void CPathExpX::SatEX(CPropLabel * & res, CPropLabel * p, CKripke * kripke)
{
	JCASSERT(res==NULL && kripke);		JCASSERT(p);

	size_t status_num = kripke->GetStatusNum();
	CreatePropLabel(res, status_num);
	
	char * _res = res->GetLabel();		JCASSERT(_res);
	char * _p = p->GetLabel();		JCASSERT(_p);

	// initialize result,
	memset(_res, 0, status_num);
	// search status connect to result and p.
	for (STATUS_TYPE s1=0; s1<status_num; ++s1)
	{
		STATUS_TYPE s2=kripke->GetFirstRelation(s1);
		while (s2 != 0xFFFFFFFF)
		{	// s1 -> s2, p[s2]
			if (_p[s2])
			{
				_res[s1]=1;
				break;
			}
			kripke->GetNextRelation(s1, s2);
		}
	}
}

// ----------------------------------------------------------------------------
// -- AU and EU --
CPathExpU::CPathExpU(void)
{
}

CPathExpU::~CPathExpU(void)
{
}

// AU
void CPathExpU::CalculateAExp(CPropLabel * & res, CKripke * kripke)
{
	JCASSERT(res==NULL && kripke);		JCASSERT(m_op[0] && m_op[1]);
	CreatePropLabel(res, kripke->GetStatusNum());	JCASSERT(res);
	
	char * _res = res->GetLabel();		JCASSERT(_res);
	char * _p = m_op[0]->GetLabel();	JCASSERT(_p);
	char * _q = m_op[1]->GetLabel();	JCASSERT(_q);
	SatAU(_res, _p, _q, kripke);
	res->SetName(std::wstring(L"A(")+m_op[0]->GetName()+L" U "+m_op[1]->GetName()+L")");
}

// EU
void CPathExpU::CalculateEExp(CPropLabel * & res, CKripke * kripke)
{
	JCASSERT(res==NULL && kripke);		JCASSERT(m_op[0] && m_op[1]);
	CreatePropLabel(res, kripke->GetStatusNum());	JCASSERT(res);
	
	char * _res = res->GetLabel();		JCASSERT(_res);
	char * _p = m_op[0]->GetLabel();	JCASSERT(_p);
	char * _q = m_op[1]->GetLabel();	JCASSERT(_q);
	SatEU(_res, _p, _q, kripke);
	res->SetName(std::wstring(L"E(")+m_op[0]->GetName()+L" U "+m_op[1]->GetName()+L")");
}

void CPathExpU::SatEU(char * _res, char * _p, char * _q, CKripke *kripke)
{
	JCASSERT(_res && _p && _q);
	size_t status_num = kripke->GetStatusNum();
	// mark all q to result,
	memcpy_s(_res, status_num, _q, status_num);
	// repeat
	while (1)
	{
		size_t updated=0;
		// search status connect to result and p.
		for (STATUS_TYPE s1=0; s1<status_num; ++s1)
		{
			if (_res[s1] || (!_p[s1]) ) continue;

			STATUS_TYPE s2=kripke->GetFirstRelation(s1);
			while (s2 != 0xFFFFFFFF)
			{
				if (_res[s2])
				{
					_res[s1]=1;
					updated++;
					break;
				}
				bool br=kripke->GetNextRelation(s1, s2);
			}
		}
		if (updated==0) break;
	}
}

void CPathExpU::SatAU(char * _res, char * _p, char * _q, CKripke *kripke)
{
	JCASSERT(_res && _p && _q)
	size_t status_num = kripke->GetStatusNum();
	memcpy_s(_res, status_num, _q, status_num);
	// repeat
	while (1)
	{
		size_t updated=0;
		// search status connect to result and p.
		for (STATUS_TYPE s1=0; s1<status_num; ++s1)
		{
			if (_res[s1] || (!_p[s1]) ) continue;

			STATUS_TYPE s2=kripke->GetFirstRelation(s1);
			while (s2 != 0xFFFFFFFF)
			{	// exist not connect s2->s1 (no status has not connection)
				if (!_res[s2]) break;
				kripke->GetNextRelation(s1, s2);
			}
			if (s2==0xFFFFFFFF) 
			{
				_res[s1]=1;
				updated++;
			}
		}
		if (updated==0) break;
	}
}

//-----------------------------------------------------------------------------
// -- AW and EW

// AW
void CPathExpW::CalculateAExp(CPropLabel * & res, CKripke * kripke)
{	// A(pWq) = A(pUq) or AG(p)
	THROW_ERROR(ERR_APP, L"AW is not supported");
	//JCASSERT(res==NULL && kripke);		JCASSERT(m_op[0] && m_op[1]);
	//CreatePropLabel(res, kripke->GetStatusNum());	JCASSERT(res);
	//
	//char * _res = res->GetLabel();		JCASSERT(_res);
	//char * _p = m_op[0]->GetLabel();	JCASSERT(_p);
	//char * _q = m_op[1]->GetLabel();	JCASSERT(_q);
	//SatAU(_res, _p, _q, kripke);
	//res->SetName(std::wstring(L"A(")+m_op[0]->GetName()+L" U "+m_op[1]->GetName()+L")");
}

// EW
void CPathExpW::CalculateEExp(CPropLabel * & res, CKripke * kripke)
{
	THROW_ERROR(ERR_APP, L"EW is not supported");
	//JCASSERT(res==NULL && kripke);		JCASSERT(m_op[0] && m_op[1]);
	//CreatePropLabel(res, kripke->GetStatusNum());	JCASSERT(res);
	//
	//char * _res = res->GetLabel();		JCASSERT(_res);
	//char * _p = m_op[0]->GetLabel();	JCASSERT(_p);
	//char * _q = m_op[1]->GetLabel();	JCASSERT(_q);
	//SatEU(_res, _p, _q, kripke);
	//res->SetName(std::wstring(L"E(")+m_op[0]->GetName()+L" U "+m_op[1]->GetName()+L")");
}

//-----------------------------------------------------------------------------
// -- AG and EG

// AR
void CPathExpR::CalculateAExp(CPropLabel * & res, CKripke * kripke)
{	// 
	THROW_ERROR(ERR_APP, L"AR is not supported");
}

// ER
void CPathExpR::CalculateEExp(CPropLabel * & res, CKripke * kripke)
{
	THROW_ERROR(ERR_APP, L"ER is not supported");
}

//-----------------------------------------------------------------------------
// -- Logical Expressions
void CalculateNot(CPropLabel * & res, CPropLabel *p1, size_t len)
{
	JCASSERT(res == NULL && p1);
	CreatePropLabel(res, len);		JCASSERT(res);
	//res = new jcvos::CDynamicInstance<CPropLabel>;
	//res->CreateLabel(len);
	char * r=res->GetLabel();
	char * p=p1->GetLabel();
	for (size_t ii=0; ii<len; ++ii)	r[ii]=!p[ii];
}

void CalculateAnd(CPropLabel * & res,CPropLabel * p1, CPropLabel *p2, size_t len)
{
	JCASSERT(res == NULL && p1 && p2);
	CreatePropLabel(res, len);		JCASSERT(res);
	//res = new jcvos::CDynamicInstance<CPropLabel>;
	//res->CreateLabel(len);
	char * _r=res->GetLabel();
	char * _p1=p1->GetLabel();
	char * _p2=p2->GetLabel();
	for (size_t ii=0; ii<len; ++ii)	_r[ii]=(_p1[ii] && _p2[ii]);
}

void CalculateOr(CPropLabel * & res, CPropLabel *p1, CPropLabel *p2, size_t len)
{
	JCASSERT(res == NULL && p1 && p2);
	CreatePropLabel(res, len);		JCASSERT(res);
	//res = new jcvos::CDynamicInstance<CPropLabel>;
	//res->CreateLabel(len);
	char * _r=res->GetLabel();
	char * _p1=p1->GetLabel();
	char * _p2=p2->GetLabel();
	for (size_t ii=0; ii<len; ++ii)	_r[ii]=(_p1[ii] || _p2[ii]);
}

void CalculateImply(CPropLabel * & res, CPropLabel *p1, CPropLabel *p2, size_t len)
{
	JCASSERT(res == NULL && p1 && p2);
	CreatePropLabel(res, len);		JCASSERT(res);
	//res = new jcvos::CDynamicInstance<CPropLabel>;
	//res->CreateLabel(len);
	char * _r=res->GetLabel();
	char * _p1=p1->GetLabel();
	char * _p2=p2->GetLabel();
	for (size_t ii=0; ii<len; ++ii)	_r[ii]=((!_p1[ii]) || _p2[ii]);
}