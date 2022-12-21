/*
	This is a SAMP (0.3.7-R3) API project file.
	Developer: LUCHARE <luchare.dev@gmail.com>
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#pragma once

#include "common.h"

#define MAX_CLIENT_CMDS 144
#define MAX_CMD_LENGTH 32

SAMP_BEGIN

typedef void(__cdecl *CMDPROC)(const char *);

class SAMP_API CInput {
public:
	IDirect3DDevice9 *m_pDevice;
	CDXUTDialog		  *m_pGameUI;
	CDXUTEditBox	  *m_pEditbox;
	CMDPROC				m_commandProc[MAX_CLIENT_CMDS];
	char					m_szCommandName[MAX_CLIENT_CMDS][MAX_CMD_LENGTH + 1];
	int					m_nCommandCount;
	BOOL					m_bEnabled;
	char					m_szInput[129];
	char					m_szRecallBufffer[10][129];
	char					m_szCurrentBuffer[129];
	int					m_nCurrentRecall;
	int					m_nTotalRecall;
	CMDPROC				m_defaultCommand;
};

extern CInput *&pInputBox;

SAMP_END