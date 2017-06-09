#pragma once
#include "ChessBoard.h"

struct PLAY_INFO
{
	UINT	m_index;
	UINT	m_seed;
	int		m_preset;
	int		m_score;
	UINT	m_player_ai[2];
	UINT	m_player_param[2];
	int		m_winner;
	int		m_movement;
};


#define MSG_EXIT	1
#define MSG_PLAY_FINISHED	2

class CTestHook
{
public:
	CTestHook(CWnd * wnd = NULL);
	virtual ~CTestHook(void);

	// for new thread
public:
	void Start(void);
	void SetMainWnd(CWnd * wnd)	{m_wnd = wnd; };
	bool WaitForTerminate(DWORD timeout);
	bool SendMessage(UINT id, PLAY_INFO * info);

protected:
	static DWORD WINAPI StaticRun(LPVOID param);
	virtual DWORD Run(void);

protected:
	// 用于消息的线程同步
	CRITICAL_SECTION	m_msg_critical; 
	// 消息触发事件
	HANDLE	m_msg_event;
	UINT		m_msg_id;
	PLAY_INFO	m_msg_info;

	HANDLE m_thread;
	CWnd	* m_wnd;
};
