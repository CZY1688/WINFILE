//////////////////////////////////////////////////////////////////////
// mdlFileSys.cpp: 文件系统操作函数的实现
// 
// 本模块要使用全局对象 HM 管理动态内存 
//////////////////////////////////////////////////////////////////////



#include "BWindows.h"
#include "mdlFileSys.h"
#include <memory.h>


// 共用空间供字符串函数容错使用：
//（gEmptyTSTR用于兼容Unicode和Ansi；gEmptySTR仅用于Ansi；gEmptyWSTR 仅用于 Unicode）
// 出错时（如指针为0），容错返回空字符串，就返回此空间的内容
// 此空间在 mdlFileSys.h 中未作声明，其他模块不得使用
static TCHAR gEmptyTSTR[1];
static char gEmptySTR[1];
static WCHAR gEmptyWSTR[1];

//////////////////////////////////////////////////////////////////////////
// 自定义文件 函数
//////////////////////////////////////////////////////////////////////////


// 判断一个文件或文件夹是否存在：存在且是文件返回1，存在且是文件夹返回2，不存在返回0
int FMFileExist(LPCTSTR szFile)
{
	bool bIsFolder = false;		// 如此为 true，强制说明：要判断的 szFile 是个目录
	int result=0;				// 本函数的返回值

	// 将要判断的文件名字符串拷贝到 sFile
	TCHAR * sFile = new TCHAR[lstrlen(szFile)+1];
	lstrcpy(sFile, szFile);

	// 如果 sFile 最后一个字符为"\"则认为它是一个目录，
	//	去掉"\"以便测试目录是否存在，并说明 sFile 定是个目录，下面按目录查找其是否存在
	TCHAR * pTailCh = sFile + lstrlen(sFile) - 1;
	if (*pTailCh == '\\') 
	{
		*pTailCh='\0';		// 去掉结尾的 '\\'
		bIsFolder=true;		// 说明 szFile 定是个目录，下面按目录查找其是否存在
		pTailCh--;			// pTailCh 仍指向结尾
	}
		
	// 判断结尾是否为 ':'（或去掉'\\'后的新结尾为':'(原倒数第二个字符)）
	// 如果是，不能用 FindFirstFile 判断根目录是否存在，要用 GetFileAttributes
	if (*pTailCh == ':')
	{
		int r=GetFileAttributes(sFile);		// 如果 GetFileAttributes 失败，返回 -1
		if (r!=-1 && ((r & FILE_ATTRIBUTE_DIRECTORY)!=0) )
			result = 2;		// 说明目录存在（根目录）
		else
			result = 0;		// 目录不存在
	}
	else		// if (*pTailCh == ':')
	{
		// 使用 FindFirstFile 测试 sFile 是否存在
		WIN32_FIND_DATA fd;
		HANDLE hr=FindFirstFile(sFile, &fd);
		if (hr==INVALID_HANDLE_VALUE)
		{
			result = 0;
		}
		else
		{
			FindClose(hr);	// 关闭查找句柄
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				result = 2;
			else
				if (bIsFolder) result = 0; else result = 1;	//去掉'\\'应该是个文件，
											// 但先前有'\\'(blIsFolder=true)说明文件不存在
		}
	}	// end if (*pTailCh == ':')


	delete []sFile;
	return result;
}


LPTSTR FMAddBackSlash(LPCTSTR szPath)
{
	int len = lstrlen(szPath);
	TCHAR *pTailCh = (TCHAR *)(szPath + len - 1);
	if (*pTailCh == '\\') 
	{
		// 仍使用原字符串
		return (TCHAR *)szPath;
	}
	else
	{
		// 创建新的字符串，添加 '\\'，新字符串内存由 HM 管理
		TCHAR *strResult = new TCHAR[len+2];
		HM.AddPtr(strResult);

		lstrcpy(strResult, szPath);
		*(strResult + len) = '\\';
		*(strResult + len + 1) = '\0';

		return strResult;
	}
}


// 列出 tFolder 下面的文件和子文件夹，各文件（夹）字符串的地址分别放在 retFiles 
//   和 retSubFolders 两个指针数组中 （内存空间本函数已开辟并自动管理，主调程序不必干预）
//   两个指针数组下标均从 1 开始，[0] 元素不用
// bReturnEntirePath 是否返回整个路径（加上 tFolder），否则只返回文件和子文件夹名
//  sFilter 指定文件(夹)要查找的条件（带通配符），为 NULL 时默认为"*"，表示所有文件(夹)都列出
// 本函数不递归，要想递归可将返回的子文件夹再循环调用本函数
// 函数返回值表示是否成功
bool FMListFilesAPI( LPCTSTR tFolder, 
					 TCHAR ** &retFiles, 
					 TCHAR ** &retSubFolders, 
					 int * retpFilesCount/*=NULL*/, 
					 int * retpSubFoldersCount/*=NULL*/, 
					 bool bReturnEntirePath/*=true*/, 
					 LPCTSTR sFilter/*=NULL*/ )
{
	const int cArrExpandPer = 100;			//步进式扩增数组每次扩增大小

	if (tFolder==NULL) return false;		// 目标文件夹未给出
	if (lstrlen(tFolder)==0) return false;	// 目标文件夹为 ""

	// 初始化数据
	TCHAR *pFolder=FMAddBackSlash(tFolder);	// 目标文件夹最后确保有 '\'
	int iLengthFolder = lstrlen(pFolder);	// iLengthFolder 为目标文件夹字符串的长度
	WIN32_FIND_DATA WFDFileData;
	HANDLE findHandle;

	memset(&WFDFileData, 0, sizeof(WFDFileData));
	// 开始查找文件，搜索句柄保存到 findHandle
	if (sFilter)
	{
		// 有 Filter，拼接 pFolder + sFilter
		TCHAR * pFolderFilter = new TCHAR [lstrlen(pFolder) + lstrlen(sFilter) + 1];
		HM.AddPtr(pFolderFilter);
		lstrcpy(pFolderFilter, pFolder);
		lstrcat(pFolderFilter, sFilter);
		// 按 pFolder + sFilter 查找文件
		findHandle = FindFirstFile(pFolderFilter, &WFDFileData);
	}
	else
	{
		// 无 Filter，查找 pFolder 中的所有文件
		// 拼接 pFolder + "*"
		TCHAR * pFolderFilter = new TCHAR [lstrlen(pFolder) + 2];
		HM.AddPtr(pFolderFilter);
		lstrcpy(pFolderFilter, pFolder);
		lstrcat(pFolderFilter, TEXT("*"));
		// 按 pFolder + "*" 查找文件
		findHandle = FindFirstFile(pFolderFilter, &WFDFileData);
	}

	// 根据 findHandle 判断，如果无效则返回 false
	if (findHandle == INVALID_HANDLE_VALUE)
	{
		// FindFirstFile 调用失败，搜索句柄无效
		return false;
	}

	// 有效句柄 findHandle
	// 循环查找所有文件(夹)，第一个文件(夹)现在已由 FindFirstFile 找到
	// 文件(夹)信息在 WFDFileData 数据的各成员中
	int iFilesCount=0, iSubFoldersCount=0;
	int uboundFilesArr=-1, uboundSubFoldersArr=-1;	// retFiles[] 数组、retSubFolders[] 数组的上界
	TCHAR *pEachFile=NULL;							// 用于保存每个文件（夹）名字符串的空间地址
	int ret=0;										// 用于 FindNextFile 函数的返回值
	do
	{
		if (lstrcmp(WFDFileData.cFileName, TEXT("."))!=0 && 
			lstrcmp(WFDFileData.cFileName, TEXT(".."))!=0)	// 跳过当前目录及上层目录
		{
			// 合法文件或文件夹名，处理之
			// 获得此文件（夹）名字符串 => pEachFile
			//   即先让新开辟的字符串的空间的地址暂时由 pEachFile 保存，
			//   之后再将该地址存入 retFiles[] 、retSubFolders[] 数组
			if (bReturnEntirePath)
			{
				// 需要返回全路径文件名，拼接“文件路径”+“文件名”数据 => pEachFile
				pEachFile=new TCHAR[iLengthFolder + lstrlen(WFDFileData.cFileName) + 1];
				HM.AddPtr(pEachFile);
				lstrcpy(pEachFile, pFolder);
				lstrcat(pEachFile, WFDFileData.cFileName);
			}
			else
			{
				// 只需要返回文件名，复制“文件名”数据 => pEachFile
				// 必须复制字符串，因下次 WFDFileData.cFileName 中的数据会失效
				pEachFile=new TCHAR[lstrlen(WFDFileData.cFileName) + 1];
				HM.AddPtr(pEachFile);
				lstrcpy(pEachFile, WFDFileData.cFileName);
			}
			
			// 使用位比较来确定 pEachFile 是否代表一个子文件夹
			if (WFDFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// 是子文件夹，添加到 retSubFolders[] 数组中
				iSubFoldersCount++;
				if (iSubFoldersCount > uboundSubFoldersArr)
				{
					// 重定义 retSubFolders[] 数组大小
					Redim(retSubFolders, uboundSubFoldersArr+cArrExpandPer, uboundSubFoldersArr, true);
					uboundSubFoldersArr=uboundSubFoldersArr+cArrExpandPer;
				}
				// 添加新数据：让 retSubFolders[iSubFoldersCount] 记录这个新开辟的空间的地址
				retSubFolders[iSubFoldersCount] = pEachFile;
			}
			else
			{
				// 是文件，添加到 retFiles[] 数组中
				iFilesCount++;
				if (iFilesCount > uboundFilesArr)
				{
					// 重定义 retFiles[] 数组大小
					Redim(retFiles, uboundFilesArr+cArrExpandPer, uboundFilesArr, true);
					uboundFilesArr=uboundFilesArr+cArrExpandPer;
				}
				// 添加新数据：让 retFiles[iFilesCount] 记录这个新开辟的空间的地址
				retFiles[iFilesCount] = pEachFile;
			}

		}	// 跳过当前目录及上层目录
		
       
        // 用 FindNextFile 查找下一个文件
		memset(&WFDFileData, 0, sizeof(WFDFileData));
        ret = FindNextFile(findHandle, &WFDFileData);
	} while(ret!= 0 && ret!=ERROR_NO_MORE_FILES);
		
	// 关闭句柄
	FindClose(findHandle);
		
	// 从参数返回文件（子文件夹）个数
	if (retpFilesCount) *retpFilesCount=iFilesCount;
	if (retpSubFoldersCount) *retpSubFoldersCount=iSubFoldersCount;
	
	// 返回成功
	return true;
}






// 使用 SHFileOperation 操作文件的公用函数
// oper=1: 移动文件, oper=2: 拷贝文件, oper=3: 删除文件, oper=4: 重命名文件
// srcFiles 为以 '\0' 分隔，双 '\0' 结尾的字符串指针
static bool ShFilesOper(char	oper, 
				  	 LPCTSTR	srcFiles, 
					 LPCTSTR	destFolder, 
						bool	bShowProgressDlg/*=true*/, 
						bool	bShowConfirmation/*=true*/,
						bool	bShowErrGui/*=true*/,
						bool	bAllowUndo/*=true*/)
{
	SHFILEOPSTRUCT sfo;
	sfo.wFunc = oper;
	sfo.pFrom = srcFiles;
	sfo.pTo = destFolder;
	sfo.fFlags = 0;

	sfo.fAnyOperationsAborted = 0;
	sfo.hNameMappings = 0;
	sfo.hwnd = 0;
	sfo.lpszProgressTitle = 0;

	if (bAllowUndo) sfo.fFlags |= FOF_ALLOWUNDO;
    if (! bShowProgressDlg) sfo.fFlags |= FOF_SILENT;
    if (! bShowConfirmation) sfo.fFlags |= FOF_NOCONFIRMATION;
    if (! bShowErrGui) sfo.fFlags |= FOF_NOERRORUI;

	return (SHFileOperation(&sfo) == 0);
}



bool FMCopyFiles(LPCTSTR srcOne, 
				 LPCTSTR dest, 
				 bool bShowProgressDlg/*=true*/, 
				 bool bShowConfirmation/*=true*/, 
				 bool bShowErrGui/*=true*/)
{
	// 分配临时空间：双'\0'结尾的源文件字符串的空间
	int iSrcOneLen = lstrlen(srcOne);
	TCHAR * srcDblNULL = new TCHAR[iSrcOneLen + 2];	// +2为最后的 '\0' 空间（双'\0'时指最后一个'\0'）

	// 使用临时空间，使源文件结尾有连续两个 '\0'
	lstrcpy(srcDblNULL, srcOne);
	*(srcDblNULL + iSrcOneLen + 1)='\0';			// 使结尾有连续两个 '\0'

	// 调用 ShFilesOper 执行拷贝文件操作
	bool bRet;
	bRet = ShFilesOper(2, srcDblNULL, dest, bShowProgressDlg, bShowConfirmation, bShowErrGui, true);

	// 删除临时空间
	delete []srcDblNULL;

	// 返回
	return bRet;
}

bool FMCopyFiles(TCHAR * mulFilesSrcArr[], 
				  int	srcArrIndexEnd, 
		      LPCTSTR	dest, 
				 bool   bShowProgressDlg/*=true*/, 
				 bool   bShowConfirmation/*=true*/, 
				 bool   bShowErrGui/*=true*/, 
				 int    srcArrIndexStart/*=1*/ )
{
	// 生成源文件字符串数组组成的双'\0'结尾的字符串
	TCHAR * szSrcDblNULL;
	szSrcDblNULL = Join(mulFilesSrcArr, srcArrIndexEnd, NULL, srcArrIndexStart, true);

	// 调用 ShFilesOper 执行拷贝文件操作
	return ShFilesOper(2, szSrcDblNULL, dest, bShowProgressDlg, bShowConfirmation, bShowErrGui, true);
}

bool FMMoveFiles(LPCTSTR srcOne, 
				 LPCTSTR dest, 
				 bool bShowProgressDlg/*=true*/, 
				 bool bShowConfirmation/*=true*/, 
				 bool bShowErrGui/*=true*/ )
{
	// 分配临时空间：双'\0'结尾的源文件字符串的空间
	int iSrcOneLen = lstrlen(srcOne);
	TCHAR * srcDblNULL = new TCHAR[iSrcOneLen + 2];	// +2为最后的 '\0' 空间（双'\0'时指最后一个'\0'）
	
	// 使用临时空间，使源文件结尾有连续两个 '\0'
	lstrcpy(srcDblNULL, srcOne);
	*(srcDblNULL + iSrcOneLen + 1)='\0';			// 使结尾有连续两个 '\0'
	
	// 调用 ShFilesOper 执行移动文件操作
	bool bRet;
	bRet = ShFilesOper(1, srcDblNULL, dest, bShowProgressDlg, bShowConfirmation, bShowErrGui, true);
	
	// 删除临时空间
	delete []srcDblNULL;
	
	// 返回
	return bRet;
}

bool FMMoveFiles( TCHAR * mulFilesSrcArr[], 
				 int srcArrIndexEnd, 
				 LPCTSTR dest, 
				 bool bShowProgressDlg/*=true*/, 
				 bool bShowConfirmation/*=true*/, 
				 bool bShowErrGui/*=true*/, 
				 int srcArrIndexStart/*=1*/ )
{
	// 生成源文件字符串数组组成的双'\0'结尾的字符串
	TCHAR * szSrcDblNULL;
	szSrcDblNULL = Join(mulFilesSrcArr, srcArrIndexEnd, NULL, srcArrIndexStart, true);
	
	// 调用 ShFilesOper 执行移动文件操作
	return ShFilesOper(1, szSrcDblNULL, dest, bShowProgressDlg, bShowConfirmation, bShowErrGui, true);
}

bool FMDeleteFiles( LPCTSTR srcOne, 
				   bool bMoveToRecycle/*=true*/, 
				   bool bShowProgressDlg/*=true*/, 
				   bool bShowConfirmation/*=true*/, 
				   bool bShowErrGui/*=true*/ )
{
	// 分配临时空间：双'\0'结尾的源文件字符串的空间
	int iSrcOneLen = lstrlen(srcOne);
	TCHAR * srcDblNULL = new TCHAR[iSrcOneLen + 2];	// +2为最后的 '\0' 空间（双'\0'时指最后一个'\0'）
	
	// 使用临时空间，使源文件结尾有连续两个 '\0'
	lstrcpy(srcDblNULL, srcOne);
	*(srcDblNULL + iSrcOneLen + 1)='\0';			// 使结尾有连续两个 '\0'
	
	// 调用 ShFilesOper 执行删除文件操作
	bool bRet;
	bRet = ShFilesOper(3, srcDblNULL, 0, bShowProgressDlg, bShowConfirmation, bShowErrGui, bMoveToRecycle);
	
	// 删除临时空间
	delete []srcDblNULL;
	
	// 返回
	return bRet;	
}

bool FMDeleteFiles(TCHAR * mulFilesSrcArr[], 
				   int srcArrIndexEnd, 
				   bool bMoveToRecycle/*=true*/, 
				   bool bShowProgressDlg/*=true*/, 
				   bool bShowConfirmation/*=true*/, 
				   bool bShowErrGui/*=true*/, 
				   int srcArrIndexStart/*=1*/ )
{
	// 生成源文件字符串数组组成的双'\0'结尾的字符串
	TCHAR * szSrcDblNULL;
	szSrcDblNULL = Join(mulFilesSrcArr, srcArrIndexEnd, NULL, srcArrIndexStart, true);
	
	// 调用 ShFilesOper 执行移动文件操作
	return ShFilesOper(3, szSrcDblNULL, 0, bShowProgressDlg, bShowConfirmation, bShowErrGui, bMoveToRecycle);
}

bool FMRenameFile( LPCTSTR srcOne, 
				   LPCTSTR dest, 
				   bool bShowConfirmation/*=true*/, 
				   bool bShowErrGui/*=true*/, 
				   bool bAllowUndo/*=true*/ )
{
	// 分配临时空间：双'\0'结尾的源文件字符串的空间
	int iSrcOneLen = lstrlen(srcOne);
	TCHAR * srcDblNULL = new TCHAR[iSrcOneLen + 2];	// +2为最后的 '\0' 空间（双'\0'时指最后一个'\0'）
	
	// 使用临时空间，使源文件结尾有连续两个 '\0'
	lstrcpy(srcDblNULL, srcOne);
	*(srcDblNULL + iSrcOneLen + 1)='\0';			// 使结尾有连续两个 '\0'
	
	// 调用 ShFilesOper 执行更改文件名的操作
	bool bRet;
	bRet = ShFilesOper(4, srcDblNULL, dest, true, bShowConfirmation, bShowErrGui, bAllowUndo);
	
	// 删除临时空间
	delete []srcDblNULL;
	
	// 返回
	return bRet;	
}

LPTSTR FMGetSysTempPath()
{
	DWORD iLen=0;

	// 获得所需空间大小
	TCHAR t[1];
	iLen = GetTempPath(1, t);
	if (iLen==0)
	{
		gEmptyTSTR[0]=TEXT('\0');
		return gEmptyTSTR;
	}
	else
	{
		iLen++;
		LPTSTR buff = new TCHAR [iLen+1];
		HM.AddPtr(buff);	// 用 HM 管理动态空间
		if (GetTempPath(iLen, buff)==0)
		{
			gEmptyTSTR[0]=TEXT('\0');
			return gEmptyTSTR;
		}
		else
		{
			return buff;
		}
	}
}

LPTSTR FMGetTempFile( LPCTSTR szFileHeader/*=NULL*/, LPCTSTR szInPath/*=NULL*/ )
{
	// szFileHeader==NULL 时使用"~t"，szInPath==NULL 时使用 FMGetSysTempPath()
	
	LPCTSTR szHeader = szFileHeader;
	LPCTSTR szPath = szInPath;
	if (szFileHeader == NULL) szFileHeader = TEXT("~t");
	if (szPath==NULL) szPath = FMGetSysTempPath();

	LPTSTR buff = new TCHAR [1024];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	buff[0] = TEXT('\0');

	GetTempFileName(szPath, szFileHeader, 0, buff);
    return buff;
}

LPTSTR FMTrimFileName( LPCTSTR szFileName, bool fRemovePath /*= true*/, bool fRemoveExp /*= false*/, LPTSTR *pszRetPath /*= NULL*/, LPTSTR *pszRetExp /*= NULL*/ )
{
	// 设置文件名、路径和扩展名默认返回值：空串 ""
	*gEmptyTSTR = TEXT('\0');
	if (pszRetPath) *pszRetPath = gEmptyTSTR; 
	if (pszRetExp) *pszRetExp = gEmptyTSTR; 
	TCHAR *szFileNameRet = gEmptyTSTR;

	// szFileName 的内容拷贝一份到 szFileBuff
	TCHAR *szFileBuff = new TCHAR [lstrlen(szFileName) + 1];
	HM.AddPtr(szFileBuff);
	_tcscpy(szFileBuff, szFileName);
	TCHAR *p=szFileBuff;

    if (fRemovePath)
	{
        // 除去路径部分
        // 从字符串末尾向前查找最后一个"\"或":"，之前的部分就是路径部分
		while (*p) p++; p--;
		while (p>=szFileBuff)
		{
			if (*p == TEXT('\\') || *p == TEXT(':') ) break;
			p--;
		}
		if (p>=szFileBuff)
		{
			*p = TEXT('\0');			// \ 或 : 的位置设置 \0
			if (pszRetPath) *pszRetPath = szFileBuff;   // 之前部分为路径部分
			szFileNameRet = p+1;	// 将来之后部分返回文件名
		}
		else
		{
			// 未找到 "\"或":"
			// *pszRetPath = ""  // 路径部分保持空串（前面已设置）
			szFileNameRet = szFileBuff;  // 所有将来之后部分返回文件名：return p+1 即 szFileBuff; 
		}
	}
	
	if (fRemoveExp)
	{
		//判断是否为文件夹，如果为文件夹不去掉扩展名（因文件夹无扩展名）
		DWORD r = GetFileAttributes(szFileName);
		if ( r != 0xFFFFFFFF && ((r & FILE_ATTRIBUTE_DIRECTORY) == 0) )	// r=0xFFFFFFFF表示失败；r & FILE_ATTRIBUTE_DIRECTORY) == 0表示不是文件夹
		{
			// 除去扩展名，继续看 szFileNameRet 开始的之后的内容
			p = szFileNameRet;
			while (*p) p++; p--;
			while (p>=szFileNameRet)
			{
				if (*p == TEXT('.') ) break;
				p--;
			}
			if (p>=szFileNameRet)
			{
				// 找到 "."
				*p = TEXT('\0');   // 将来文件名部分继续返回 szFileNameRet
				if (pszRetExp) *pszRetExp = p+1; // 扩展名部分返回 p+1
			}
			else
			{
				// 未找到 "."
				// 扩展名部分保持空串（前面已设置）
				// 将来文件名部分继续返回 szFileNameRet
				;
			}
		}
	}

	return szFileNameRet;
}
