//////////////////////////////////////////////////////////////////////
// mdlOpenSaveDlg.cpp: 实现打开、保存对话框
//
//////////////////////////////////////////////////////////////////////


#include "mdlOpenSaveDlg.h"
#include "BHashStrK.h"

// ========================================================================
// 模块级变量及类型定义，这些全局变量只能被本源程序文件使用，不能被其他源程序文件使用
// ========================================================================

static CBHashLK mMulSelFiles;				// 如果对话框允许多选，最近一次多选的每个文件保存于此。仅使用哈希表的 ItemStr 收集一批字符串
static TCHAR mFilePath[mcMaxFileLenSingle];	// 存放最近一次选择的文件名(多选文件时为路径)，也可给它赋值作为对话框选择的默认文件
static int mLastSelFilterIdx=0;				// 存放最近一次对话框的用户从"文件类型"下拉列表中选择的 filter 的 索引号
static TCHAR * mpStrFilter=NULL;			// 对话框的文件类型 Filter 字符串("|"分隔)。

// 哈希表保存已登记的对话框信息
// Key=登记对话框的 Key (字符串)，ItemStr=该对话框的 Filter，ItemStr2=上次选择的文件
//   ItemLong=上次选择的 Filter 的索引号
static CBHashStrK mRegedDlgKeys;				


// 设定对话框的文件类型 Filter ("|"分隔)
// 开辟字符串空间，将新的 Filter 字符串存入此空间，并让 mpStrFilter 指向它
// sFilter 为 NULL 时，使用“"文本文件(*.txt)|*.txt"”
//         为 "" 时，让 bAddAllFile 为 true，则使用“所有文件(*.*)|*.*”
// bAddAllFile 是否自动在末尾加上 "|所有文件(*.*)|*.*"
// sAsRegKey：以此为 Key 登记对话框（=""不登记）
// 已登记的对话框可以保存上次打开的文件路径和上次选择的 Filter
//   在 OsdOpenDlg/OsdSaveDlg 中给出 Key 即可打开这个对话框
void OsdSetFilter( LPCTSTR sFilter/*=NULL*/, bool bAddAllFile/*=true*/, LPCTSTR sAsRegKey/*=NULL*/ )
{

	// 如果 sFilter 为 NULL，则让其指向字符串常量 mc_strFilterTxtFile 的地址
	if (sFilter==NULL) sFilter=mc_strFilterTxtFile;

	// 开辟新的字符串空间，以便保存新的 Filter 字符串
	TCHAR * pft = NULL;
	if (bAddAllFile)
		pft = new TCHAR [_tcslen(sFilter) + 1 + _tcslen(mc_strFilterAllFile)];	// 所有文件的 Filter 部分的长度一起算入
	else
		pft = new TCHAR [_tcslen(sFilter) + 1];
	HM.AddPtr(pft);		// 用 HM 管理 pft 的字符串空间

	// 将新的 Filter 字符串存入新空间
	_tcscpy(pft, sFilter);
	if (*(pft+_tcslen(pft)-1)=='|') *(pft + _tcslen(pft)-1)='\0';	// 去掉最后的 | （在添加“所有文件”之前）
	if (bAddAllFile) _tcscat(pft, mc_strFilterAllFile);
	// 这时不必再检查和去掉最后的 | ，因如果添加了“所有文件”，mc_strFilterAllFile 的最后没有 |；
	//   若没添加所有文件，刚刚已经检查和去掉了最后的 | 

	// 让 mpStrFilter 指向新空间
	mpStrFilter=pft;	// 如果 mpStrFilter 原已有指向，那个指向在以前已被存入 HM 对象处理，不必关心要释放那个指向的空间
	if (*mpStrFilter == '|') mpStrFilter++;		// 如果开头是一个 |，去掉开头的这个 |

	// 登记对话框
	if (sAsRegKey)
	{
		if (mRegedDlgKeys.IsKeyExist(sAsRegKey))
		{
			// 该对话框已登记，先删除以前的登记，以便用新数据覆盖
			mRegedDlgKeys.Remove(sAsRegKey, false);
		}
		// 登记
		mRegedDlgKeys.Add(0, sAsRegKey, 1, 0, TEXT(""), mpStrFilter);
	}
}


// 返回最近一次设置的对话框的文件类型 Filter ("|"分隔)
// 返回的是一个字符指针，指向所需字符串；字符串内存空间由本模块自动管理，主调程序不必干预
// 主调程序若修改了其所指向的内容，则本模块的 Filter 内容也被改变
LPTSTR OsdLastFilter()
{
	return mpStrFilter;
}


// 打开“打开文件”对话框
// bAllowMulSel=false 时，函数返回文件名（字符串空间由本模块管理），出错或被按取消返回 ""（非NULL）；
// bAllowMulSel=true 时，函数返回文件路径（字符串空间由本模块管理），并设置 mMulSelFiles
// bRetReadOnly 指向一个 bool 型变量的指针，*b=true 时显示“以只读方式打开”复选框，并由此参数返回 
//   用户是否勾选了“以只读方式打开”复选框；*b=false 或 =NULL 不显示“以只读方式打开”复选框
// 若给出 sAsRegedKey 显示已登记为 sAsRegedKey 的 Key 的对话框，否则显示一个“新的”对话框
LPTSTR OsdOpenDlg( HWND hWnd/*=0*/, LPCTSTR sDlgTitle/*=NULL*/, LPCTSTR sAsRegedKey/*=NULL*/, bool bAllowMulSel/*=false*/, bool *bRetReadOnly/*=false*/ )
{

	OPENFILENAME ofn;
	TCHAR *pstr;
	int i;

	// 填充 ofn
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = 0;
	ofn.lpstrFileTitle = NULL;  // 不需要接收文件主名
	ofn.nMaxFileTitle= 0;
	ofn.lpstrDefExt = NULL;		// 不需要自动添加扩展名

	// 开辟缓冲区，接收用户选择的文件名
	if (bAllowMulSel)
	{
		// 启用多选文件时，使用 mcMaxFileLenMul 大小的缓冲区
		ofn.lpstrFile = new TCHAR[mcMaxFileLenMul];
		memset(ofn.lpstrFile, 0, mcMaxFileLenMul);
		HM.AddPtr(ofn.lpstrFile);	// 将指针保存到 HM 管理该 new 分配的空间
		ofn.nMaxFile = mcMaxFileLenMul;
	}
	else
	{
		// 只允许选择单个文件时，使用 mcMaxFileLenSingle 大小的缓冲区
		ofn.lpstrFile = new TCHAR[mcMaxFileLenSingle];
		memset(ofn.lpstrFile, 0, mcMaxFileLenSingle);
		HM.AddPtr(ofn.lpstrFile);	// 将指针保存到 HM 管理该 new 分配的空间
		ofn.nMaxFile = mcMaxFileLenSingle;		
	}


	// ===== 如果使用已登记的对话框，根据已登记的内容再更新设置 =====
	if (sAsRegedKey && mRegedDlgKeys.IsKeyExist(sAsRegedKey))	// 既指定了使用登记对话框，又必须检验指定的key有效
	{
		TCHAR *ptemp;
		long lngTemp;
		// 设置 Filter
		ptemp = mRegedDlgKeys.ItemStr2(sAsRegedKey,false);
		if (ptemp) 
		{
			mpStrFilter = new TCHAR[_tcslen(ptemp)+1];  // 如果 mpStrFilter 原已有指向，那个指向在以前已被存入 HM 对象处理，不必关心要释放那个指向的空间
			HM.AddPtr(mpStrFilter);  					// 用 HM 管理 Filter 字符串空间
			_tcscpy(mpStrFilter, ptemp);				// 后面程序再将 mpStrFilter 改为 '\0' 分隔并填入 ofn
		}
		// 设置上次选择的文件目录为本次默认目录
		ptemp = mRegedDlgKeys.ItemStr(sAsRegedKey,false);
		if (ptemp) _tcscpy(mFilePath, ptemp);	// 后面程序再将 mFilePath 填入 ofn
		// 设置上次选择的 Filter 项为本次默认选择项
		lngTemp=mRegedDlgKeys.ItemLong(sAsRegedKey,false);
		if (lngTemp>0) ofn.nFilterIndex = (DWORD)lngTemp;
	}

    // ===== 设置默认取值 =====
	if (mpStrFilter)
	{
		// 设置 ofn.lpstrFilter，拷贝 mpStrFilter 的内容过来
		pstr = new TCHAR [_tcslen(mpStrFilter)+2];	// +2 因为最后要有连续的两个'\0'
		HM.AddPtr(pstr);							// 将指针保存到 HM 管理该 new 分配的空间
		_tcscpy(pstr, mpStrFilter);
		// 将地址赋值给 ofn.lpstrFilter
		ofn.lpstrFilter = pstr;
		// 将以“|”分隔的 ofn.lpstrFilter 转换为以 '\0' 分隔
		while (*pstr)
		{
			if (*pstr=='|') *pstr='\0';
			pstr++;
		}
		*(pstr+1)='\0';			// 让最后有两个连续的 '\0'
	}
	else
	{
		ofn.lpstrFilter = NULL;
	}
    // 设置 lpstrInitialDir
	ofn.lpstrInitialDir = mFilePath;
            

    // ===== 设置对话框标题 =====
	ofn.lpstrTitle = sDlgTitle;
    

    // ===== 设置 flags =====
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (bRetReadOnly==NULL)
		ofn.Flags = ofn.Flags | OFN_HIDEREADONLY;
	else
		if (! *bRetReadOnly) ofn.Flags = ofn.Flags | OFN_HIDEREADONLY;
    if (bAllowMulSel) ofn.Flags = ofn.Flags | OFN_ALLOWMULTISELECT;

    
    // 显示打开文件对话框
    i = GetOpenFileName(&ofn);
    if (i==0)
	{
		// 测试 GetOpenFileName 返回 False 的原因
		i=CommDlgExtendedError();
		if (i==0)
		{
			// 用户按取消：返回空字符串
			*ofn.lpstrFile='\0'; if (bRetReadOnly) *bRetReadOnly=false;
			return ofn.lpstrFile; 
		}
		else
		{
			// 出错：返回空字符串
			*ofn.lpstrFile='\0'; if (bRetReadOnly) *bRetReadOnly=false;
			return ofn.lpstrFile; 
		}
	}
    
    // ================= 正常处理 =================
    // 返回“以只读方式打开”复选框是否被选中
    if (bRetReadOnly) *bRetReadOnly = (ofn.Flags & OFN_READONLY);

	// 设置模块级变量
	_tcscpy(mFilePath, ofn.lpstrFile);		// 多选文件时用户若仅选择了一个文件，下面程序还会修改 mFilePath 使其仅保存路径部分
	mLastSelFilterIdx = ofn.nFilterIndex;	
	
    // 处理返回的文件名
	if (bAllowMulSel)
	{
		// 将多选的文件逐个添加到 mMulSelFiles
		mMulSelFiles.Clear();
		i=_tcslen(ofn.lpstrFile)+1;	// i 指向下一字段的起始位置（跳过第一字段，因第一字段为路径）

		while (*(ofn.lpstrFile+i))
		{
			mMulSelFiles.Add(0, i, 0, 0, ofn.lpstrFile+i, 0, 0.0, false);	// key使用 i，不同项不同即可；mMulSelFiles 会开辟内存空间拷贝字符串
			i+=_tcslen(ofn.lpstrFile+i)+1;		// i 指向下一字段的起始位置
		}

		// 多选状态用户只选了一个文件
		if (mMulSelFiles.Count()==0)
		{
			// 分解出路径部分和文件名部分，仍按照路径由函数返回、文件名由 mMulSelFiles 返回
			TCHAR *pSlash=_tcsrchr(ofn.lpstrFile, '\\');		// 查找最后一个 '\\'
			if (pSlash)
			{
				mMulSelFiles.Add(0, i, 0, 0, pSlash+1, 0, 0.0, false);	// mMulSelFiles 会开辟内存空间拷贝字符串
				*(pSlash+1)='\0';						// 截断 ofn.lpstrFile，使其仍仅为路径部分
				_tcscpy(mFilePath, ofn.lpstrFile);		// 重新拷贝仅路径部分的内容到 mFilePath
			}
			else
			{
				// 未找到最后的 '\\'，此段程序仅为容错
				mMulSelFiles.Add(0, i, 0, 0, ofn.lpstrFile, 0, 0.0, false);
			}
		}		// if (mMulSelFiles.Count()==0)

		// 如多选文件，返回的路径后自动确保有 '\'
		if (*(ofn.lpstrFile + _tcslen(ofn.lpstrFile) - 1) != '\\')
		{
			*(ofn.lpstrFile + _tcslen(ofn.lpstrFile) + 1) = '\0';
			*(ofn.lpstrFile + _tcslen(ofn.lpstrFile)) = '\\';
		}
	}  // if (bAllowMulSel)


	// ===== 如调用成功，更新已登记的信息 =====
	if (sAsRegedKey && mRegedDlgKeys.IsKeyExist(sAsRegedKey))	// 既指定了使用登记对话框，又必须检验指定的key有效
	{
		// 获得 ofn.lpstrFile 的路径部分，登记信息只保存路径部分 => pPath
		TCHAR *pPath=new TCHAR[_tcslen(ofn.lpstrFile) + 1];	// 空间开辟得足够多，pPath 空间稍后 delete
		TCHAR *pSlh;
		_tcscpy(pPath, ofn.lpstrFile);
		pSlh=_tcsrchr(pPath,'\\');	// 查找最后一个 '\\'（多选时路径前面已确保最后为 '\\'）
		if (pSlh) *(pSlh+1)='\0';	// 让在 '\\' 的下一个位置处终止字符串
		mRegedDlgKeys.Remove(sAsRegedKey, false);
		mRegedDlgKeys.Add(0, sAsRegedKey, ofn.nFilterIndex, 0, pPath, mpStrFilter);	// 对 pPath、mpStrFilter 函数将拷贝字符串到哈希表中，之后哈希表不再使用 pPath、mpStrFilter 指向的内容因此指向的内容可被安全删除
		delete []pPath;
	}

	// 返回文件名
	return ofn.lpstrFile;
}



// 打开“保存文件”对话框
// sDefaultFileName 表示自动填充到对话框的默认文件名
// 若给出 sAsRegedKey 显示已登记为 sAsRegedKey 的 Key 的对话框，否则显示一个“新的”对话框
LPTSTR OsdSaveDlg( HWND hWnd/*=0*/, LPCTSTR sDefaultFileName/*=NULL*/, LPCTSTR sDlgTitle/*=NULL*/, LPCTSTR sAsRegedKey/*=NULL*/ )
{
	OPENFILENAME ofn;
	TCHAR *pstr;
	int i;

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = 0;

	ofn.lpstrFile = new TCHAR[mcMaxFileLenSingle];
	memset(ofn.lpstrFile, 0, mcMaxFileLenSingle);
	HM.AddPtr(ofn.lpstrFile);
	if (sDefaultFileName) _tcscpy(ofn.lpstrFile, sDefaultFileName);
	ofn.nMaxFile = mcMaxFileLenSingle;

    ofn.lpstrFileTitle = NULL;	// 不需要接收文件主名
    ofn.nMaxFileTitle = 0;
    ofn.lpstrDefExt = TEXT("");		// 自动添加扩展名（如果为 NULL，则不自动添加【文件类型框】中的选择）


	// ===== 如果使用已登记的对话框，根据已登记的内容再更新设置 =====
	if (sAsRegedKey && mRegedDlgKeys.IsKeyExist(sAsRegedKey))	// 既指定了使用登记对话框，又必须检验指定的key有效
	{
		TCHAR *ptemp;
		long lngTemp;
		// 设置 Filter
		ptemp = mRegedDlgKeys.ItemStr2(sAsRegedKey,false);
		if (ptemp) 
		{
			mpStrFilter = new TCHAR[_tcslen(ptemp)+1];  // 如果 mpStrFilter 原已有指向，那个指向在以前已被存入 HM 对象处理，不必关心要释放那个指向的空间
			HM.AddPtr(mpStrFilter);						// 用 HM 管理 Filter 字符串空间
			_tcscpy(mpStrFilter, ptemp);	// 后面程序再将 mpStrFilter 改为 '\0' 分隔并填入 ofn
		}
		// 设置上次选择的文件目录为本次默认目录
		ptemp = mRegedDlgKeys.ItemStr(sAsRegedKey,false);
		if (ptemp) _tcscpy(mFilePath, ptemp);	// 后面程序再将 mFilePath 填入 ofn
		// 设置上次选择的 Filter 项为本次默认选择项
		lngTemp=mRegedDlgKeys.ItemLong(sAsRegedKey,false);
		if (lngTemp>0) ofn.nFilterIndex = (DWORD)lngTemp;
	}


	
    // ===== 设置默认取值 =====
	if (mpStrFilter)
	{
		// 设置 ofn.lpstrFilter，拷贝 mpStrFilter 的内容过来
		pstr = new TCHAR [_tcslen(mpStrFilter)+2];	// +2 因为最后要有连续的两个'\0'
		HM.AddPtr(pstr);							// 将指针保存到 HM 管理该 new 分配的空间
		_tcscpy(pstr, mpStrFilter);
		// 将地址赋值给 ofn.lpstrFilter
		ofn.lpstrFilter = pstr;
		// 将以“|”分隔的 ofn.lpstrFilter 转换为以 '\0' 分隔
		while (*pstr)
		{
			if (*pstr=='|') *pstr='\0';
			pstr++;
		}
		*(pstr+1)='\0';			// 让最后有两个连续的 '\0'
	}
	else
	{
		ofn.lpstrFilter = NULL;
	}

    // 设置 lpstrInitialDir
	ofn.lpstrInitialDir = mFilePath;

   
    // ===== 设置对话框标题 =====
	ofn.lpstrTitle = sDlgTitle;
    
    // ===== 设置 flags =====
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	
	// 显示保存文件对话框
    i = GetSaveFileName(&ofn);
    if (i==0)
	{
		// 测试 GetSaveFileName 返回 False 的原因
		i=CommDlgExtendedError();
		if (i==0)
		{
			// 用户按取消：返回空字符串
			*ofn.lpstrFile='\0'; 
			return ofn.lpstrFile; 
		}
		else
		{
			// 出错：返回空字符串
			*ofn.lpstrFile='\0'; 
			return ofn.lpstrFile; 
		}
	}

	// 设置模块级变量
	_tcscpy(mFilePath, ofn.lpstrFile);
	mLastSelFilterIdx = ofn.nFilterIndex;

	// ===== 如调用成功，更新已登记的信息 =====
	if (sAsRegedKey && mRegedDlgKeys.IsKeyExist(sAsRegedKey))	// 既指定了使用登记对话框，又必须检验指定的key有效
	{
		// 获得 ofn.lpstrFile 的路径部分，登记信息只保存路径部分 => pPath
		TCHAR *pPath=new TCHAR[_tcslen(ofn.lpstrFile) + 1];	// 空间开辟得足够多，pPath 空间稍后 delete
		TCHAR *pSlh;
		_tcscpy(pPath, ofn.lpstrFile);
		pSlh=_tcsrchr(pPath,'\\');	// 查找最后一个 '\\'（多选时路径前面已确保最后为 '\\'）
		if (pSlh) *(pSlh+1)='\0';	// 让在 '\\' 的下一个位置处终止字符串
		mRegedDlgKeys.Remove(sAsRegedKey, false);
		mRegedDlgKeys.Add(0, sAsRegedKey, ofn.nFilterIndex, 0, pPath, mpStrFilter);	// 对 pPath、mpStrFilter 函数将拷贝字符串到哈希表中，之后哈希表不再使用 pPath、mpStrFilter 指向的内容因此指向的内容可被安全删除
		delete []pPath;
	}

	// 返回对话框中用户输入的文件名及路径
	return ofn.lpstrFile;
}



// 获得最近一次打开对话框选择的多选文件，将由本类对象创建动态指针数组 pptrMulFiles
//   该数组下标从 1 开始，空间 [0] 不用
//   指针数组的首地址由参数返回。本类对象管理一切动态空间
//   多选文件的个数（也是动态数组的个数）由函数返回，每个数组元素分别指向一个字符串
//   空间表示每个选择的文件（只有文件名部分，无路径）
// 例如：
//	TCHAR **mul;
//	int fileCount, i;
//  fileCount=OSDMulSelFiles(mul);
//  for(i=1; i<=fileCount; i++)
//		cout<<mul[i]<<endl;
int OSDMulSelFiles( TCHAR ** &pptrMulFiles )
{
	int i;

	// 无多选文件，返回 0
	if (mMulSelFiles.Count()==0)
	{
		pptrMulFiles=0;
		return 0;
	}

	// 创建指针数组
	pptrMulFiles=new TCHAR *[mMulSelFiles.Count()+1];	// 该数组下标从 1 开始
	pptrMulFiles[0] = NULL;								// 下标为 [0] 的空间不用，赋值为 NULL 以安全
	HM.AddPtr(pptrMulFiles);

	// 为各个选择的文件字符串开辟存储空间，并将空间地址存入 pptrMulFiles[] 的各个元素
	for (i=1; i<=mMulSelFiles.Count(); i++)
	{
		pptrMulFiles[i] = new TCHAR [_tcslen(mMulSelFiles.ItemStrFromIndex(i,false))+1];
		HM.AddPtr(pptrMulFiles[i]);
		_tcscpy(pptrMulFiles[i], mMulSelFiles.ItemStrFromIndex(i,false));
	}

	// 返回文件个数
	return mMulSelFiles.Count();
}


// 返回最近一次选择的文件名(多选文件时为路径)
LPTSTR OSDFilePath()
{
	return mFilePath;
}

// 设置 mFilePath 作为默认文件名
void OSDSetInitFilePath( LPCTSTR newInitFilePath )
{
	_tcscpy(mFilePath, newInitFilePath);
}


// 返回 最近一次对话框的用户从"文件类型"下拉列表中选择的 filter 的 索引号 mLastSelFilterIdx
int OSDLastSelFilterIdx()
{
	return mLastSelFilterIdx;
}

