//////////////////////////////////////////////////////////////////////
// mdlFileSys.h: 文件系统操作函数
//  
// 支持：需要 BWindows 模块的支持
//////////////////////////////////////////////////////////////////////



#pragma once

#include "BWindows.h"


// 判断一个文件、文件夹 或 根目录是否存在：
//   存在且是文件返回1，存在且是文件夹返回2，不存在返回0
int FMFileExist(LPCTSTR szFile);	

	
// lpszPath 为路径，函数，返回一个字符串指针，
//   确保最后有一个 '\\'。所返回的字符串可能还是原字符串，
//   可能是由函数自己管理的新开辟的内存的字符串
LPTSTR FMAddBackSlash(LPCTSTR szPath);	


// 将文件名全名（含路径）中的路径部分、扩展名部分都除去，只返回主名部分 _
// fRemovePath=true 就除去路径，fRemoveExp=true 就除去扩展名
// 函数返回除去后的文件主名
// 可以返回所除去的路径和扩展名部分分别到 *pszRetPath（不含最后  \ ）、*pszRetExp（不含 .）
//    若 pszRetPath 和 pszRetExp 为 NULL 不返回
// 如果相应的 fRemovePath、fRemoveExp 为 false，则相应的参数返回""
// 例如：
// 	LPTSTR szFile, szPath, szExp;
//  szFile = FMTrimFileName(TEXT("C:\\a\\789.123.456"), true, true, &szPath, &szExp);
LPTSTR FMTrimFileName(LPCTSTR szFileName, bool fRemovePath = true, bool fRemoveExp = false, LPTSTR *pszRetPath = NULL, LPTSTR *pszRetExp = NULL);


// 列出 tFolder 下面的文件和子文件夹，各文件（夹）字符串的地址分别放在 retFiles 
// 和 retSubFolders 两个指针数组中 （内存空间本函数已开辟并自动管理，主调程序不必干预）
// bReturnEntirePath 是否返回整个路径（加上 tFolder），否则只返回文件和子文件夹名
//  sFilter 指定文件(夹)要查找的条件（带通配符），为 NULL 时默认为"*"，表示所有文件(夹)都列出
// 本函数不递归，要想递归可将返回的子文件夹再循环调用本函数
// 函数返回值表示是否成功
bool FMListFilesAPI(LPCTSTR		tFolder, 
					TCHAR   **	&retFiles, 
					TCHAR   **	&retSubFolders, 
					int     *	retpFilesCount=NULL,
					int     *	retpSubFoldersCount=NULL, 
					bool		bReturnEntirePath=true, 
					LPCTSTR		sFilter=NULL);



// 拷贝文件：一次只拷贝一个文件或文件夹
// 例如：FMCopyFiles("D:\\1.dat","J:\\2.dat");
// 或目标指定为目录：FMCopyFiles("D:\\1.dat","J:\\");
// 或源指定为目录：FMCopyFiles("D:\\文件夹","J:\\");
bool FMCopyFiles(LPCTSTR srcOne,					// 单个文件或文件夹
				 LPCTSTR dest,
				 bool    bShowProgressDlg=true, 
				 bool    bShowConfirmation=true,
				 bool    bShowErrGui=true);

// 拷贝文件：一次同时拷贝多个文件或文件夹到一个指定目录（重载函数）
bool FMCopyFiles(TCHAR * mulFilesSrcArr[],	// 多个源文件字符串指针（数组）
				 int     srcArrIndexEnd,		// 源文件字符串指针（数组）的结束下标（不一定为数组元素个数）
				 LPCTSTR dest,				// 目标文件夹
				 bool    bShowProgressDlg=true, 
				 bool    bShowConfirmation=true,
				 bool    bShowErrGui=true,
				 int     srcArrIndexStart=1); // 源文件字符串指针（数组）的起始下标（默认不使用 [0] 的内容）


// 移动文件：一次只移动一个文件或文件夹
bool FMMoveFiles(LPCTSTR srcOne,				// 单个文件或文件夹
				 LPCTSTR dest,
				 bool    bShowProgressDlg=true, 
				 bool    bShowConfirmation=true,
				 bool    bShowErrGui=true);

// 拷贝文件：一次同时移动多个文件或文件夹到一个指定目录（重载函数）
bool FMMoveFiles(TCHAR * mulFilesSrcArr[],		// 多个源文件字符串指针（数组）
				 int     srcArrIndexEnd,		// 源文件字符串指针（数组）的结束下标（不一定为数组元素个数）
				 LPCTSTR dest,					// 目标文件夹
				 bool    bShowProgressDlg=true, 
				 bool    bShowConfirmation=true,
				 bool    bShowErrGui=true,
				 int     srcArrIndexStart=1);	// 源文件字符串指针（数组）的起始下标（默认不使用 [0] 的内容）

// 删除文件：一次只删除一个文件或文件夹
bool FMDeleteFiles(LPCTSTR srcOne,				// 单个文件或文件夹
				   bool    bMoveToRecycle=true, 
				   bool    bShowProgressDlg=true, 
				   bool    bShowConfirmation=true,
				   bool    bShowErrGui=true);

// 删除文件：一次同时删除多个文件或文件夹（重载函数）
bool FMDeleteFiles(TCHAR * mulFilesSrcArr[],		// 多个源文件字符串指针（数组）
				   int     srcArrIndexEnd,			// 源文件字符串指针（数组）的结束下标（不一定为数组元素个数）
				   bool    bMoveToRecycle=true, 
				   bool    bShowProgressDlg=true, 
				   bool    bShowConfirmation=true,
				   bool    bShowErrGui=true,
				   int     srcArrIndexStart=1);		// 源文件字符串指针（数组）的起始下标（默认不使用 [0] 的内容）

// 重命名一个文件
bool FMRenameFile(LPCTSTR srcOne, 
				  LPCTSTR dest,
				  bool    bShowConfirmation=true,
				  bool    bShowErrGui=true, 
				  bool    bAllowUndo=true);


// 使用 SHFileOperation 操作文件的公用函数
static bool ShFilesOper(char   oper,			// oper=1: 移动文件, oper=2: 拷贝文件, oper=3: 删除文件, oper=4: 重命名文件
						LPCTSTR srcFiles, 
						LPCTSTR destFolder, 
						bool    bShowProgressDlg=true, 
						bool    bShowConfirmation=true,
						bool    bShowErrGui=true,
						bool    bAllowUndo=true);



// 返回临时文件夹字符串首地址（结尾有 \），字符串空间自动开辟、由 HM 自动管理
LPTSTR FMGetSysTempPath();

// 创建一个临时文件，返回临时文件路径文件名字符串首地址，字符串空间自动开辟、由 HM 自动管理
// szFileHeader==NULL 时使用"~t"，szInPath==NULL 时使用 FMGetSysTempPath()
LPTSTR FMGetTempFile(LPCTSTR szFileHeader=NULL, LPCTSTR szInPath=NULL);