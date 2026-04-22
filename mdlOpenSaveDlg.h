//////////////////////////////////////////////////////////////////////
// mdlOpenSaveDlg.h: 实现打开、保存对话框的模块函数原型及变量常量定义
// 支持：
//     需要 BWindows、BHashStrK 模块的支持
//////////////////////////////////////////////////////////////////////

#include "BWindows.h"

// ========================================================================
// 常量定义
// ========================================================================

#define mc_strFilterAllFile TEXT("|所有文件(*.*)|*.*")		// 默认所有文件的 Filter 部分，因要直接添加到 Filter 的结尾部分，开头必须有"|"
#define mc_strFilterTxtFile TEXT("文本文件(*.txt)|*.txt")	// 文本文件的 Filter 部分，因要直接使用到 Filter 的开头部分，开头不得有"|"
#define mcMaxFileLenSingle 2048		// 预留缓冲区大小  单选文件时用此大小的缓冲区存储单个文件名及路径，保存时也用此存储文件名及路径
#define mcMaxFileLenMul 63488		// 预留缓冲区大小  打开对话框的多选文件时用此大小的缓冲区存储所有文件名字符串



// ========================================================================
// 函数声明
// ========================================================================


// 设定对话框的文件类型 Filter ("|"分隔); 开辟字符串空间，将新的 Filter 字符串存入此空间，并让 mpStrFilter 指向它
void OsdSetFilter(LPCTSTR sFilter=NULL, bool bAddAllFile=true, LPCTSTR sAsRegKey=NULL);

// 打开“打开文件”对话框
// bAllowMulSel=false 时，函数返回文件名（字符串空间由本模块管理），出错或被按取消返回 ""（非NULL）；
// bAllowMulSel=true 时，函数返回文件路径（字符串空间由本模块管理），并设置 mMulSelFiles
// bRetReadOnly指向一个 bool 型变量的指针，*b=true 时显示“以只读方式打开”复选框，并由此参数返回 
//   用户是否勾选了“以只读方式打开”复选框；*b=false 或 =NULL 不显示“以只读方式打开”复选框
// 若给出 sAsRegedKey 显示已登记为 sAsRegedKey 的 Key 的对话框，否则显示一个“新的”对话框
LPTSTR OsdOpenDlg(HWND hWnd=0, LPCTSTR sDlgTitle=NULL, LPCTSTR sAsRegedKey=NULL, bool bAllowMulSel=false, bool *bRetReadOnly=NULL);

// 打开“保存文件”对话框
// sDefaultFileName 表示自动填充到对话框的默认文件名
// 若给出 sAsRegedKey 显示已登记为 sAsRegedKey 的 Key 的对话框，否则显示一个“新的”对话框
LPTSTR OsdSaveDlg(HWND hWnd=0, LPCTSTR sDefaultFileName=NULL, LPCTSTR sDlgTitle=NULL, LPCTSTR sAsRegedKey=NULL);


// 获得最近一次打开对话框选择的多选文件，将由本类对象创建动态指针数组 pptrMulFiles
//   该数组下标从 1 开始，空间 [0] 不用
//   指针数组的首地址由参数返回。本类对象管理一切动态空间
//   多选文件的个数（也是动态数组的个数）由函数返回，每个数组元素分别指向一个字符串
//   空间表示每个选择的文件（只有文件名部分，无路径）
// 例如：
//	char **mul;
//	int fileCount, i;
//  fileCount=OSDMulSelFiles(mul);
//  for(i=1; i<=fileCount; i++)
//		cout<<mul[i]<<endl;
int OSDMulSelFiles(char ** &pptrMulFiles);

// 返回最近一次设置的对话框的文件类型 Filter ("|"分隔)
LPTSTR OsdLastFilter();

// 返回最近一次选择的文件名(多选文件时为路径)
// 返回时本类对象会新建一个字符串空间，返回新空间地址；该空间由本类释放
LPTSTR OSDFilePath();

// 设置 mFilePath 作为默认文件名
void OSDSetInitFilePath(LPCTSTR newInitFilePath);

// 返回 最近一次对话框的用户从"文件类型"下拉列表中选择的 filter 的 索引号 mLastSelFilterIdx
int OSDLastSelFilterIdx();