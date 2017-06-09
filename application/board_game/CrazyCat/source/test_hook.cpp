#define _CRT_RAND_S
#include "stdafx.h"
#include "configuration.h"
#include "test_hook.h"
#include "../resource.h"
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("test_hook"), LOGGER_LEVEL_DEBUGINFO);

CTestHook::CTestHook(CWnd * wnd)
	: m_thread(NULL)
	, m_wnd(wnd)
	, m_msg_event(NULL)
	, m_msg_id(0)
{
	InitializeCriticalSection(&m_msg_critical);

	m_msg_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!m_msg_event) THROW_WIN32_ERROR(_T("failure on create syncronize event"));

	//m_thread = CreateThread(NULL, 64 * 1024, 
	//		StaticRun, (LPVOID)(this), 
	//		CREATE_SUSPENDED , NULL);
	//if (!m_thread) THROW_WIN32_ERROR(_T("failure on creating thread"));
}

CTestHook::~CTestHook(void)
{
	if (m_thread) CloseHandle(m_thread);
	if (m_msg_event)	CloseHandle(m_msg_event);
	DeleteCriticalSection(&m_msg_critical);
}

DWORD WINAPI CTestHook::StaticRun(LPVOID param)
{
	CTestHook * this_ptr = reinterpret_cast<CTestHook *>(param);
	return this_ptr->Run();
}

#define TEST_LOOP	10000

DWORD CTestHook::Run(void)
{
	LOG_STACK_TRACE();
	static const int MAX_CHESS = BOARD_SIZE_ROW * BOARD_SIZE_COL;

	HANDLE file = CreateFile(_T("test_result.txt"),
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				OPEN_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL,					//如果使用NO_BUFFERING选项，文件操作必须sector对齐。
				NULL );
	char log[256];
	// write header
	sprintf_s(log, ("index,preset,seed,score,ai_id_1,ai_param_1,ai_id_2,ai_param_2,winner,step\n"));
	DWORD written = 0;
	BOOL br = WriteFile(file, log, strlen(log), &written, NULL);

	int win_rate[MAX_CHESS];
	memset(win_rate, 0, sizeof(win_rate));
	int win_step[MAX_CHESS];
	memset(win_step, 0, sizeof(win_step));
	int loss_step[MAX_CHESS];
	memset(loss_step, 0, sizeof(loss_step));

	// waiting for window ready
	//<>
	LOG_DEBUG(_T("start test"));
	Sleep(2000);
	TCHAR str[64];
	// loop
	int loops = 0;
	bool exit = false;
	for (loops = 0; loops < TEST_LOOP; ++loops)
	{
		for (int pp = 1; pp < MAX_CHESS-1; ++pp)
		{
			LOG_DEBUG(_T("generate game preset= %d"), pp);
			HWND hwnd = NULL;
			// invoke test item

			// set preset chess
			_stprintf_s(str, _T("%d"), pp);
			m_wnd->GetDlgItem(IDC_GAME_CHESS, &hwnd);
			::SetWindowText(hwnd, str);
			//Sleep(50);

			// set seed
			UINT seed=0;
			rand_s(&seed);
			seed &= RAND_MAX;
			_stprintf_s(str, _T("%d"), seed);
			m_wnd->GetDlgItem(IDC_SEED, &hwnd);
			::SetWindowText(hwnd, str);

			// generate game
			LOG_DEBUG(_T("start press make game"));
			m_wnd->SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BT_MAKEGAME, BN_CLICKED));
			LOG_DEBUG(_T("finished press make game"));

			// set player catcher and cat
			//CWnd * player_cat =m_wnd->GetDlgItem(IDC_PLAYER_CATCHER);
			//player_cat->SendMessage(CB_SETCURSEL, 2, 0);
			//m_wnd->GetDlgItem(IDC_PLAYER_CAT, &hwnd);
			//::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

			// set depth
			//m_wnd->GetDlgItem(IDC_SEARCH_DEPTH, &hwnd);
			//::SetWindowText(hwnd, _T("3"));

			// start play
			LOG_DEBUG(_T("start play game"));
			m_wnd->SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BT_PLAY, BN_CLICKED) );
			LOG_DEBUG(_T("finishedn play game"));

			// waiting rest result
			WaitForSingleObject(m_msg_event, INFINITE);
			EnterCriticalSection(&m_msg_critical);
			// log result
			if (m_msg_id == MSG_EXIT) 
			{
				LeaveCriticalSection(&m_msg_critical);
				exit = true;
				break;
			}
//			_stprintf_s(str, _T("index,preset,seed,score,ai_id_1,ai_param_1,ai_id_2,ai_param_2,winner,step\n"));
			int len = sprintf_s(log, ("%d,%d,%d,%d,%d,%d,%d,%d,%S,%d\n"), 
				m_msg_info.m_index, m_msg_info.m_preset, m_msg_info.m_seed, m_msg_info.m_score,
				m_msg_info.m_player_ai[0], m_msg_info.m_player_param[0], m_msg_info.m_player_ai[1], m_msg_info.m_player_param[1],
				CChessBoard::PLAYER_SNAME[m_msg_info.m_winner], m_msg_info.m_movement);
			if (m_msg_info.m_winner == PLAYER_CATCHER)
			{
				win_rate[m_msg_info.m_preset]++;
				win_step[m_msg_info.m_preset]+=m_msg_info.m_movement;
			}
			else
			{
				loss_step[m_msg_info.m_preset]+=m_msg_info.m_movement;
			}
			m_msg_id = 0;
			LeaveCriticalSection(&m_msg_critical);
			written = 0;
			br = WriteFile(file, log, len, &written, NULL);
			FlushFileBuffers(file);
		}
		if (exit) break;
	}
	
	//int len = sprintf_s(log, "win rate: preset,rate,win_step,loss_step\n");
	//br = WriteFile(file, log, len, &written, NULL);
	//for (int pp = 1; pp < MAX_CHESS; ++ pp)
	//{
	//	len = sprintf_s(log, "%d,%.1f%%,%.1f,%.1f\n", pp, (win_rate[pp]*100) / (float)(loops)
	//		, win_step[pp] / (float)(win_rate[pp]), loss_step[pp] / (float)(loops - win_rate[pp]));
	//	br = WriteFile(file, log, len, &written, NULL);
	//}

	CloseHandle(file);
	if (m_wnd->m_hWnd)	m_wnd->SendMessage(WM_CLOSE);

	return 0;
}

void CTestHook::Start(void)
{
	//JCASSERT(m_thread);
	//ResumeThread(m_thread);
	m_thread = CreateThread(NULL, 1024 * 1024, 
			StaticRun, (LPVOID)(this), 
			0, NULL);
	if (!m_thread) THROW_WIN32_ERROR(_T("failure on creating thread"));
}

bool CTestHook::WaitForTerminate(DWORD timeout)
{
	if (!m_thread) return true;
	//ResumeThread(m_thread);
	DWORD ir = WaitForSingleObject(m_thread, timeout);
	CloseHandle(m_thread);	m_thread = NULL;
	return (ir != WAIT_TIMEOUT);
}

bool CTestHook::SendMessage(UINT id, PLAY_INFO * info)
{
	//JCASSERT(info);
	EnterCriticalSection(&m_msg_critical);
	JCASSERT(m_msg_id == 0);
	m_msg_id = id;
	if (info)	m_msg_info = *info;
	LeaveCriticalSection(&m_msg_critical);
	SetEvent(m_msg_event);
	return true;
}
