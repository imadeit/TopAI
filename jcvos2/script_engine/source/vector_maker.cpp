#include "stdafx.h"
#include "syntax_parser.h"
#include "atom_operates.h"

#include <math.h>

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);
using namespace jcscript;


LOG_CLASS_SIZE(COpVectorMaker);
const TCHAR COpVectorMaker::m_name[] = _T("mkvector");

COpVectorMaker::COpVectorMaker(void)
	: m_outport(NULL)
	, m_current(0), m_end(1), m_step(1)
	, m_init(false)
{
}

COpVectorMaker::~COpVectorMaker(void)
{
}

bool COpVectorMaker::Invoke(void)
{
	if (! m_init)
	{
		m_init = true;
		// get start
		JCASSERT(m_src[0]);
		AUTO_IVAL val_begin;
		m_src[0]->GetResult(val_begin);	JCASSERT(val_begin);
		jcvos::GetVal(val_begin, m_current);

		// get end
		JCASSERT(m_src[1]);
		AUTO_IVAL val_end;
		m_src[1]->GetResult(val_end);	JCASSERT(val_end);
		jcvos::GetVal(val_end, m_end);

		// get step
		if (m_src[2])
		{
			AUTO_IVAL val_step;
			m_src[2]->GetResult(val_step);	JCASSERT(val_step);
			jcvos::GetVal(val_step, m_step);
		}
		else m_step = 1;
		// check
		if ( (m_end - m_current) * m_step < 0)	THROW_ERROR(ERR_USER, _T("step is wrong")); 
		LOG_SCRIPT(_T("init: %d -> %d, step %d"), m_current, m_end, m_step );
	}
	// 仅考虑 m_step > 0
	LOG_SCRIPT(_T("%d"), m_current );
	if (m_current >= m_end) return false;

	// dummy
	jcvos::IValue * val = jcvos::CTypedValue<int>::Create(m_current);
	m_outport->PushResult(val);
	val->Release();

	m_current += m_step;
	return true;
}

bool COpVectorMaker::GetResult(jcvos::IValue * & val)
{
	return false;
}

void COpVectorMaker::DebugInfo(FILE * outfile)
{
	jcvos::jc_fprintf(outfile, _T(":") );
}

void COpVectorMaker::SetOutPort(IOutPort * outport)
{
	JCASSERT(NULL == m_outport);
	m_outport = outport;
}


