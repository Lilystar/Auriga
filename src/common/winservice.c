/*
 * Copyright (C) 2002-2007  Auriga
 *
 * This file is part of Auriga.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifdef WINDOWS

#pragma comment(lib, "advapi32.lib")

#include <stdio.h>
#include <windows.h>
#include <winsvc.h>
#include <sys/types.h>

#include "winservice.h"
#include "core.h"

static SERVICE_STATUS_HANDLE service_handle = 0;
static SERVICE_STATUS        service_status;

static char service_name[128] = "";
static int checkpoint = 0;

static int cache_argc = 0;
static char *cache_argv[128];

static void winservice_final(void);

/*==========================================
 * �J�����g�f�B���N�g����exe�̈ʒu�ɕύX
 *------------------------------------------
 */
bool winservice_change_current_dir(void)
{
	size_t i;
	char path[MAX_PATH];

	if(!GetModuleFileName(NULL, path, MAX_PATH))
		return false;

	for(i = strlen(path) - 1; i != 0; i--) {
		if(path[i] == '/' || path[i] == '\\') {
			path[i] = '\0';
			break;
		}
	}
	SetCurrentDirectory(path);

	return true;
}

/*==========================================
 * �T�[�r�X�̏�ԕύX
 *------------------------------------------
 */
static bool winservice_change_status(void)
{
	return SetServiceStatus(service_handle, &service_status);
}

/*==========================================
 * �T�[�r�X�̃n���h��
 *------------------------------------------
 */
static DWORD WINAPI winservice_ctrl_handler(DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context)
{
	switch(ctrl) {
		case SERVICE_CONTROL_INTERROGATE:	// ���
			break;
		case SERVICE_CONTROL_STOP:			// ��~
		case SERVICE_CONTROL_SHUTDOWN:		// �V���b�g�_�E��
			do_stop();
			break;
		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
	}

	return NO_ERROR;
}

/*==========================================
 * �T�[�r�X�̃��C�����[�`��
 *------------------------------------------
 */
static void WINAPI winservice_main(DWORD argc, LPSTR *argv)
{
	// �n���h���o�^
	service_handle = RegisterServiceCtrlHandlerEx(service_name, winservice_ctrl_handler, NULL);

	if(service_handle == 0)
		return;

	atexit(winservice_final);

	printf("--> Windows Service Startup ...\n");

	// ��ԍ\���̂̏�����
	checkpoint = 0;
	service_status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	service_status.dwCurrentState            = SERVICE_START_PENDING;
	service_status.dwWin32ExitCode           = NO_ERROR;
	service_status.dwServiceSpecificExitCode = NO_ERROR;
	service_status.dwCheckPoint              = ++checkpoint;
	service_status.dwWaitHint                = 0;
	service_status.dwControlsAccepted =      SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	// �����J�n��ʒm
	winservice_change_status();

	// �T�[�r�X�̊J�n�p�����[�^�͖������ċN�����̃p�����[�^���g���ă��C�����s
	main_sub(cache_argc, cache_argv);

	return;
}

/*==========================================
 * �T�[�r�X�̊J�n���ʒm
 *------------------------------------------
 */
bool winservice_notify_start()
{
	if(service_handle == 0)
		return true;

	service_status.dwCurrentState = SERVICE_START_PENDING;
	service_status.dwCheckPoint   = ++checkpoint;
	//service_status.dwWaitHint = 0;

	return winservice_change_status();
}

/*==========================================
 * �T�[�r�X�̏��������ʒm
 *------------------------------------------
 */
bool winservice_notify_ready(void)
{
	if(service_handle == 0)
		return true;

	checkpoint = 0;
	service_status.dwCurrentState = SERVICE_RUNNING;
	service_status.dwCheckPoint   = 0;
	service_status.dwWaitHint     = 0;

	return winservice_change_status();
}

/*==========================================
 * �T�[�r�X�̒�~���ʒm
 *------------------------------------------
 */
bool winservice_notify_stop(void)
{
	if(service_handle == 0)
		return true;

	service_status.dwCurrentState = SERVICE_STOP_PENDING;
	service_status.dwCheckPoint   = ++checkpoint;
	//service_status.dwWaitHint = 0;

	return winservice_change_status();
}

/*==========================================
 * �T�[�r�X�̒�~�����ʒm
 *------------------------------------------
 */
bool winservice_notify_finish(void)
{
	if(service_handle == 0)
		return true;

	checkpoint = 0;
	service_status.dwCurrentState = SERVICE_STOPPED;
	service_status.dwCheckPoint   = 0;
	service_status.dwWaitHint     = 0;
	//service_status.dwWin32ExitCode = 0;

	return winservice_change_status();
}

/*==========================================
 * �T�[�r�X�̏I������
 *------------------------------------------
 */
static void winservice_final(void)
{
	printf("<-- Windows Service Shutdown ...\n");

	winservice_notify_finish();

	return;
}

/*==========================================
 * �T�[�r�X�̏�����
 *------------------------------------------
 */
bool winservice_init(int argc, char **argv)
{
	int i;
	bool result;
	SERVICE_TABLE_ENTRY table[2];

	// �N������
	for(i = 1; i < argc - 1; i += 2) {
		if(strcmp(argv[i], "--service-name") == 0) {
			strncpy(service_name, argv[i+1], sizeof(service_name));
			service_name[sizeof(service_name) - 1] = '\0';
		}
	}

	if(!service_name[0]) {
		// �T�[�r�X����̋N���ł͂Ȃ�
		return false;
	}

	// �p�����[�^�̕ۑ�
	cache_argc = argc;
	for(i = 0; i < argc && i < sizeof(cache_argv)/sizeof(cache_argv[0]); i++) {
		cache_argv[i] = argv[i];
	}

	memset(table, 0, sizeof(table));

	table[0].lpServiceName = service_name;
	table[0].lpServiceProc = winservice_main;

	// �T�[�r�X�R���g���[���f�B�X�p�b�`���N��
	result = StartServiceCtrlDispatcher(table);
	if(result == false)
		printf("StartServiceCtrlDispatcher failed (%u)\n", GetLastError());

	return result;
}

#endif
