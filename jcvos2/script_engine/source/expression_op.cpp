﻿#include "stdafx.h"
#include "syntax_parser.h"

#pragma warning (disable: 4800)

#include "expression_op.h"

// LOG 输出定义：
//  WARNING:	运行错误
//	NOTICE:		script运行trace
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

LOG_CLASS_SIZE(CONVERT_FUN)
LOG_CLASS_SIZE(SOURCE_FUN)
LOG_CLASS_SIZE(ATHOP_FUN)
LOG_CLASS_SIZE(RELOP_FUN)

///////////////////////////////////////////////////////////////////////////////
void ToIValue(jcvos::VALUE_TYPE type, void * data, jcvos::IValue * & val)
{
	JCASSERT(NULL == val);
	switch (type)
	{
	case jcvos::VT_BOOL:	val = jcvos::CTypedValue<bool>::Create(*((bool*)data));		break;
	case jcvos::VT_CHAR:	val = jcvos::CTypedValue<char>::Create(*((char*)data));		break;
	case jcvos::VT_UCHAR:	val = jcvos::CTypedValue<BYTE>::Create(*((BYTE*)data));		break;
	case jcvos::VT_SHORT:	val = jcvos::CTypedValue<short>::Create(*((short*)data));		break;
	case jcvos::VT_USHORT:	val = jcvos::CTypedValue<WORD>::Create(*((WORD*)data));	break;
	case jcvos::VT_INT:	val = jcvos::CTypedValue<int>::Create(*((int*)data));			break;
	case jcvos::VT_UINT:	val = jcvos::CTypedValue<DWORD>::Create(*((DWORD*)data));		break;
	case jcvos::VT_INT64:	val = jcvos::CTypedValue<INT64>::Create(*((INT64*)data));		break;
	case jcvos::VT_UINT64:	val = jcvos::CTypedValue<UINT64>::Create(*((UINT64*)data));	break;
	case jcvos::VT_FLOAT:	val = jcvos::CTypedValue<float>::Create(*((float*)data));		break;
	case jcvos::VT_DOUBLE:	val = jcvos::CTypedValue<double>::Create(*((double*)data));	break;
	//case VT_STRING:	val = jcvos::CTypedValue<BYTE>::Create(*data);	break;
	}
	return;
}

template <typename T>
bool FromIValue(jcvos::IValue * val, void * data)
{
	using namespace jcvos;
	CTypedValue<T> * _val = dynamic_cast<CTypedValue<T>*>(val);
	if (! _val ) return false;
	*((T*)data) = (*_val);
	return true;
}

static const FROM_IVALUE FROM_IVALUE_FUN_TAB[jcvos::VT_MAXNUM] = {
	NULL, // unknown
	FromIValue<bool>,	FromIValue<char>,	FromIValue<BYTE>,	FromIValue<short>,	
	FromIValue<WORD>,	FromIValue<int>,	FromIValue<UINT>,	FromIValue<INT64>,	
	FromIValue<UINT64>,	FromIValue<float>,	FromIValue<double>,	NULL};


///////////////////////////////////////////////////////////////////////////////
//-- CAthOpBase


static const CONVERT_FUN CONV_FUN_TAB[jcvos::VT_MAXNUM-1][jcvos::VT_MAXNUM-1] = 
{
	// bool
	{	Convert<bool, bool>,	Convert<bool, char>,	Convert<bool, BYTE>,	Convert<bool, short>,	
		Convert<bool, WORD>,	Convert<bool, int>,		Convert<bool, DWORD>,	Convert<bool, INT64>,	
		Convert<bool, UINT64>,	Convert<bool, float>,	Convert<bool, double>,	NULL},
	// char
	{	Convert<char, bool>,	Convert<char, char>,	Convert<char, BYTE>,	Convert<char, short>,	
		Convert<char, WORD>,	Convert<char, int>,		Convert<char, DWORD>,	Convert<char, INT64>,	
		Convert<char, UINT64>,	Convert<char, float>,	Convert<char, double>, NULL},
	// BYTE
	{	Convert<BYTE, bool>,	Convert<BYTE, char>,	Convert<BYTE, BYTE>,	Convert<BYTE, short>,	
		Convert<BYTE, WORD>,	Convert<BYTE, int>,		Convert<BYTE, DWORD>,	Convert<BYTE, INT64>,	
		Convert<BYTE, UINT64>,	Convert<BYTE, float>,	Convert<BYTE, double>,	NULL},
	// short
	{	Convert<short, bool>,	Convert<short, char>,	Convert<short, BYTE>,	Convert<short, short>,	
		Convert<short, WORD>,	Convert<short, int>,	Convert<short, DWORD>,	Convert<short, INT64>,	
		Convert<short, UINT64>,	Convert<short, float>,	Convert<short, double>, NULL},
	// WORD
	{	Convert<WORD, bool>,	Convert<WORD, char>,	Convert<WORD, BYTE>,	Convert<WORD, short>,	
		Convert<WORD, WORD>,	Convert<WORD, int>,		Convert<WORD, DWORD>,	Convert<WORD, INT64>,	
		Convert<WORD, UINT64>,	Convert<WORD, float>,	Convert<WORD, double>,	NULL},
	// int
	{	Convert<int, bool>,		Convert<int, char>,		Convert<int, BYTE>,		Convert<int, short>,	
		Convert<int, WORD>,		Convert<int, int>,		Convert<int, DWORD>,	Convert<int, INT64>,	
		Convert<int, UINT64>,	Convert<int, float>,	Convert<int, double>,	NULL},
	// DWORD
	{	Convert<DWORD, bool>,	Convert<DWORD, char>,	Convert<DWORD, BYTE>,	Convert<DWORD, short>,	
		Convert<DWORD, WORD>,	Convert<DWORD, int>,	Convert<DWORD, DWORD>,	Convert<DWORD, INT64>,	
		Convert<DWORD, UINT64>,	Convert<DWORD, float>,	Convert<DWORD, double>, NULL},
	// INT64
	{	Convert<INT64, bool>,	Convert<INT64, char>,	Convert<INT64, BYTE>,	Convert<INT64, short>,	
		Convert<INT64, WORD>,	Convert<INT64, int>,	Convert<INT64, DWORD>,	Convert<INT64, INT64>,	
		Convert<INT64, UINT64>,	Convert<INT64, float>,	Convert<INT64, double>, NULL},
	// UINT64
	{	Convert<UINT64, bool>,	Convert<UINT64, char>,	Convert<UINT64, BYTE>,	Convert<UINT64, short>,	
		Convert<UINT64, WORD>,	Convert<UINT64, int>,	Convert<UINT64, DWORD>,	Convert<UINT64, INT64>,	
		Convert<UINT64, UINT64>,Convert<UINT64, float>,	Convert<UINT64, double>,NULL},
	// float
	{	Convert<float, bool>,	Convert<float, char>,	Convert<float, BYTE>,	Convert<float, short>,	
		Convert<float, WORD>,	Convert<float, int>,	Convert<float, DWORD>,	Convert<float, INT64>,	
		Convert<float, UINT64>,	Convert<float, float>,	Convert<float, double>, NULL},
	// double
	{	Convert<double, bool>,	Convert<double, char>,	Convert<double, BYTE>,	Convert<double, short>,	
		Convert<double, WORD>,	Convert<double, int>,	Convert<double, DWORD>,	Convert<double, INT64>,	
		Convert<double, UINT64>,Convert<double, float>, Convert<double, double>, NULL},
	// string
	{	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL, NULL, NULL, NULL, 
	}
	// string
	//{	Convert<bool, bool>,				Convert<bool, char>,	Convert<bool, BYTE>,	Convert<bool, short>,	Convert<bool, WORD>, 
	//	Convert<bool, int>, Convert<bool, DWORD>,	Convert<bool, INT64>,	Convert<bool, UINT64>,	Convert<bool, float>, 
	//	Convert<bool, double>, NULL},
};

const TCHAR no_name::m_name[] = _T("no name");

CONVERT_FUN GetConvFunc(jcvos::VALUE_TYPE t1, jcvos::VALUE_TYPE t2)
{
	JCASSERT( t1 < jcvos::VT_MAXNUM -1 );
	JCASSERT( t2 < jcvos::VT_MAXNUM -1 );

	return CONV_FUN_TAB[t1-1][t2-1];
}

template <class ATH_OP>
CAthOpBase<ATH_OP>::CAthOpBase(void)
	: m_conv_l(NULL)
	, m_conv_r(NULL)
	, m_op(NULL)
{
}

template <class ATH_OP>
CAthOpBase<ATH_OP>::~CAthOpBase(void)
{
}

template <class ATH_OP>
bool CAthOpBase<ATH_OP>::GetResult(jcvos::IValue * & val)
{
	ToIValue(m_res_type, m_res, val);
	return true;
}

template <class ATH_OP>
bool CAthOpBase<ATH_OP>::Invoke(void)
{
	BYTE sl[sizeof(double)];
	BYTE sr[sizeof(double)];

	if ( ! m_op )
	{	// 初始化，设定转换与比较函数

		// 左值的类型，右值的类型
		jcvos::VALUE_TYPE tl, tr;
		// 取得L,R的类型
		tl = m_src[0]->GetValueType();
		tr = m_src[1]->GetValueType();
		// 类型推导
		m_res_type = max(tl, tr);
		// 设置类型转换
		m_conv_l = GetConvFunc(tl, m_res_type);
		m_conv_r = GetConvFunc(tr, m_res_type);

		// 
		m_op = AthOp[m_res_type-1];
		JCASSERT(m_op);

		LOG_NOTICE(_T("%s init: type_l=%d, type_r=%d, type_res=%d"), ATH_OP::m_name, tl, tr, m_res_type);
	}
	m_conv_l(m_src[0]->GetValue(), sl);
	m_conv_r(m_src[1]->GetValue(), sr);
	m_op(sl, sr, m_res);

	LOG_NOTICE(_T("%d %s %d = %d"), *((int*)sl), ATH_OP::m_name, *((int*)(sr)), *((int*)m_res) );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//-- relation operator

template <class ATH_OP>
const ATHOP_FUN CAthOpBase<ATH_OP>::AthOp[] = {
//	0					1					2					3					4
	ATH_OP::Op<bool>,	ATH_OP::Op<char>,	ATH_OP::Op<BYTE>,	ATH_OP::Op<short>,	ATH_OP::Op<WORD>,
	ATH_OP::Op<int>,	ATH_OP::Op<DWORD>,	ATH_OP::Op<INT64>,	ATH_OP::Op<UINT64>, ATH_OP::Op<float>, 
	ATH_OP::Op<double>, NULL, 
};

const TCHAR CAthAdd::m_name[] = _T("+");
template class CAthOpBase<CAthAdd>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthAdd)


const TCHAR CAthSub::m_name[] = _T("-");
template class CAthOpBase<CAthSub>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthSub)

const TCHAR CAthMul::m_name[] = _T("*");
template class CAthOpBase<CAthMul>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthMul)

const TCHAR CAthDiv::m_name[] = _T("/");
template class CAthOpBase<CAthDiv>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthDiv)
template <> void CAthDiv::Op<bool>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}

const TCHAR CAthAnd::m_name[] = _T("&");
template class CAthOpBase<CAthAnd>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthAnd)
template <> void CAthAnd::Op<float>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}
template <> void CAthAnd::Op<double>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}


const TCHAR CAthOr::m_name[] = _T("|");
template class CAthOpBase<CAthOr>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthOr)
template <> void CAthOr::Op<float>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}
template <> void CAthOr::Op<double>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}

const TCHAR CAthXor::m_name[] = _T("^");
template class CAthOpBase<CAthXor>;
LOG_CLASS_SIZE_T1(CAthOpBase, CAthXor)
template <> void CAthXor::Op<float>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}
template <> void CAthXor::Op<double>(const void * l, const void* r, void * res)	{NOT_SUPPORT0;}

///////////////////////////////////////////////////////////////////////////////
//-- Column Value
LOG_CLASS_SIZE(CColumnVal);
const TCHAR CColumnVal::m_name[] = _T("column_val");

CColumnVal::CColumnVal(const CJCStringT & col_name)
	: m_col_name(col_name)
	, m_res_type(jcvos::VT_UNKNOW)
	, m_convertor(NULL)
{
}

CColumnVal::~CColumnVal(void)
{
}

bool CColumnVal::GetResult(jcvos::IValue * & val)
{
	ToIValue(m_res_type, m_res, val);
	return true;
}

bool CColumnVal::Invoke(void)
{
	JCASSERT(m_src[0]);

	jcvos::auto_interface<jcvos::IValue> val;
	m_src[0]->GetResult(val);
	jcvos::ITableRow * row = NULL;
	row = val.d_cast<jcvos::ITableRow*>();

	if (!row) THROW_ERROR(ERR_USER, _T("input is not a row"));

	if (jcvos::VT_UNKNOW == m_res_type)
	{
		const jcvos::CFieldDefinition * info = row->GetColumnInfo(m_col_name.c_str());
		if (!info) THROW_ERROR(ERR_USER, _T("Unknow column %s"), m_col_name.c_str() );
		m_res_type = info->m_type;
		m_col_id = info->m_id;
		// 选择转换函数
		LOG_SCRIPT(_T(": init col=%d, type=%d"), m_col_id, m_res_type);
		m_convertor = FROM_IVALUE_FUN_TAB[m_res_type];
		if (! m_convertor) THROW_ERROR(ERR_UNSUPPORT, _T("unsupport type %d"), m_res_type);
	}

	jcvos::auto_interface<jcvos::IValue> col_val;
	row->GetColumnData(m_col_id, col_val);
	bool br = m_convertor(col_val, m_res);
	if (!br) THROW_ERROR(ERR_PARAMETER, _T("wrong type convert"));

	LOG_SCRIPT(_T(": val=%X"), *((UINT*)(m_res)) );
	return true;
}

void CColumnVal::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T("col=%s"), m_col_name.c_str() );
}

///////////////////////////////////////////////////////////////////////////////
//-- constant


///////////////////////////////////////////////////////////////////////////////
//-- bool operator

template <class BOOL_OP>
CBoolOpBase<BOOL_OP>::CBoolOpBase(void)
{
}

template <class BOOL_OP>
bool CBoolOpBase<BOOL_OP>::GetResult(jcvos::IValue * & val)
{
	if (NULL == val) val = static_cast<jcvos::IValue*>(jcvos::CTypedValue<bool>::Create(m_res));
	return m_res;
}

template <class BOOL_OP>
bool CBoolOpBase<BOOL_OP>::Invoke(void)
{
	bool *_r = (bool*)(m_src[0]->GetValue());
	bool *_l = (bool*)(m_src[1]->GetValue());
	m_res = BOOL_OP::op(* _r, * _l);
	LOG_NOTICE(_T("%d %s %d = %d"), *_r, BOOL_OP::m_name, *_l, m_res );
	return true;
}

const TCHAR CBoolAnd::m_name[] = _T("and");
const TCHAR CBoolOr::m_name[] = _T("or");
const TCHAR CBoolOpNot::m_name[] = _T("not");

template class CBoolOpBase<CBoolAnd>;
LOG_CLASS_SIZE_T1(CBoolOpBase, CBoolAnd);

template class CBoolOpBase<CBoolOr>;
LOG_CLASS_SIZE_T1(CBoolOpBase, CBoolOr);

///////////////////////////////////////////////////////////////////////////////
//-- Constant
LOG_CLASS_SIZE_T1(CConstantOp, int)
LOG_CLASS_SIZE_T1(CConstantOp, INT64)


///////////////////////////////////////////////////////////////////////////////
//-- relation operator
template <class REL_OP>
CRelOpBase<REL_OP>::CRelOpBase(void)
	: m_op(NULL), m_conv_l(NULL), m_conv_r(NULL)
{
}

template <class REL_OP>
bool CRelOpBase<REL_OP>::GetResult(jcvos::IValue * & val)
{
	if (NULL == val)	val = static_cast<jcvos::IValue*>(jcvos::CTypedValue<bool>::Create(m_res));
	return m_res;
}

template <class REL_OP>
bool CRelOpBase<REL_OP>::Invoke(void)
{
	BYTE sl[sizeof(double)];
	BYTE sr[sizeof(double)];
	if ( ! m_op )
	{	// 初始化，设定转换与比较函数

		// 左值的类型，右值的类型
		jcvos::VALUE_TYPE tl, tr;
		// 取得L, R的类型
		tl = m_src[0]->GetValueType();
		tr = m_src[1]->GetValueType();
		// 类型推导
		m_res_type = max(tl, tr);
		JCASSERT(m_res_type > 0);
		// 设置类型转换
		m_conv_l = GetConvFunc(tl, m_res_type);
		m_conv_r = GetConvFunc(tr, m_res_type);

		m_op = REL_OP::RelOp[m_res_type-1];
		JCASSERT(m_op);
	}
	m_conv_l(m_src[0]->GetValue(), sl);
	m_conv_r(m_src[1]->GetValue(), sr);
	m_res = m_op(sl, sr);
	LOG_NOTICE(_T("%d %s %d = %d"), *((int*)sl), REL_OP::m_name, *((int*)(sr)), m_res );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//-- relation operator

const RELOP_FUN CRopEQ::RelOp[] = {
//	0				1				2				3				4
	CmpEQ<bool>,	CmpEQ<char>,	CmpEQ<BYTE>,	CmpEQ<short>,	CmpEQ<WORD>,
	CmpEQ<int>,		CmpEQ<DWORD>,	CmpEQ<INT64>,	CmpEQ<UINT64>,	CmpEQ<float>, 
	CmpEQ<double>,	NULL, 
};
const TCHAR CRopEQ::m_name[] = _T("==");

template class CRelOpBase<CRopEQ>;


const RELOP_FUN CRopLT::RelOp[] = {
//	0				1				2				3				4
	CmpLT<bool>,	CmpLT<char>,	CmpLT<BYTE>,	CmpLT<short>,	CmpLT<WORD>,
	CmpLT<int>,		CmpLT<DWORD>,	CmpLT<INT64>,	CmpLT<UINT64>,	CmpLT<float>, 
	CmpLT<double>,	NULL, 
};
const TCHAR CRopLT::m_name[] = _T("<");

template class CRelOpBase<CRopLT>;


const RELOP_FUN CRopLE::RelOp[] = {
	CmpLE<bool>,	CmpLE<char>,	CmpLE<BYTE>,	CmpLE<short>,	CmpLE<WORD>,
	CmpLE<int>,		CmpLE<DWORD>,	CmpLE<INT64>,	CmpLE<UINT64>,	CmpLE<float>, 
	CmpLE<double>,	NULL, 
};
const TCHAR CRopLE::m_name[] = _T("<=");

template class CRelOpBase<CRopLE>;

LOG_CLASS_SIZE(CRelOpEQ);
LOG_CLASS_SIZE(CRelOpLT);
LOG_CLASS_SIZE(CRelOpLE);



///////////////////////////////////////////////////////////////////////////////
//-- CVariableOp
LOG_CLASS_SIZE(CVariableOp)

const TCHAR CVariableOp::m_name[] = _T("variable");

CVariableOp::CVariableOp(const CJCStringT & var_name)
	: m_val(NULL)
	, m_var_name(var_name)
{
	LOG_STACK_TRACE();
}

CVariableOp::CVariableOp(jcvos::IValue * val)
	: m_val(val)
{
	if (m_val) m_val->AddRef();
}

CVariableOp::~CVariableOp(void)
{
	LOG_STACK_TRACE();
	if (m_val) m_val->Release();
}

bool CVariableOp::GetResult(jcvos::IValue * & val)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);

	val = m_val;
	if (val) val->AddRef();
	return true;
}

bool CVariableOp::Invoke(void)
{
	LOG_STACK_TRACE();

	JCASSERT(m_src[0]);
	JCASSERT(NULL == m_val);
	jcvos::IValue * val = NULL;
	m_src[0]->GetResult(val);		// 如果m_parent被设，m_src[0]一定等于m_parent。
	val->GetSubValue(m_var_name.c_str(), m_val);
	val->Release();
	if (NULL == m_val)	THROW_ERROR(ERR_USER, _T("variable $%s is not exist"), m_var_name.c_str() )
	m_typed_val = dynamic_cast<jcvos::CTypedValueBase*>(m_val);
#ifdef _DEBUG
	CJCStringT _str;
	//if ( NULL != dynamic_cast<jcvos::IValueConvertor*>(val) )
	{
		jcvos::GetVal(m_val, _str);
		LOG_SCRIPT(_T("%s=%s"), m_var_name.c_str(), _str.c_str());
	}
#else
	LOG_SCRIPT(_T("%s"), m_var_name.c_str() );
#endif	
	return true;
}

const void * CVariableOp::GetValue(void)
{
	JCASSERT(m_val);
	if (!m_typed_val)	
		THROW_ERROR(ERR_USER, _T("variable %s is not a simple type"), m_var_name.c_str());
	return m_typed_val->GetData();
}

jcvos::VALUE_TYPE CVariableOp::GetValueType(void)
{
	JCASSERT(m_val);

	if (!m_typed_val)	
		THROW_ERROR(ERR_USER, _T("variable %s is not a simple type"), m_var_name.c_str());

	return m_typed_val->GetType();

}

void CVariableOp::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T("sub=%s"), m_var_name.c_str());
}


///////////////////////////////////////////////////////////////////////////////
// -- pre define variables
const TCHAR CPdvIndex::m_name[] = _T("%INDEX");
