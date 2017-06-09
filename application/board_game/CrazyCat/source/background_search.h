#pragma once
#include "ChessBoard.h"

#include "robot_catcher.h"

class ISearchWrapper
{
public:
	virtual void Release(void) = 0;
	virtual void StartSearch(const CChessBoard * board, int depth) = 0;
	virtual void SetRobot(IRobot * robot) = 0;
	virtual void DetachRobot(void) = 0;
	virtual int GetProgress(void) = 0;
	//virtual void TerminateSearch(void) = 0;
};


class CBackgroundSearch	: public ISearchWrapper
{
public:
	CBackgroundSearch(void);
	virtual ~CBackgroundSearch(void);

// 以下函数都是由控制器调用，工作与前台线程
public:
	// 启动搜索
	virtual void StartSearch(const CChessBoard * board, int depth);
	// 任务完成，销毁线程。
	virtual void SetRobot(IRobot * robot);
	virtual void DetachRobot(void);
	virtual void Release(void) {delete this;}
	virtual int GetProgress(void);
	virtual void TerminateSearch(void);

// 以下函数工作与后台线程
protected:
	static DWORD WINAPI StaticRun(LPVOID param);
	DWORD Run(void);

protected:
	IRobot * m_robot;

	HANDLE	m_start_event;
	HANDLE	m_thread;
	// 0: continue running, 1: stop running
	volatile long		m_terminate;
	CChessBoard * m_board;
	int		m_depth;
};

class CFrontSearch	: public ISearchWrapper
{	
public:
	CFrontSearch(void) : m_robot(NULL) {};
	virtual ~CFrontSearch(void) {delete m_robot; };
public:
	virtual void StartSearch(const CChessBoard * board, int depth)
	{
		JCASSERT(m_robot);
		CChessBoard * _board = NULL;
		board->Dupe(_board);
		m_robot->StartSearch(_board, depth);
		delete _board;
	}
	virtual void SetRobot(IRobot * robot)
	{
		JCASSERT(m_robot == NULL);
		m_robot = robot;
	}
	virtual void DetachRobot(void) 
	{
		delete m_robot;
		m_robot = NULL;
	}
	virtual void Release(void) {delete this;}
	virtual int GetProgress(void)
	{
		if (!m_robot) return 0;
		long prog = 0, max_prog = 0;
		m_robot->GetProgress(prog, max_prog);
		return prog * 100 / max_prog;
	}


protected:
	IRobot * m_robot;
};