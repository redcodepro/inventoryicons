/*
	This is a SAMP (0.3.7-R3) API project file.
	Developer: LUCHARE <luchare.dev@gmail.com>
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#pragma once

#include "common.h"

SAMP_BEGIN

class CTextDrawSelection {
public:
    BOOL     m_bIsActive;
    D3DCOLOR m_hoveredColor;
    ID       m_nHoveredId;
};

SAMP_END
