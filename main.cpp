#include "resource.h"
#include "BForm.h"
#include <commdlg.h>
#include <stdio.h>
#include <string>
#include <vector>
#pragma comment(lib, "comdlg32.lib")

CBForm form1(ID_form1);
CBForm frmInputEnc(ID_frmInputEnc);

TCHAR g_szCurrentFile[MAX_PATH] = TEXT("");
bool g_bModified = false;
bool g_bInternalUpdating = false;
bool g_bEncInputConfirmed = false;
std::string g_encryptKeyAnsi;
const int cEditorMargin = 6;
const int cMinEditorSize = 10;
const LONGLONG cMaxOpenFileSize = 1024LL * 1024LL * 64LL;
const COLORREF cNoTransparencyKey = (COLORREF)0xFFFFFFFF;
const int cOpacityNoLayered = -1;

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

	if (g_bModified) form1.TextAdd(TEXT(" *"));
}

bool ConvertWideToAnsi(LPCTSTR textWide, std::string& outAnsi)
{
#ifdef UNICODE
	int need = WideCharToMultiByte(CP_ACP, 0, textWide, -1, NULL, 0, NULL, NULL);
	if (need <= 0) return false;
	std::vector<char> buff((size_t)need);
	if (WideCharToMultiByte(CP_ACP, 0, textWide, -1, &buff[0], need, NULL, NULL) <= 0) return false;
	outAnsi.assign(&buff[0]);
	return true;
#else
	outAnsi = textWide ? textWide : "";
	return true;
#endif
}

void XorBytes(std::vector<BYTE>& bytes, const std::string& key)
{
	// 仅用于教学演示：异或算法安全性有限，不适合保护敏感数据。
	if (key.empty() || bytes.empty()) return;
	size_t keyLen = key.length();
	for (size_t i = 0; i < bytes.size(); ++i)
	{
		bytes[i] = (BYTE)(bytes[i] ^ (BYTE)key[i % keyLen]);
	}
}

bool DecodeMultiByteText(UINT codePage, const BYTE* pBytes, int cbBytes, tstring& textOut)
{
	if (cbBytes <= 0)
	{
		textOut = TEXT("");
		return true;
	}

	int cchNeed = MultiByteToWideChar(codePage, 0, (LPCSTR)pBytes, cbBytes, NULL, 0);
	if (cchNeed <= 0) return false;

	std::vector<WCHAR> chars((size_t)cchNeed + 1);
	if (MultiByteToWideChar(codePage, 0, (LPCSTR)pBytes, cbBytes, &chars[0], cchNeed) <= 0) return false;
	chars[cchNeed] = 0;
	textOut = &chars[0];
	return true;
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
		lstrcpyn(szFilePath, g_szCurrentFile, cchFilePath);
	else
		szFilePath[0] = 0;

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

bool InputEncrypt()
{
	g_bEncInputConfirmed = false;
	frmInputEnc.Load();
	frmInputEnc.Move(
		form1.Left() + (form1.Width() - frmInputEnc.Width()) / 2,
		form1.Top() + (form1.Height() - frmInputEnc.Height()) / 2
	);
	frmInputEnc.Control(ID_txtEnc).TextSet(TEXT(""));
	frmInputEnc.Show(1, form1.hWnd());
	return g_bEncInputConfirmed;
}

bool LoadTextFile(LPCTSTR szFilePath)
{
	pApp->MousePointerGlobalSet(IDC_Wait);
	HANDLE hFile = CreateFile(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		pApp->MousePointerGlobalSet(0);
		MsgBox(TEXT("文件打开失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	LARGE_INTEGER liSize;
	if (!GetFileSizeEx(hFile, &liSize))
	{
		CloseHandle(hFile);
		pApp->MousePointerGlobalSet(0);
		MsgBox(TEXT("无法读取文件大小。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	if (liSize.QuadPart > cMaxOpenFileSize)
	{
		CloseHandle(hFile);
		pApp->MousePointerGlobalSet(0);
		MsgBox(TEXT("文件过大（超过64MB），暂不支持打开。"), TEXT("我的记事本"), mb_OK, mb_IconExclamation);
		return false;
	}

	DWORD dwSize = (DWORD)liSize.QuadPart;
	std::vector<BYTE> bytes((size_t)dwSize);
	DWORD dwRead = 0;
	if (dwSize > 0)
	{
		if (!ReadFile(hFile, &bytes[0], dwSize, &dwRead, NULL) || dwRead != dwSize)
		{
			CloseHandle(hFile);
			pApp->MousePointerGlobalSet(0);
			MsgBox(TEXT("文件读取失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
			return false;
		}
	}
	CloseHandle(hFile);

	XorBytes(bytes, g_encryptKeyAnsi);

	tstring sContent(TEXT(""));
	bool bDecoded = false;
	if (dwSize >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE)
	{
		int cch = (int)((dwSize - 2) / sizeof(WCHAR));
		std::vector<WCHAR> chars((size_t)cch + 1);
		if (cch > 0)
		{
			CopyMemory(&chars[0], &bytes[2], (size_t)cch * sizeof(WCHAR));
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
		pApp->MousePointerGlobalSet(0);
		MsgBox(TEXT("文件解析失败。可能原因：文件已加密且密码错误，或文件编码不支持。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	g_bInternalUpdating = true;
	form1.Control(ID_txtMain).TextSet(sContent);
	g_bInternalUpdating = false;

	lstrcpyn(g_szCurrentFile, szFilePath, MAX_PATH);
	g_bModified = false;
	UpdateTitle();
	pApp->MousePointerGlobalSet(0);
	return true;
}

bool SaveTextFile(LPCTSTR szFilePath)
{
	if (lstrlen(szFilePath) >= MAX_PATH - 4)
	{
		MsgBox(TEXT("文件路径过长，无法保存。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	TCHAR szTempPath[MAX_PATH + 8];
	lstrcpyn(szTempPath, szFilePath, _countof(szTempPath));
	lstrcat(szTempPath, TEXT(".tmp"));

	pApp->MousePointerGlobalSet(IDC_Wait);
	HANDLE hFile = CreateFile(szTempPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		pApp->MousePointerGlobalSet(0);
		MsgBox(TEXT("文件保存失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	tstring sText = form1.Control(ID_txtMain).Text();
	DWORD textBytes = (DWORD)(sText.length() * sizeof(WCHAR));
	std::vector<BYTE> bytes;
	bytes.reserve((size_t)textBytes + 2);
	bytes.push_back(0xFF);
	bytes.push_back(0xFE);
	if (textBytes > 0)
	{
		BYTE* pText = (BYTE*)sText.c_str();
		bytes.insert(bytes.end(), pText, pText + textBytes);
	}

	XorBytes(bytes, g_encryptKeyAnsi);

	DWORD dwWritten = 0;
	DWORD dwToWrite = (DWORD)bytes.size();
	bool ok = (dwToWrite == 0) || (WriteFile(hFile, &bytes[0], dwToWrite, &dwWritten, NULL) && dwWritten == dwToWrite);
	CloseHandle(hFile);
	pApp->MousePointerGlobalSet(0);

	if (!ok)
	{
		DeleteFile(szTempPath);
		MsgBox(TEXT("写入文件失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	if (!MoveFileEx(szTempPath, szFilePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
	{
		DeleteFile(szTempPath);
		MsgBox(TEXT("替换目标文件失败。"), TEXT("我的记事本"), mb_OK, mb_IconError);
		return false;
	}

	lstrcpyn(g_szCurrentFile, szFilePath, MAX_PATH);
	g_bModified = false;
	UpdateTitle();
	return true;
}

bool SaveCurrentFileAs()
{
	TCHAR szFilePath[MAX_PATH];
	if (!SelectSaveFile(szFilePath, MAX_PATH)) return false;
	if (!InputEncrypt()) return false;
	return SaveTextFile(szFilePath);
}

bool SaveCurrentFile()
{
	if (g_szCurrentFile[0] == 0) return SaveCurrentFileAs();
	return SaveTextFile(g_szCurrentFile);
}

bool ShouldCancelByUnsavedChanges()
{
	if (!g_bModified) return false;

	EDlgBoxCmdID cmd = MsgBox(TEXT("当前内容已修改，是否先保存？"), TEXT("我的记事本"), mb_YesNoCancel, mb_IconExclamation);
	if (cmd == idYes) return !SaveCurrentFile();
	if (cmd == idCancel) return true;
	return false;
}

void form1_Load()
{
	g_szCurrentFile[0] = 0;
	g_bModified = false;
	g_encryptKeyAnsi.clear();
	form1.IconSet(IDI_ICON1);
	form1.SetMenuMain(IDR_MENU_MAIN);
	form1.Control(ID_txtMain).MousePointerSet(IDC_IBEAM);
	UpdateTitle();
}

void form1_Resize()
{
	int w = form1.ClientWidth() - cEditorMargin * 2;
	int h = form1.ClientHeight() - cEditorMargin * 2;
	if (w < cMinEditorSize) w = cMinEditorSize;
	if (h < cMinEditorSize) h = cMinEditorSize;
	form1.Control(ID_txtMain).Move(cEditorMargin, cEditorMargin, w, h);
}

void txtMain_Change()
{
	if (g_bInternalUpdating) return;
	g_bModified = true;
	UpdateTitle();
}

void txtMain_FilesDrop(int ptrArrFiles, int count, int x, int y)
{
	TCHAR** files = (TCHAR**)ptrArrFiles;
	if (count <= 0 || files == NULL) return;
	if (ShouldCancelByUnsavedChanges()) return;
	if (!InputEncrypt()) return;
	LoadTextFile(files[0]);
}

void cmdOK_Click()
{
	tstring inputText = frmInputEnc.Control(ID_txtEnc).Text();
	if (inputText.empty())
	{
		g_encryptKeyAnsi.clear();
	}
	else
	{
		std::string keyTemp;
		if (ConvertWideToAnsi(inputText.c_str(), keyTemp))
			g_encryptKeyAnsi = keyTemp;
		else
			g_encryptKeyAnsi.clear();
	}
	g_bEncInputConfirmed = true;
	frmInputEnc.UnLoad();
}

void cmdCancel_Click()
{
	g_bEncInputConfirmed = false;
	frmInputEnc.UnLoad();
}

void form1_QueryUnload(int pbCancel)
{
	if (ShouldCancelByUnsavedChanges())
	{
		int* pCancel = (int*)(INT_PTR)pbCancel;
		if (pCancel != NULL) *pCancel = 1;
	}
}

void Menu_Click(int menuID, int bIsFromAcce, int bIsFromSysMenu)
{
	if (menuID == ID_mnuFileNew)
	{
		if (ShouldCancelByUnsavedChanges()) return;
		g_bInternalUpdating = true;
		form1.Control(ID_txtMain).TextSet(TEXT(""));
		g_bInternalUpdating = false;
		g_szCurrentFile[0] = 0;
		g_bModified = false;
		g_encryptKeyAnsi.clear();
		UpdateTitle();
		return;
	}

	if (menuID == ID_mnuFileOpen)
	{
		if (ShouldCancelByUnsavedChanges()) return;
		TCHAR szFilePath[MAX_PATH];
		if (SelectOpenFile(szFilePath, MAX_PATH))
		{
			if (InputEncrypt()) LoadTextFile(szFilePath);
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

	if (menuID == ID_mnuEditCut) { form1.Control(ID_txtMain).Cut(); return; }
	if (menuID == ID_mnuEditCopy) { form1.Control(ID_txtMain).Copy(); return; }
	if (menuID == ID_mnuEditPaste) { form1.Control(ID_txtMain).Paste(); return; }
	if (menuID == ID_mnuEditDelete) { form1.Control(ID_txtMain).SelTextSet(TEXT("")); return; }
	if (menuID == ID_mnuEditSelectAll) { form1.Control(ID_txtMain).SelSet(0, -1); return; }

	if (menuID == ID_mnuEditTimeDate)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		TCHAR szTimeDate[64];
		swprintf_s(szTimeDate, _countof(szTimeDate), TEXT("%04d-%02d-%02d %02d:%02d:%02d"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		form1.Control(ID_txtMain).SelTextSet(szTimeDate);
		return;
	}

	if (menuID == ID_mnuViewOpacity)
	{
		form1.TransparencyKeySet(cNoTransparencyKey);
		form1.OpacitySet(128);
		return;
	}

	if (menuID == ID_mnuViewPunchWhite)
	{
		form1.OpacitySet(cOpacityNoLayered);
		form1.TransparencyKeySet(RGB(255, 255, 255));
		return;
	}

	if (menuID == ID_mnuViewNormal)
	{
		form1.TransparencyKeySet(cNoTransparencyKey);
		form1.OpacitySet(255);
		return;
	}

	if (menuID == ID_mnuHelpInnovation)
	{
		MsgBox(
			TEXT("创新点：\r\n")
			TEXT("1. 文件可选异或加密保存和解密打开；\r\n")
			TEXT("2. 支持拖放文件打开；\r\n")
			TEXT("3. 关闭/新建/打开前自动询问保存；\r\n")
			TEXT("4. 支持窗体透明度与透明色挖空效果。"),
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
	form1.EventAdd(ID_txtMain, eFilesDrop, txtMain_FilesDrop);

	frmInputEnc.EventAdd(ID_cmdOK, eCommandButton_Click, cmdOK_Click);
	frmInputEnc.EventAdd(ID_cmdCancel, eCommandButton_Click, cmdCancel_Click);

	form1.Show();
	return 0;
}
