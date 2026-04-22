//////////////////////////////////////////////////////////////////////
// mdlShellExec.h: 运行程序或打开文档
//  
// 支持：需要 BWindows 模块的支持
//////////////////////////////////////////////////////////////////////

#include "Bwindows.h"


// 系统文件夹枚举类型
enum eSESysFolder
{
	sesfMyComputer,		// 我的电脑
	sesfMyDocuments,	// 我的文档
	sesfNetworkPlaces,	// 网上邻居
    sesfRecycleBin		// 回收站
};




// 获得本程序的运行路径（返回值最后有'\'）
// bGet83PathFile=true 时获得8.3短文件名，否则获得长文件名
LPTSTR SEAppPath(bool bGet83PathFile=false);


// 执行一个程序、打开一个文档或打开一个URL
// 若bWithParam=true，则将 sFileOrURL 的第一个空格前看做程序的文件名， _
// 之后看作参数；否则都看作程序文件名，参数为空
//   winShowStyle 表示窗口显示状态（最大化、最小化、隐藏等）
// 用法举例：
//   运行程序 SEShellRun("C:\Windows\System32\mspaint.exe");
//   打开文件夹 SEShellRun("C:\Windows");
//   自动启动关联程序打开文档 SEShellRun("E:\my\abc.txt");
//   打开网页：SEShellRun("http://www.163.com");
//   发送邮件：SEShellRun("mailto:abc@def.com");
bool SEShellRun(LPCTSTR sFileOrURL, 
				bool bWithParam=false, 
				EShowWindowCmd winShowStyle=SW_ShowNormal);


// 打开一个系统文件夹
bool SEOpenSysFolder(eSESysFolder sysFolder, EShowWindowCmd winShowStyle=SW_ShowNormal);

// 打印一个文档
bool SEPrintDocument(LPCTSTR fileToPrint, EShowWindowCmd winShowStyle=SW_ShowNormal);


// 运行一个程序并等待，函数返回进程句柄，失败返回0
// bReturnUntilTerm = true时，只有程序结束后，函数才会返回；
//   =False 函数立即返回一个进程句柄，之后可反复调用
// SETestProcessTerminate 测试进程是否结束
//   或 随时调用 SETerminateProcess 终止进程
// 若 bReturnUntilTerm=false，必须之后再用 SECloseProcessHandle 关闭进程句
//   柄（函数返回值）；而 bReturnUntilTerm=true 时则不必，本函数会自动关闭句柄
HANDLE SERunWaitTerminate(LPCTSTR sExeFileWithParams,
						  bool bReturnUntilTerm = false, 
						  EShowWindowCmd winShowStyle=SW_ShowNormal);


// 测试一个进程是否已经结束
// 如没有该进程或失败也返回 true，只有进程仍在运行才返回 false
// bClosehProcessIfTerm = true 表示如果进程已经结束就关闭句柄 hProcess
// 否则主调程序必须自行调用 SETerminateProcess 关闭句柄
bool SETestProcessTerminate(HANDLE hProcess, bool bClosehProcessIfTerm = true);


// 终止一个进程
// bClosehProcess：是否自动关闭进程句柄
bool SETerminateProcess(HANDLE hProcess, bool bClosehProcess = true);


// 关闭一个进程句柄
void SECloseProcessHandle(HANDLE hProcess);
