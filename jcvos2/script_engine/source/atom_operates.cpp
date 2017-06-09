﻿#include "stdafx.h"
#include "syntax_parser.h"
#include "atom_operates.h"

// LOG 输出定义：
//  WARNING:	运行错误
//	NOTICE:		script运行trace
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CAssignOp
LOG_CLASS_SIZE(CAssignOp)

const TCHAR CAssignOp::m_name[] = _T("assign");

CAssignOp::CAssignOp(IAtomOperate * dst_op, const CJCStringT & dst_name)
	: m_dst_op(dst_op)
	, m_dst_name(dst_name)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dst_op);
	m_dst_op->AddRef();
}

CAssignOp::~CAssignOp(void)
{
	LOG_STACK_TRACE();
	if (m_dst_op) m_dst_op->Release();
}

bool CAssignOp::GetResult(jcvos::IValue * & val)
{
	// 可能作为combo/script的result op使用
	return false;
}

bool CAssignOp::Invoke(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src[0]);
	JCASSERT(m_dst_op);

	jcvos::IValue *val = NULL;
	m_src[0]->GetResult(val);

#ifdef _DEBUG
	CJCStringT _str;
	if (val)
	{
		jcvos::GetVal(val, _str);
		LOG_SCRIPT(_T("val=%s => %s"), _str.c_str(), m_dst_name.c_str());
	}
#else
	LOG_SCRIPT(_T("=>%s"),  m_dst_name.c_str());
#endif


	if (val)
	{
		// 赋值的实现1: m_dst_op基于IAtomOperate的界面
		jcvos::IValue * val_dst = NULL;
		m_dst_op->GetResult(val_dst);
		val_dst->SetSubValue(m_dst_name.c_str(), val);
		val_dst->Release();
		val->Release();
	}

	return true;
}

void CAssignOp::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T("dst=<%08X>, var=%s"),
		(UINT)(m_dst_op), m_dst_name.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//-- CPushParamOp
LOG_CLASS_SIZE(CPushParamOp)
const TCHAR CPushParamOp::m_name[] = _T("push_param");

CPushParamOp::CPushParamOp(IFeature * dst_op, const CJCStringT & dst_name)
	: m_function(dst_op)
	, m_param_name(dst_name)
{
	LOG_STACK_TRACE();
	JCASSERT(m_function);
	m_function->AddRef();
}

CPushParamOp::~CPushParamOp(void)
{
	LOG_STACK_TRACE();
	if (m_function) m_function->Release();
}

bool CPushParamOp::GetResult(jcvos::IValue * & val)
{
	// 应当不会被调用
	JCASSERT(0);
	return false;
}

bool CPushParamOp::Invoke(void)
{
	JCASSERT(m_src[0]);
	JCASSERT(m_function);

	LOG_SCRIPT(_T("<%08X> -> <%08X>:%s"), 
		(UINT)(m_src[0]), (UINT)(m_function), m_param_name.c_str() );

	jcvos::IValue *val = NULL;
	m_src[0]->GetResult(val);

	m_function->PushParameter(m_param_name, val);
	val->Release();
	return true;
}

void CPushParamOp::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T("func=<%08X>, param=%s"),
		(UINT)(m_function), m_param_name.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//-- CSaveToFileOp
LOG_CLASS_SIZE(CSaveToFileOp)
const TCHAR CSaveToFileOp::m_name[] = _T("save_file");

CSaveToFileOp::CSaveToFileOp(const CJCStringT & filename)
	: m_file_name(filename)
	, m_file(NULL)
	, m_stream(NULL)
{
}
		
CSaveToFileOp::~CSaveToFileOp(void)
{
	if (m_file)		fclose(m_file);
	if (m_stream)	m_stream->Release();
}

bool CSaveToFileOp::CreateStream(jcvos::IValue * val, jcvos::IJCStream * & stream)
{
	jcvos::ITableRow * row = NULL;
	if (row = dynamic_cast<jcvos::ITableRow *>(val) )
	{
		CreateStreamFile(m_file_name.c_str(), jcvos::WRITE, stream);
		// output header if it is a row
		jcvos::ITable * tab = NULL;
		row->CreateTable(tab);
		tab->ToStream(m_stream, jcvos::VF_HEAD, 0);
		tab->Release();
	}
	else	// binary data
	{
		CreateStreamBinaryFile(m_file_name.c_str(), jcvos::WRITE, stream);
	}
	return true;
}

bool CSaveToFileOp::Invoke(void)
{
	LOG_SCRIPT(_T(""));
	// 支持逐次运行
	JCASSERT(m_src[0]);

	// 行写入
	jcvos::auto_interface<jcvos::IValue> val;
	m_src[0]->GetResult(val);
	if ( !val) return false; // !! warning

	// try for IValueFormat if
	jcvos::IVisibleValue * vval = NULL;

	vval = val.d_cast<jcvos::IVisibleValue*>();
	if (!vval)
	{
		LOG_ERROR(_T("value do not support IVisiblaValue"));
		return false;
	}

	if (NULL == m_stream)	CreateStream(val, m_stream);
	JCASSERT(m_stream);
	vval->ToStream(m_stream, jcvos::VF_DEFAULT, 0);
	m_stream->Put(_T('\n'));

	return false;
}

void CSaveToFileOp::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T("fn=%s"), m_file_name.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//-- CExitOp
LOG_CLASS_SIZE(CExitOp)

bool CExitOp::Invoke(void)
{
	_tprintf( _T("Bye bye!\r\n") );
	throw CExitException();
	return false;
}

const TCHAR CExitOp::name[] = _T("exit");

///////////////////////////////////////////////////////////////////////////////
//-- feature wrapper
LOG_CLASS_SIZE(CFeatureWrapper)
const TCHAR CFeatureWrapper::m_name[] = _T("feature");

CFeatureWrapper::CFeatureWrapper(IFeature * feature)
	: m_feature(feature)
	, m_outport(NULL)
{
	JCASSERT(m_feature);
	m_feature->AddRef();
}

CFeatureWrapper::~CFeatureWrapper(void)
{
	JCASSERT(m_feature);
	m_feature->Release();
}

bool CFeatureWrapper::GetResult(jcvos::IValue * & val)
{
	return false;
}

bool CFeatureWrapper::Invoke(void)
{
	jcvos::auto_interface<jcvos::IValue> val;
	if (m_src[0])	m_src[0]->GetResult(val);
	bool br = m_feature->Invoke(val, m_outport);
	LOG_SCRIPT(_T("%s: <%08X>: more=%d"), m_feature->GetFeatureName(), (UINT)(m_feature), br )
	return br;
}

void CFeatureWrapper::SetOutPort(IOutPort * outport)
{
	JCASSERT(NULL == m_outport);
	m_outport = outport;
}

void CFeatureWrapper::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T(": %s, <%08X>"), m_feature->GetFeatureName(), (UINT)(m_feature) );
}

///////////////////////////////////////////////////////////////////////////////
//-- filter statement
LOG_CLASS_SIZE(CFilterSt);
const TCHAR CFilterSt::m_name[] = _T("filter");

CFilterSt::CFilterSt(IOutPort * out_port)
	: m_init(false)
	, m_outport(out_port)
{
	// m_outport指向filter的上层，一般为single_st。不能引用计数，否则会引起release的死锁。
	JCASSERT(m_outport);
}
		
CFilterSt::~CFilterSt(void)
{
}

bool CFilterSt::Invoke(void)
{
	JCASSERT(m_outport);
	// src0: the source of bool expression.
	// src1: the source of table

	static jcvos::IValue * val = DUMMY_VALPTR;
	jcvos::auto_interface<jcvos::IValue> _row;
	bool br = m_src[SRC_EXP]->GetResult( val );
	if ( br )
	{
		m_src[SRC_TAB]->GetResult(_row);
		jcvos::IValue * row = _row;
		if ( !row ) THROW_ERROR(ERR_USER, _T("The input of filter should be a row"));
		m_outport->PushResult(row);
	}
	LOG_SCRIPT(_T(": %s"), br?_T("true"):_T("false"));
	// return false means no need more running for current input
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//-- CInPort
LOG_CLASS_SIZE(CInPort);

const TCHAR CInPort::m_name[] = _T("inport");

CInPort::CInPort(void)
	: m_row(NULL)
{
}

CInPort::~CInPort(void)
{
	if (m_row) m_row->Release();
}

bool CInPort::GetResult(jcvos::IValue * & val)
{
	JCASSERT(NULL == val);
	if (m_row)
	{
		val = static_cast<jcvos::IValue*>(m_row);
		val->AddRef();
		return true;
	}
	else	return false;
}

bool CInPort::Invoke(void)
{
	if (m_row)	m_row->Release(), m_row=NULL;
	bool br = m_src[0]->PopupResult(m_row);
	LOG_SCRIPT(_T("inport=%d"), br);
	return br;
}

