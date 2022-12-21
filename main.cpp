#include "main.h"

std::string				path = ".\\inventoryicons\\";
bool					helper_enabled = false;
SAMP::CTextDrawPool*	textdraw_pool = nullptr;

SAMP::CInput**			ppInput = nullptr;
unsigned int			AddCommandAddr = 0;
unsigned int			WndProcAddr = 0;

std::unordered_map<size_t, RwRaster*> icons;

RwCamera***		pppCamera = 0;

unsigned int	FindFreeSlot = 0;
RwTexture**		RwTexureSlot = 0;

unsigned int	r1_hook_exit = 0;
unsigned int	r3_hook_exit = 0;
unsigned int	r3_hook_exit_1 = 0;

void _ScanDirectory(const std::string &path, std::deque<WIN32_FIND_DATA> &data)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile((path + "*").c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do {
		if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
			continue;
		data.push_back(fd);
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
}

void UnloadIcons()
{
	for (auto &it : icons)
		RwRasterDestroy(it.second);
	icons.clear();
}

void LoadIcons()
{
	std::deque<WIN32_FIND_DATA> data;
	_ScanDirectory(path, data);

	UnloadIcons();

	for (auto &it : data)
	{
		if (it.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		size_t index = 0;
		if (sscanf(it.cFileName, "%08X.png", &index) != 1)
			continue;

		if (RwImage* image = RtPNGImageRead((path + it.cFileName).c_str()))
		{
			RwInt32 width = 0, height = 0, depth = 0, flags = 0;
			RwImageFindRasterFormat(image, 4, &width, &height, &depth, &flags);

			if (width == height)
			{
				if (RwRaster* raster = RwRasterCreate(width, height, depth, flags))
				{
					RwRasterSetFromImage(raster, image);
					icons.insert_or_assign(index, raster);
				}
			}
			RwImageDestroy(image);
		}
	}
}

RwTexture* RenderIcon(RwRGBA* color, RwRaster* raster)
{
	static RwIm2DVertex verts[4] =
	{
		{ 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f },
	};

	RwRaster* pRaster = RwRasterCreate(raster->width, raster->height, 0, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	if (pRaster == nullptr)
		return nullptr;

	verts[1].x = raster->width;
	verts[3].x = raster->width;
	verts[2].y = raster->height;
	verts[3].y = raster->height;

	RwCamera* pCamera = **(RwCamera***)pppCamera;

	pCamera->frameBuffer = pRaster;
	pCamera->zBuffer = nullptr;
	
	RwCameraClear(pCamera, color, rwCAMERACLEARIMAGE);
	
	if (pCamera->beginUpdate(pCamera))
	{
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER,	RWRSTATE(raster));
		RwRenderStateSet(rwRENDERSTATEZTESTENABLE,		RWRSTATE(FALSE));
		RwRenderStateSet(rwRENDERSTATESHADEMODE,		RWRSTATE(rwSHADEMODEGOURAUD));
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,		RWRSTATE(FALSE));
		RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,	RWRSTATE(rwFILTERNEAREST));
		RwRenderStateSet(rwRENDERSTATEFOGENABLE,		RWRSTATE(FALSE));
		RwRenderStateSet(rwRENDERSTATECULLMODE,			RWRSTATE(rwCULLMODECULLNONE));

		RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, 4);

		pCamera->endUpdate(pCamera);
	}

	RwTexture* pTexture = RwTextureCreate(pRaster);
	if (pTexture == nullptr)
	{
		RwRasterDestroy(pRaster);
	}

	return pTexture;
}

RwTexture* __stdcall GenTextdrawIcon(SAMP::CTextDraw* _this)
{
	// { m_nModel, m_rotation[3], m_fZoom } => hash
	std::string str((char*)(&_this->m_data.m_nModel), 2 + 12 + 4);
	size_t hash = std::hash<std::string>()(str);

	auto result = icons.find(hash);
	if (result == icons.end())
		return nullptr;
	
	return RenderIcon((RwRGBA*)&_this->m_data.m_backgroundColor, result->second);
}

void __fastcall r1_hook_ACDB0(SAMP::CTextDraw* _this)
{
	if (_this->m_data.m_nStyle == 5 && _this->m_data.m_nIndex == -1)
	{
		if (RwTexture* texture = GenTextdrawIcon(_this))
		{
			int FreeSlot = ((int(__cdecl*)())FindFreeSlot)();
			_this->m_data.m_nIndex = FreeSlot;
			RwTexureSlot[FreeSlot] = texture;
			return;
		}
	}
	return ((void(__thiscall*)(SAMP::CTextDraw*))r1_hook_exit)(_this);
}

void __declspec(naked) r3_hook_B2CCB()
{
	__asm
	{
		push esi
		call GenTextdrawIcon

		cmp eax, 0
		jne if_exists

		movsx eax, word ptr[esi+9A8h]
		jmp r3_hook_exit

	if_exists:
		jmp r3_hook_exit_1
	}
}

// ------------------------------------------------------------ //

void cmd_toggle_helper(const char* params)
{
	helper_enabled ^= true;
}

void cmd_reload_icons(const char* params)
{
	LoadIcons();
}

void _AddCommand(const char* name, SAMP::CMDPROC proc)
{
	return ((void(__thiscall *)(SAMP::CInput*, const char*, SAMP::CMDPROC))AddCommandAddr)(*ppInput, name, proc);
}

void __stdcall mainloop(SAMP::CTextDrawPool* pool)
{
	static bool init = false;
	if (!init)
	{
		if (pool == nullptr || d3ddev == nullptr || *ppInput == nullptr)
			return;

		MH_Initialize();
		MH_InstallHook(VT_FUNC(d3ddev, 16), dxReset);
		MH_InstallHook(VT_FUNC(d3ddev, 17), dxPresent);
		MH_InstallHook(WndProcAddr, wndproc);

		textdraw_pool = pool;

		LoadIcons();

		_AddCommand("ii.reload", cmd_reload_icons);
		_AddCommand("ii.helper", cmd_toggle_helper);

		init = true;
	}
}

void __declspec(naked) hook_1E143()
{
	__asm
	{
		push	edi
		call	mainloop

		pop		edi
		pop		esi
		retn
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH && SAMP::GetHandle())
	{
		char buf[MAX_PATH];
		if (!GetModuleFileName(hModule, buf, MAX_PATH))
			return false;

		path = buf;
		path = path.substr(0, path.find_last_of("/\\"));
		path += "\\inventoryicons\\";

		IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(SAMP::GetHandle());
		IMAGE_NT_HEADERS* ntHeader = reinterpret_cast<IMAGE_NT_HEADERS*>((uint8_t*)dosHeader + dosHeader->e_lfanew);

		if (ntHeader->OptionalHeader.AddressOfEntryPoint == 0x31DF13)
		{
			ppInput			= (SAMP::CInput**)SAMP_ADDROF(0x21A0E8);
			AddCommandAddr	= SAMP_ADDROF(0x65AD0);
			WndProcAddr		= SAMP_ADDROF(0x6CF90);

			FindFreeSlot	= SAMP_ADDROF(0xAC4E0);
			RwTexureSlot	= (RwTexture**)SAMP_ADDROF(0x216058);
			pppCamera		= (RwCamera***)SAMP_ADDROF(0x21A108);

			r1_hook_exit	= SAMP_ADDROF(0x299CFF);
			plugin::patch::RedirectJump(SAMP_ADDROF(0xACDB0), r1_hook_ACDB0);

			plugin::patch::RedirectJump(SAMP_ADDROF(0x1ADA1), hook_1E143);
		}
		else if (ntHeader->OptionalHeader.AddressOfEntryPoint == 0x0CC4D0)
		{
			ppInput			= (SAMP::CInput**)SAMP_ADDROF(0x26E8CC);
			AddCommandAddr	= SAMP_ADDROF(0x69000);
			WndProcAddr		= SAMP_ADDROF(0x70E80);

//			FindFreeSlot	= SAMP_ADDROF(0xB2290);
//			RwTexureSlot	= (RwTexture**)SAMP_ADDROF(0x26B2B8);
			pppCamera		= (RwCamera***)SAMP_ADDROF(0x26E8F0);

			r3_hook_exit	= SAMP_ADDROF(0xB2CD2);
			r3_hook_exit_1	= SAMP_ADDROF(0xB2DF7);
			plugin::patch::RedirectJump(SAMP_ADDROF(0xB2CCB), r3_hook_B2CCB);

			plugin::patch::RedirectJump(SAMP_ADDROF(0x1E141), hook_1E143);
		}
	}
	return TRUE;
}
