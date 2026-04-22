//////////////////////////////////////////////////////////////////////
// mdlOpenSaveDlg.h: 实现浏览文件夹对话框的函数原型及变量常量定义
//  
// 支持：需要 BWindows 的支持
//////////////////////////////////////////////////////////////////////


#include "BWindows.h"



#define B_MAX_PATH MAX_PATH*2			// 扩大一下 MAX_PATH 的大小，以允许容纳更长字符串
// ========================================================================
// 模块级变量及类型定义，这些全局变量只能被本源程序文件使用，不能被其他源程序文件使用
// ========================================================================
static TCHAR mLastPath[B_MAX_PATH];		// 上次显示对话框时用户选择的路径
static TCHAR mSetPathIni[B_MAX_PATH];	// 对话框初始显示时自动选择的路径




// ========================================================================
// 函数声明
// ========================================================================


// 调用 Windows 浏览文件夹对话框 选择文件夹，返回选择的文件夹路径
//   本函数会自动确保返回的、用户选择的路径最后有"\"
//   如用户按“取消”返回 空串""
//   所返回的字符串的空间由本模块用全局 HM 自动开辟、自动管理
// sPrompt 为对话框中显示的提示信息，默认 NULL 时不显示提示
// bOnlyShowFileSystem=true 时只显示文件系统项目，否则还显示 控制面板、回收站 等
// sIniSelPath 为开始自动选择的目录，""为自动选择上次的目录，NULL 为不自动选
LPTSTR BrowPath(HWND		hWndOwner = NULL, 
				LPCTSTR		sPrompt = NULL, 
				bool		bOnlyShowFileSystem = true, 
				LPCTSTR		sIniSelPath = NULL, 
				bool		bBrowseFiles = false, 
				bool		bWithEditCtrol =false);


// SHBrowseForFolder 的回调函数
int __stdcall pdlgBrowseCallback(HWND hWnd, unsigned int uMsg, long lParam, long lpData);

    