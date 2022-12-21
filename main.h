#ifndef _MAIN_H_
#define _MAIN_H_

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS 1
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1

#include <windows.h>
#include <deque>
#include <unordered_map>
#include <string>

// plugin sdk / shared
#include <Patch.h>
#include <RenderWare.h>

// samp api
#include "sampapi/CInput.h"
#include "sampapi/CTextDraw.h"
#include "sampapi/CTextDrawPool.h"
#include "sampapi/CTextDrawSelection.h"

#include "minhook/MinHook.h"
#pragma comment(lib, "minhook/MinHook.lib")

#include "d3drender.h"

typedef HRESULT(WINAPI* DX9_Present_t)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
typedef HRESULT(WINAPI* DX9_Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef BOOL(__thiscall* TDS_WndProc_t)(void* ecx, UINT msg, WPARAM wParam, LPARAM lParam);

#define VT_FUNC(vt, i)	((void**)(*(void***)(vt))[i])

extern bool helper_enabled;
extern SAMP::CTextDrawPool* textdraw_pool;

extern DX9_Reset_t orig_dxReset;
extern DX9_Present_t orig_dxPresent;
extern TDS_WndProc_t orig_wndproc;
HRESULT CALLBACK hooked_dxReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT CALLBACK hooked_dxPresent(IDirect3DDevice9* pDevice, const RECT* r1, const RECT* r2, HWND hwnd, const RGNDATA* rngd);
BOOL __fastcall hooked_wndproc(SAMP::CTextDrawSelection* ecx, void* edx, UINT msg, WPARAM wParam, LPARAM lParam);

extern IDirect3DDevice9*& d3ddev;
extern D3DPRESENT_PARAMETERS& d3dpp;

#endif
