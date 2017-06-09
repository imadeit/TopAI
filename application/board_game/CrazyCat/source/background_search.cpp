#include "stdafx.h"
#include "background_search.h"

#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("background_search"), LOGGER_LEVEL_NOTICE);

CBackgroundSearch::CBackgroundSearch(void)
	: m_start_event(NULL)	, m_thread(NULL)
	, m_board(NULL)
	, m_depth(0)
	, m_terminate(0)
	, m_robot(NULL)
{
	m_start_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!m_start_event) THROW_WIN32_ERROR(_T("failure on create syncronize event"));
}

CBackgroundSearch::~CBackgroundSearch(void)
{
	if (m_thread)
	{	// 等待线程结束
		DetachRobot();
	}
	delete m_board;
	if (m_start_event) CloseHandle(m_start_event);
}

DWORD WINAPI CBackgroundSearch::StaticRun(LPVOID param)
{
	CBackgroundSearch * this_ptr = reinterpret_cast<CBackgroundSearch *>(param);
	return this_ptr->Run();
}

DWORD CBackgroundSearch::Run(void)
{
	DWORD	ir = 0;
	while (1)
	{
		DWORD ir = WaitForSingleObject(m_start_event, INFINITE);
		if (ir != WAIT_OBJECT_0)	THROW_WIN32_ERROR(_T("failure on waiting object"));
		if ( InterlockedCompareExchange(&m_terminate, 0, 0) != 0 )
		{
			ir = 0;
			break;
		}
		LOG_DEBUG(_T("start searching"));
		JCASSERT(m_robot);
		try
		{
			m_robot->StartSearch(m_board, m_depth);
		}
		catch (int)
		{
			LOG_DEBUG(_T("searching is canceled"));
			ir = 1;
			break;
		}
	
		LOG_DEBUG(_T("finish searching"));
		ResetEvent(m_start_event);
	};
	LOG_DEBUG(_T("terminate searching thread"));
	return ir;
}

// 一下函数都是由控制器调用，工作与前台线程

void CBackgroundSearch::SetRobot(IRobot * robot)
{
	// 确保后台线程停止
	LOG_STACK_TRACE();
	JCASSERT(NULL == m_thread);
	JCASSERT(m_start_event);
	m_terminate = 0;
	m_robot = robot;

	ResetEvent(m_start_event);
	m_thread = CreateThread(NULL, 1024 * 1024, StaticRun, (LPVOID)(this), 0, NULL);
	if (!m_thread) THROW_WIN32_ERROR(_T("failure on creating thread"));
}

void CBackgroundSearch::DetachRobot(void)
{	// 终止当前搜索线程
	LOG_STACK_TRACE();
	JCASSERT(m_start_event);
	if (m_robot)	m_robot->CancelSearch();
	InterlockedExchange(&m_terminate, 1);
	SetEvent(m_start_event);
	LOG_DEBUG(_T("waiting thread exit"));
	DWORD ir = WaitForSingleObject(m_thread, 1000);
	if (WAIT_TIMEOUT == ir) THROW_ERROR(ERR_USER, _T("terminate thread failed"));
	LOG_DEBUG(_T("thread exited"));
	CloseHandle(m_thread);		m_thread = NULL;
	if (m_robot)	m_robot->Release(),	m_robot = NULL;
}

void CBackgroundSearch::TerminateSearch(void)
{
	LOG_STACK_TRACE();
	if (m_thread)
	{
		LOG_DEBUG(_T("waiting thread exit"));
		TerminateThread(m_thread, 0);
		DWORD ir = WaitForSingleObject(m_thread, 1000);
		if (WAIT_TIMEOUT == ir) THROW_ERROR(ERR_USER, _T("terminate thread failed"));
		LOG_DEBUG(_T("thread exited"));
		CloseHandle(m_thread);		m_thread = NULL;
	}
	if (m_robot) m_robot->Release(), m_robot = NULL;
}
	
void CBackgroundSearch::StartSearch(const CChessBoard * board, int depth)
{	// this function runs in front thread
	JCASSERT(m_start_event);

	// 等待搜索过程结束 = m_start_event reset
	
	while (1)
	{
		// 测试event在reset状态
		DWORD ir = WaitForSingleObject(m_start_event, 0);
		if (ir == WAIT_TIMEOUT) break;
		LOG_WARNING(_T("warning! waiting for search thread..."));
		Sleep(50);
	}
	//if (ir != WAIT_TIMEOUT)		LOG_ERROR(_T("error! previous searching did finish"));
	//JCASSERT(ir == WAIT_TIMEOUT);
	// 复制棋盘
	delete m_board;	m_board = NULL;
	board->Dupe(m_board);
	m_depth = depth;

	SetEvent(m_start_event);
}

int CBackgroundSearch::GetProgress(void)
{	// this function runs in front thread
	if (!m_robot) return 0;
	long prog = 0, max_prog = 0;
	m_robot->GetProgress(prog, max_prog);
	return prog * 100 / max_prog;
}

