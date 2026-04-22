//////////////////////////////////////////////////////////////////////
// mdlOpenSaveDlg.cpp: 实现浏览文件夹对话框
//
//////////////////////////////////////////////////////////////////////

#include "mdlPathDlg.h"
#include <tchar.h>
#include <shlobj.h>
#include <memory.h>
#include <string.h>


// 调用 Windows 浏览文件夹对话框 选择文件夹，返回选择的文件夹路径
//   本函数会自动确保返回的、用户选择的路径最后有"\"
//   如用户按“取消”返回 空串""
//   所返回的字符串的空间由本模块用全局 HM 自动开辟、自动管理
// sPrompt 为对话框中显示的提示信息，默认 NULL 时不显示提示
// bOnlyShowFileSystem=true 时只显示文件系统项目，否则还显示 控制面板、回收站 等
// sIniSelPath 为开始自动选择的目录，""为自动选择上次的目录，NULL 为不自动选
LPTSTR BrowPath( HWND		hWndOwner /*= NULL*/, 
				 LPCTSTR	sPrompt /*= NULL*/, 
				 bool		bOnlyShowFileSystem /*= true*/, 
				 LPCTSTR	sIniSelPath /*= NULL*/, 
				 bool	bBrowseFiles /*= false*/, 
				 bool	bWithEditCtrol /*=false*/ )
{
	BROWSEINFO bi;
	TCHAR * szRetPath = new TCHAR [B_MAX_PATH];	// 开辟返回路径的字符串空间
	HM.AddPtr(szRetPath);	// 用 HM 管理返回路径的字符串空间

	memset(&bi, 0, sizeof(bi));
    bi.hwndOwner = hWndOwner;
	bi.lpszTitle = sPrompt;
	bi.pszDisplayName = szRetPath;
    bi.lpfn = pdlgBrowseCallback;	// 设置回调函数为 pdlgBrowseCallback
    
	// 设置 bi.ulFlags
	bi.ulFlags = 0;
	if (bOnlyShowFileSystem) bi.ulFlags |= BIF_RETURNONLYFSDIRS;
	if (bBrowseFiles) bi.ulFlags |= BIF_BROWSEINCLUDEFILES;
	if (bWithEditCtrol) bi.ulFlags |= BIF_EDITBOX;
	
	// 对话框初始显示时自动选择的路径
	if (sIniSelPath == NULL)
		*mSetPathIni='\0';					// pdlgBrowseCallback 回调函数中将不会选择初始文件夹
	else if (_tcslen(sIniSelPath)==0 && _tcslen(sIniSelPath)<B_MAX_PATH)
		_tcscpy(mSetPathIni, mLastPath);		// pdlgBrowseCallback 回调函数中将自动选择 mLastPath 文件夹
	else
		_tcscpy(mSetPathIni, sIniSelPath);	// pdlgBrowseCallback 回调函数中将自动选择 sIniSelPath 文件夹
	
	// 显示浏览文件夹对话框
	LPITEMIDLIST retID= SHBrowseForFolder(&bi);
	if (! retID)
	{
		// 调用 SHBrowseForFolder 函数失败，返回空字符串
		*szRetPath='\0';
		return szRetPath;
	}
	else
	{
		// 调用 SHBrowseForFolder 函数成功，将路径存入 szRetPath[] 
		//   并返回 szRetPath 的首地址
		// retID 为被选文件夹的项目标识符列表（Item Identifier List）的地址
		// 通过 SHGetPathFromIDList 函数获得此标识符列表所代表的文件夹路径名
		SHGetPathFromIDList(retID, szRetPath);

		// 确保最后有“\”
		if (_tcslen(szRetPath))
		{
			TCHAR *pTail=szRetPath+_tcslen(szRetPath)-1;	// szRetPath 最后一个字符的地址
			if (*pTail!='\\') {*(pTail+1)='\\';*(pTail+2)='\0';}
		}

		// 记录到 mLastPath
		_tcscpy(mLastPath, szRetPath);
		
		// 返回路径
		return szRetPath;
	}
}


// SHBrowseForFolder 的回调函数
int __stdcall pdlgBrowseCallback( HWND hWnd, unsigned int uMsg, long lParam, long lpData )
{
	if (uMsg == BFFM_INITIALIZED)	// 其他消息还有：BFFM_SELCHANGED  BFFM_VALIDATEFAILED
	{
		// 对话框窗口初始化完毕
		if (mSetPathIni)
			if (_tcslen(mSetPathIni))
			{
				// 发送 BFFM_SETSELECTION 消息，自动选择文件夹 mSetPathIni
				// wParam为非0（True）时，lParam参数为文件夹路径（mSetPathIni）
				SendMessage(hWnd, BFFM_SETSELECTION, 1, (long)mSetPathIni);
			}
	}

	return 0;
}
