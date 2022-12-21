#include "main.h"

IDirect3DDevice9*		&d3ddev = *reinterpret_cast<IDirect3DDevice9**>(0xC97C28);
D3DPRESENT_PARAMETERS	&d3dpp = *reinterpret_cast<D3DPRESENT_PARAMETERS*>(0xC9C040);

bool			render_inited = false;

SAMP::ID		active_textdraw = 0xFFFF;
POINT			mouse_position = { 0, 0 };

DX9_Reset_t		orig_dxReset = nullptr;
DX9_Present_t	orig_dxPresent = nullptr;
TDS_WndProc_t	orig_wndproc = nullptr;

CD3DRender*		render = new CD3DRender(16);
CD3DFont*		font = new CD3DFont("Arial", 8, FCR_BOLD | FCR_BORDER);

void ConvertGameToScreenCoords(float *x, float *y)
{
	*x = ((*x) * 0.001562500) * (float)d3dpp.BackBufferWidth;
	*y = ((*y) * 0.002232143) * (float)d3dpp.BackBufferHeight;
}

bool rect_hovered(int x, int y, int w, int h)
{
	return (mouse_position.x > x && mouse_position.x < x + w && mouse_position.y > y && mouse_position.y < y + h);
}

void SetClipboardText(const char* text)
{
	if (!OpenClipboard(NULL))
		return;
	const int wbuf_length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	HGLOBAL wbuf_handle = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(WCHAR));
	if (wbuf_handle != NULL)
	{
		WCHAR* wbuf_global = (WCHAR*)GlobalLock(wbuf_handle);
		MultiByteToWideChar(CP_UTF8, 0, text, -1, wbuf_global, wbuf_length);
		GlobalUnlock(wbuf_handle);
		EmptyClipboard();
		if (SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
			GlobalFree(wbuf_handle);
	}
	CloseClipboard();
}

HRESULT CALLBACK hooked_dxPresent(IDirect3DDevice9* pDevice, const RECT* r1, const RECT* r2, HWND hwnd, const RGNDATA* rngd)
{
	if (!render_inited)
	{
		render->Initialize(pDevice);
		font->Initialize(pDevice);
		render_inited = true;
	}

	if (helper_enabled && SUCCEEDED(render->BeginRender()))
	{
		active_textdraw = 0xFFFF;
		for (int i = 0; i < 2304; ++i)
		{
			if (textdraw_pool->m_bNotEmpty[i] == FALSE)
				continue;

			SAMP::CTextDraw* info = textdraw_pool->m_pObject[i];
			if (info == nullptr || info->m_data.m_nStyle != 5)
				continue;

			float x = info->m_data.m_fX;
			float y = info->m_data.m_fY;
			float w = info->m_data.m_fBoxSizeX;
			float h = info->m_data.m_fBoxSizeY;

			ConvertGameToScreenCoords(&x, &y);
			ConvertGameToScreenCoords(&w, &h);

			std::string hash_str = std::string((char*)&info->m_data.m_nModel, 2 + 12 + 4);

			char text[32];
			sprintf(text, "%08X", std::hash<std::string>()(hash_str));

			bool is_hovered = rect_hovered(x, y, w, h);
			if (is_hovered)
			{
				render->D3DBox(x, y, w, h, (GetKeyState(VK_LBUTTON) & 0x8000) ? 0x40FFFFFF : 0x4000FF00);
				active_textdraw = i;
			}
			font->Print(text, is_hovered ? 0xFF00FF00 : 0xFFBEBEBE, (int)x, (int)y);
		}
		render->EndRender();
	}
	return orig_dxPresent(pDevice, r1, r2, hwnd, rngd);
}

HRESULT CALLBACK hooked_dxReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	font->Invalidate();
	render->Invalidate();
	render_inited = false;
	return orig_dxReset(pDevice, pPresentationParameters);
}

BOOL __fastcall hooked_wndproc(SAMP::CTextDrawSelection* ecx, void* edx, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (helper_enabled)
	{
		if (msg == WM_MOUSEMOVE)
		{
			mouse_position.x = ((int)(short)LOWORD(lParam));
			mouse_position.y = ((int)(short)HIWORD(lParam));
		}
		else if (msg == WM_LBUTTONUP && active_textdraw != 0xFFFF)
		{
			SAMP::CTextDraw* info = textdraw_pool->m_pObject[active_textdraw];
			if (info && info->m_data.m_nStyle == 5)
			{
				std::string hash_str = std::string((char*)&info->m_data.m_nModel, 2 + 12 + 4);

				char text[32];
				sprintf(text, "%08X", std::hash<std::string>()(hash_str));

				SetClipboardText(text);

				return TRUE;
			}
		}
	}
	return orig_wndproc(ecx, msg, wParam, lParam);
}