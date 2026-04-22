#include "resource.h"
#include "BForm.h"
#include <commdlg.h>
#include <vector>
#pragma comment(lib, "comdlg32.lib")

CBForm form1(ID_form1);

// 当前文件路径；为空表示尚未命名的新文件。
TCHAR g_szCurrentFile[MAX_PATH] = TEXT("");
// 文本是否已修改且未保存。
bool g_bModified = false;
// 代码主动设置文本时，避免触发“已修改”标志。
bool g_bInternalUpdating = false;

void UpdateTitle()
{
	if (g_szCurrentFile[0] != 0)
	{
		form1.TextSet(g_szCurrentFile);
		form1.TextAdd(TEXT(" - 我的记事本"));
	}
	else
	{
		form1.TextSet(TEXT("我的记事本"));
	}

	if (g_bModified)
	{
		form1.TextAdd(TEXT(" *"));
	}
}

bool SelectOpenFile(TCHAR* szFilePath, DWORD cchFilePath)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	szFilePath[0] = 0;

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = form1.hWnd();
	ofn.lpstrFile = szFilePath;
	ofn.nMaxFile = cchFilePath;
	ofn.lpstrFilter = TEXT("文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrDefExt = TEXT("txt");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	return GetOpenFileName(&ofn) == TRUE;
}

bool SelectSaveFile(TCHAR* szFilePath, DWORD cchFilePath)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	if (g_szCurrentFile[0] != 0)
	{
		lstrcpyn(szFilePath, g_szCurrentFile, cchFilePath);
	}
	else
	{
		szFilePath[0] = 0;
	}

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = form1.hWnd();
	ofn.lpstrFile = szFilePath;
	ofn.nMaxFile = cchFilePath;
	ofn.lpstrFilter = TEXT("文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrDefExt = TEXT("txt");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	return GetSaveFileName(&ofn) == TRUE;
}

bool DecodeMultiByteText(UINT codePage, const BYTE* pBytes, int cbBytes, tstring& textOut)
{
	if (cbBytes <= 0)
	{
		textOut = TEXT("");
		return true;
	}

	int cchNeed = MultiByteToWideChar(codePage, 0, (LPCSTR)pBytes, cbBytes, NULL, 0);
	if (cchNeed <= 0)
	{
		return false;
	}

	std::vector<WCHAR> chars;
	chars.resize(cchNeed + 1);
	if (MultiByteToWideChar(codePage, 0, (LPCSTR)pBytes, cbBytes, &chars[0], cchNeed) <= 0)
	{
		return false;
	}
	chars[cchNeed] = 0;
	textOut = &chars[0];
	return true;
}

bool LoadTextFile(LPCTSTR szFilePath)
{
	HANDLE hFile = CreateFile(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MsgBox(TEXT("文件打开失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	LARGE_INTEGER liSize;
	if (!GetFileSizeEx(hFile, &liSize))
	{
		CloseHandle(hFile);
		MsgBox(TEXT("无法读取文件大小。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	if (liSize.QuadPart > 1024LL * 1024LL * 64LL)
	{
		CloseHandle(hFile);
		MsgBox(TEXT("文件过大（超过64MB），暂不支持打开。"), TEXT("我的记事本"), mb_OK, mb_IconExclamation);
		return false;
	}

	DWORD dwSize = (DWORD)liSize.QuadPart;
	std::vector<BYTE> bytes;
	bytes.resize(dwSize);

	DWORD dwRead = 0;
	if (dwSize > 0)
	{
		if (!ReadFile(hFile, &bytes[0], dwSize, &dwRead, NULL) || dwRead != dwSize)
		{
			CloseHandle(hFile);
			MsgBox(TEXT("文件读取失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
			return false;
		}
	}
	CloseHandle(hFile);

	tstring sContent = TEXT("");
	bool bDecoded = false;
	if (dwSize >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE)
	{
		int cch = (int)((dwSize - 2) / sizeof(WCHAR));
		std::vector<WCHAR> chars;
		chars.resize(cch + 1);
		if (cch > 0)
		{
			CopyMemory(&chars[0], &bytes[2], cch * sizeof(WCHAR));
		}
		chars[cch] = 0;
		sContent = &chars[0];
		bDecoded = true;
	}
	else if (dwSize >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
	{
		bDecoded = DecodeMultiByteText(CP_UTF8, &bytes[3], (int)dwSize - 3, sContent);
	}
	else
	{
		bDecoded = DecodeMultiByteText(CP_ACP, dwSize > 0 ? &bytes[0] : NULL, (int)dwSize, sContent);
	}

	if (!bDecoded)
	{
		MsgBox(TEXT("文件编码解析失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	g_bInternalUpdating = true;
	form1.Control(ID_txtMain).TextSet(sContent);
	g_bInternalUpdating = false;

	lstrcpyn(g_szCurrentFile, szFilePath, MAX_PATH);
	g_bModified = false;
	UpdateTitle();
	return true;
}

bool SaveTextFile(LPCTSTR szFilePath)
{
	HANDLE hFile = CreateFile(szFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MsgBox(TEXT("文件保存失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	const BYTE bom[2] = { 0xFF, 0xFE };
	DWORD dwWritten = 0;
	if (!WriteFile(hFile, bom, 2, &dwWritten, NULL) || dwWritten != 2)
	{
		CloseHandle(hFile);
		MsgBox(TEXT("写入文件头失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	tstring sText = form1.Control(ID_txtMain).Text();
	DWORD dwTextBytes = (DWORD)(sText.length() * sizeof(WCHAR));
	if (dwTextBytes > 0)
	{
		if (!WriteFile(hFile, sText.c_str(), dwTextBytes, &dwWritten, NULL) || dwWritten != dwTextBytes)
		{
			CloseHandle(hFile);
			MsgBox(TEXT("写入文件内容失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
			return false;
		}
	}

	CloseHandle(hFile);
	lstrcpyn(g_szCurrentFile, szFilePath, MAX_PATH);
	g_bModified = false;
	UpdateTitle();
	return true;
}

bool SaveCurrentFileAs()
{
	TCHAR szFilePath[MAX_PATH];
	if (!SelectSaveFile(szFilePath, MAX_PATH))
	{
		return false;
	}
	return SaveTextFile(szFilePath);
}

bool SaveCurrentFile()
{
	if (g_szCurrentFile[0] == 0)
	{
		return SaveCurrentFileAs();
	}
	return SaveTextFile(g_szCurrentFile);
}

bool QuerySave()
{
	if (!g_bModified)
	{
		return false;
	}

	EDlgBoxCmdID cmd = MsgBox(TEXT("当前内容已修改，是否先保存？"), TEXT("我的记事本"), mb_YesNoCancel, mb_IconExclamation);
	if (cmd == idYes)
	{
		if (!SaveCurrentFile())
		{
			return true;
		}
		return false;
	}
	if (cmd == idCancel)
	{
		return true;
	}
	return false;
}

void form1_Load()
{
	form1.IconSet(IDI_ICON1);
	form1.SetMenuMain(IDR_MENU_MAIN);
	UpdateTitle();
}

void form1_Resize()
{
	int margin = 6;
	int w = form1.ClientWidth() - margin * 2;
	int h = form1.ClientHeight() - margin * 2;
	if (w < 10) w = 10;
	if (h < 10) h = 10;
	form1.Control(ID_txtMain).Move(margin, margin, w, h);
}

void txtMain_Change()
{
	if (g_bInternalUpdating)
	{
		return;
	}
	g_bModified = true;
	UpdateTitle();
}

void form1_QueryUnload(int pbCancel)
{
	if (QuerySave())
	{
		*((int*)pbCancel) = 1;
	}
}

void Menu_Click(int menuID, int bIsFromAcce, int bIsFromSysMenu)
{
	if (menuID == ID_mnuFileNew)
	{
		if (QuerySave()) return;
		g_bInternalUpdating = true;
		form1.Control(ID_txtMain).TextSet(TEXT(""));
		g_bInternalUpdating = false;
		g_szCurrentFile[0] = 0;
		g_bModified = false;
		UpdateTitle();
		return;
	}

	if (menuID == ID_mnuFileOpen)
	{
		if (QuerySave()) return;
		TCHAR szFilePath[MAX_PATH];
		if (SelectOpenFile(szFilePath, MAX_PATH))
		{
			LoadTextFile(szFilePath);
		}
		return;
	}

	if (menuID == ID_mnuFileSave)
	{
		SaveCurrentFile();
		return;
	}

	if (menuID == ID_mnuFileSaveAs)
	{
		SaveCurrentFileAs();
		return;
	}

	if (menuID == ID_mnuFileExit)
	{
		form1.UnLoad();
		return;
	}

	if (menuID == ID_mnuEditCut)
	{
		form1.Control(ID_txtMain).Cut();
		return;
	}

	if (menuID == ID_mnuEditCopy)
	{
		form1.Control(ID_txtMain).Copy();
		return;
	}

	if (menuID == ID_mnuEditPaste)
	{
		form1.Control(ID_txtMain).Paste();
		return;
	}

	if (menuID == ID_mnuEditDelete)
	{
		form1.Control(ID_txtMain).SelTextSet(TEXT(""));
		return;
	}

	if (menuID == ID_mnuEditSelectAll)
	{
		form1.Control(ID_txtMain).SelSet(0, -1);
		return;
	}

	if (menuID == ID_mnuEditTimeDate)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		TCHAR szTimeDate[64];
		wsprintf(szTimeDate, TEXT("%04d-%02d-%02d %02d:%02d:%02d"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		form1.Control(ID_txtMain).SelTextSet(szTimeDate);
		return;
	}

	if (menuID == ID_mnuHelpInnovation)
	{
		MsgBox(
			TEXT("创新点：\r\n")
			TEXT("1. 自动识别 UTF-16/UTF-8/ANSI 文本编码并打开；\r\n")
			TEXT("2. 保存采用 UTF-16 编码，中文兼容性更好；\r\n")
			TEXT("3. 标题栏实时显示“未保存(*)”状态；\r\n")
			TEXT("4. 关闭/新建/打开前自动询问保存，减少误操作。"),
			TEXT("本程序创新点"),
			mb_OK,
			mb_IconInformation
		);
		return;
	}
}

int main()
{
	form1.EventAdd(0, eForm_Load, form1_Load);
	form1.EventAdd(0, eForm_Resize, form1_Resize);
	form1.EventAdd(0, eForm_QueryUnload, form1_QueryUnload);
	form1.EventAdd(0, eMenu_Click, Menu_Click);
	form1.EventAdd(ID_txtMain, eEdit_Change, txtMain_Change);

	form1.Show();
	return 0;
}
