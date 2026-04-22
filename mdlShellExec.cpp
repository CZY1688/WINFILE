//////////////////////////////////////////////////////////////////////
// mdlShellExec.cpp: 运行程序或打开文档的函数实现
//  
//////////////////////////////////////////////////////////////////////

#include "mdlShellExec.h"
#include <string.h>
#include <memory.h>
#include <tchar.h>


// 只能本模块使用的全局变量
static TCHAR m_AppPath[MAX_PATH+1];	// 保存本程序运行目录
static CBHeapMemory mHM;			// 保存本模块中所有以 new 开辟的
									//	内存指针，该对象析构时可自动 delete 这些内存


// 获得本程序的运行路径（返回值最后有'\'）
// bGet83PathFile=true 时获得8.3短文件名，否则获得长文件名
LPTSTR SEAppPath(bool bGet83PathFile/*=false*/)
{
	// 获得应用程序文件名（包含全路径）
	GetModuleFileName(0, m_AppPath, MAX_PATH); 

	// 获得8.3格式短文件名
	if (bGet83PathFile)
	{
		TCHAR temp[MAX_PATH+1];
		GetShortPathName(m_AppPath, temp, MAX_PATH);
		_tcscpy(m_AppPath, temp);  // 再将短文件名拷回 m_AppPath
	}
	
	// 截取路径
	// 查找最后一个 '\'
	TCHAR * ptrLastSlash=_tcsrchr(m_AppPath,'\\');  
	// 最后一个 '\' 的下一个位置设置为 0
	if (ptrLastSlash) *(ptrLastSlash+1)='\0'; 

	// 返回
	return m_AppPath;
}




// 执行一个程序、打开一个文档或打开一个URL
// 若bWithParam=true，则将 sFileOrURL 的第一个空格前看做程序的文件名， _
// 之后看作参数；否则都看作程序文件名，参数为空
//   winShowStyle 表示窗口显示状态（最大化、最小化、隐藏等）
// 用法举例：
//   运行程序 SEShellRun("C:\\Windows\\System32\\mspaint.exe");
//   打开文件夹 SEShellRun("C:\\Windows");
//   自动启动关联程序打开文档 SEShellRun("E:\\my\\abc.txt");
//   打开网页：SEShellRun("http://www.163.com");
//   发送邮件：SEShellRun("mailto:zhni2011@163.com");
bool SEShellRun( LPCTSTR sFileOrURL, bool bWithParam/*=false*/, EShowWindowCmd winShowStyle/*=SW_ShowNormal*/ )
{
	TCHAR * sFileOrExe = (TCHAR *)sFileOrURL;	// 外部程序名的执行部分
	TCHAR * sParam = 0;				// 外部程序的参数
	bool ret;						// 返回值

	if (bWithParam)
	{
		// 分出 sFileOrURL 中的程序文件名和参数
		TCHAR * sFileOrExe = new TCHAR[_tcslen(sFileOrURL)+1];
		_tcscpy(sFileOrExe, sFileOrURL);
		TCHAR * ptrBlank=_tcschr(sFileOrExe, 32);	// 找到第一个空格的位置
		if (ptrBlank) // 找到了空格
		{
			*ptrBlank='\0';
			sParam=(ptrBlank+1);
		}
		ret= (INT)ShellExecute(0, TEXT("open"), sFileOrExe, 
		    sParam, 0, (INT)winShowStyle) > 32;
		delete []sFileOrExe;
	}
	else
	{
		ret= (INT)ShellExecute(0, TEXT("open"), sFileOrExe, 
			sParam, 0, (INT)winShowStyle) > 32;
	}
	
	return ret;
}


// 打开一个系统文件夹
bool SEOpenSysFolder( eSESysFolder sysFolder, EShowWindowCmd winShowStyle/*=SW_ShowNormal*/ )
{
	switch(sysFolder)
	{
	case sesfRecycleBin:
		// 打开 回收站
		return SEShellRun(
		  TEXT("explorer.exe ::{645FF040-5081-101B-9F08-00AA002F954E}"), 
		  true, winShowStyle);
		break;
	case sesfMyComputer:
		// 打开 我的电脑
		return SEShellRun(
		  TEXT("explorer.exe ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), 
		  true, winShowStyle);
		break;
	case sesfNetworkPlaces:
		// 打开 网上邻居
		return SEShellRun(
		  TEXT("explorer.exe ::{208D2C60-3AEA-1069-A2D7-08002B30309D}"), 
		  true, winShowStyle);
		break;
	case sesfMyDocuments:
		// 打开 我的文档
		return SEShellRun(TEXT("explorer.exe"), true, winShowStyle);
		break;
	default:
		return 0;
		break;
	}
}


// 打印一个文档
bool SEPrintDocument( LPCTSTR fileToPrint, EShowWindowCmd winShowStyle/*=SW_ShowNormal*/ )
{
	return (INT)ShellExecute(0, TEXT("print"), fileToPrint, 
      0, 0, (INT)winShowStyle) > 32;
}


// 运行一个程序并等待，函数返回进程句柄，失败返回0
// bReturnUntilTerm = true时，只有程序结束后，函数才会返回；
//   =False 函数立即返回一个进程句柄，之后可反复调用
// SETestProcessTerminate 测试进程是否结束
//   或 随时调用 SETerminateProcess 终止进程
// 若 bReturnUntilTerm=false，必须之后再用 SECloseProcessHandle 关闭进程句
//   柄（函数返回值）；而 bReturnUntilTerm=true 时则不必，本函数会自动关闭句柄
HANDLE SERunWaitTerminate( LPCTSTR sExeFileWithParams, bool bReturnUntilTerm /*= false*/, EShowWindowCmd winShowStyle/*=SW_ShowNormal*/ )
{
	long ret;
    STARTUPINFO sinfo;
    PROCESS_INFORMATION pinfo;

	memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);
    sinfo.dwFlags = STARTF_USESHOWWINDOW;
    sinfo.wShowWindow = winShowStyle;

    // -------创建进程
    ret = CreateProcess(0, (LPTSTR)sExeFileWithParams, 0, 0,  true, 
      NORMAL_PRIORITY_CLASS, 0, 0, &sinfo, &pinfo);
    if (ret==0)  // 创建进程失败
	{
		CloseHandle(pinfo.hThread);
		CloseHandle(pinfo.hProcess);
		return 0;
	}
    
    // 等待进程初始化完毕
    WaitForInputIdle(pinfo.hProcess, INFINITE);

    // 关闭线程句柄
    CloseHandle(pinfo.hThread);
    
    if(bReturnUntilTerm)
	{
		// 等待进程结束后函数再返回
		ret = WaitForSingleObject(pinfo.hProcess, INFINITE);
		if (ret == WAIT_FAILED)
		{
			CloseHandle(pinfo.hThread);
			CloseHandle(pinfo.hProcess);
			return 0;
		}

		// 关闭句柄 pinfo.hProcess
		CloseHandle(pinfo.hProcess);

		// 返回
		return pinfo.hProcess;
	}
	else
	{
		// 函数立即返回
		return pinfo.hProcess;
	}
}


// 测试一个进程是否已经结束
// 如没有该进程或失败也返回 true，只有进程仍在运行才返回 false
// bClosehProcessIfTerm = true 表示如果进程已经结束就关闭句柄 hProcess
// 否则主调程序必须自行调用 SETerminateProcess 关闭句柄
bool SETestProcessTerminate( HANDLE hProcess, bool bClosehProcessIfTerm /*= true*/ )
{
	DWORD ret = WaitForSingleObject(hProcess, 0);
	switch(ret)
	{
	case WAIT_TIMEOUT:
		return false;
		break;
	case WAIT_FAILED:
		return true;
		break;
	default:
		// 关闭进程句柄（如果参数指定自动关闭的话）
		if (bClosehProcessIfTerm) CloseHandle (hProcess);
		return true;
		break;
	}

}


// 终止一个进程
// bClosehProcess：是否自动关闭进程句柄
bool SETerminateProcess( HANDLE hProcess, bool bClosehProcess /*= true*/ )
{
	if (TerminateProcess(hProcess, 0))
	{
		if (bClosehProcess) CloseHandle(hProcess);
		return true;
	}
	else
	{
		return false;
	}
}


// 关闭一个进程句柄
void SECloseProcessHandle( HANDLE hProcess )
{
	CloseHandle(hProcess);
}






