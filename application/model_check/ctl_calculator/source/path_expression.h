#pragma once

#include <stdext.h>
#include "kripke.h"

///////////////////////////////////////////////////////////////////////////////
// -- Path Expressions
class IPathExpression : public IJCInterface
{
public:
	virtual void SetOperator(int id, CPropLabel *)=0;
	virtual void CalculateAExp(CPropLabel * &, CKripke * k)=0;
	virtual void CalculateEExp(CPropLabel * &, CKripke * k)=0;
};

template <size_t op_num>
class CPathExpBase : public IPathExpression
{
public:
	CPathExpBase(void) {
		for (size_t ii=0; ii<op_num; ++ii) m_op[ii]=NULL;
	}

	virtual ~CPathExpBase(void) {
		for (size_t ii=0; ii<op_num; ++ii) RELEASE(m_op[ii]);
	}

protected:
	virtual void SetOperator(int id, CPropLabel * op)
	{
		JCASSERT(id<op_num && op);
		m_op[id] = op;
		op->AddRef();
	}

protected:
	CPropLabel * m_op[op_num];
};

class CPathExpG : public CPathExpBase<1>
{
public:
	CPathExpG(void) {};
	virtual ~CPathExpG(void) {};

public:
	virtual void CalculateAExp(CPropLabel * &, CKripke * k);
	virtual void CalculateEExp(CPropLabel * &, CKripke * k);
};

class CPathExpF : public CPathExpBase<1>
{
public:
	CPathExpF(void) {};
	virtual ~CPathExpF(void) {};

public:
	virtual void CalculateAExp(CPropLabel * &, CKripke * k);
	virtual void CalculateEExp(CPropLabel * &, CKripke * k);
};

class CPathExpX : public CPathExpBase<1>
{
public:
	CPathExpX(void);
	virtual ~CPathExpX(void);
public:
	virtual void CalculateAExp(CPropLabel * &, CKripke * k);
	virtual void CalculateEExp(CPropLabel * &, CKripke * k);

protected:
	static void SatEX(CPropLabel * & res, CPropLabel * p, CKripke * kripke);
};

class CPathExpU : public CPathExpBase<2>
{
public:
	CPathExpU(void) ;
	virtual ~CPathExpU(void) ;
public:
	virtual void CalculateAExp(CPropLabel * &, CKripke * k);
	virtual void CalculateEExp(CPropLabel * &, CKripke * k);

public:
	static void SatEU(char * res, char * p, char * q, CKripke *k);
	static void SatAU(char * res, char * p, char * q, CKripke *k);
};

class CPathExpW : public CPathExpBase<2>
{
public:
	CPathExpW(void) {} ;
	virtual ~CPathExpW(void) {};
public:
	virtual void CalculateAExp(CPropLabel * &, CKripke * k);
	virtual void CalculateEExp(CPropLabel * &, CKripke * k);
};

class CPathExpR : public CPathExpBase<2>
{
public:
	CPathExpR(void) {};
	virtual ~CPathExpR(void) {};
public:
	virtual void CalculateAExp(CPropLabel * &, CKripke * k);
	virtual void CalculateEExp(CPropLabel * &, CKripke * k);
};
