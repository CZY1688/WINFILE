//////////////////////////////////////////////////////////////////////
// mdlRegOperation.cpp: 实现注册表操作的模块函数实现
//
// 支持：
//     需要 BWindows 模块的支持
//////////////////////////////////////////////////////////////////////

#include "mdlRegOperation.h"


// 保存子项路径的缓冲区，每次 MakeSubKey 将修改此值为所需路径字符串
TCHAR m_szSubKeyBuff[2048];	

// 空字符串缓冲
TCHAR m_szEmptyString[2]={0};	

// 生成子项 szSubKey 的默认路径
// 即生成路径为：Software\<mcSubKey>\<szSubKey>
LPTSTR MakeSubKey(LPCTSTR szSubKey)
{
	*m_szSubKeyBuff = 0;
	_tcscpy(m_szSubKeyBuff, TEXT("Software\\"));
	_tcscat(m_szSubKeyBuff, mcSubKey);
	if ( *m_szSubKeyBuff )	// 若 m_szSubKeyBuff 不为空串
		_tcscat(m_szSubKeyBuff, TEXT("\\"));	// 最后再加 “\”
	_tcscat(m_szSubKeyBuff, szSubKey);	// 连接子项字符串
	return m_szSubKeyBuff; 
}

// 在 <mcRootKey>\<szSubKey> 下面建立一个子项
// szSubKey 可以是多级路径，将一次性建立路径中的所有子项，如"Software\abc\de\fg"
bool RegCreate(LPCTSTR szSubKey, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	HKEY hKey; 
	LONG ret = RegCreateKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
	  0, NULL, 0, KEY_ALL_ACCESS64, 0, &hKey, NULL); 
	RegCloseKey(hKey);

	return (ret==ERROR_SUCCESS);
}


// 删除 <mcRootKey>\Software\<mcSubKey> 下面的一个子项
// szSubKey 可以是多级路径，但只删除最后一级子项
// 如果 subKey 中指定的最后一级子项中还包含下一级子项，则不能被删除
//   只有不包括下一级子项的子项才能被删除
bool RegDel(LPCTSTR szSubKey, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	return RegDeleteKey((HKEY)eRootKey, MakeSubKey(szSubKey))==ERROR_SUCCESS; 
}

// 读取注册表字符串类型的键值，由函数返回，出错或其他返回""
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 结果字符串空间由本模块自动开辟，由 HM 自动管理
LPTSTR RegGetValueString(LPCTSTR szSubKey, LPCTSTR szKeyName, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	LPTSTR szResult = NULL;		// 结果字符串指针
	LONG ret;

	// 打开子项 
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
		0, KEY_ALL_ACCESS64, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		*m_szEmptyString = 0;
		return m_szEmptyString;	// 返回空字符串
	}
	
	// 获得子项中键值名称为 keyName 的键值的数据类型和缓冲区长度
	DWORD dwType=0, dwLen=0; 
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, NULL, &dwLen);
	if (dwLen<0 || ret!= ERROR_SUCCESS)
		{ *m_szEmptyString = 0; return m_szEmptyString;	} // 返回空字符串
	// 准备缓冲区 szResult
	szResult = new TCHAR[dwLen+1];
	HM.AddPtr(szResult);
	// 用缓冲区 szResult 获取键值
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, (LPBYTE)szResult, &dwLen);
	if (ret != ERROR_SUCCESS) 
		{ *m_szEmptyString = 0; return m_szEmptyString;	} // 返回空字符串
	// 关闭子项句柄
	RegCloseKey(hKey);		
	// 返回字符串
	return szResult;
}


// 读取注册表长整数类型的键值，由函数返回，出错或其他返回0
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
long RegGetValueLong(LPCTSTR szSubKey, LPCTSTR szKeyName, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	long lResult=0;
	LONG ret;

	// 打开子项 
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
		0, KEY_ALL_ACCESS64, &hKey);
	if (ret != ERROR_SUCCESS) return 0;	// 返回 0 表示失败

	// 获取长整数的键值类型 => dwType、长度 => dwLen
	DWORD dwType=0, dwLen=0; 
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, NULL, &dwLen);

	// 获取长整数的键值到变量 lResult
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, (LPBYTE)&lResult, &dwLen);
	if (ret != ERROR_SUCCESS) lResult=0;	// 出错：准备返回 0
	
	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值 lResult
	return lResult;
}


// 在一个子项中设置一个字符串型的键值，如果该项不存在则先创建它
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 要设置的值为 szValue
// 返回值：成功返回 true，失败返回 false
bool RegSetValueString(LPCTSTR szSubKey, LPCTSTR szKeyName, LPCTSTR szValue, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	LONG ret;
	
	// 打开子项：用 RegCreateKeyEx 打开，若子项不存在则创建它
	HKEY hKey=NULL; 
	ret = RegCreateKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
	  0, NULL, 0, KEY_ALL_ACCESS64, 0, &hKey, NULL); 
	if (ret != ERROR_SUCCESS) return false;	// 返回 false 表示失败

	// 设置子项键值
	ret = RegSetValueEx(hKey, szKeyName, 0, REG_SZ, 
		(CONST BYTE *)szValue, (lstrlen(szValue)+1) * sizeof(TCHAR) );
	
	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值
	return ret==ERROR_SUCCESS;	
}

// 在一个子项中设置一个长整数型的键值，如果该项不存在则先创建它
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 要设置的值为 lValue
// 返回值：成功返回 true，失败返回 false
bool RegSetValueLong(LPCTSTR szSubKey, LPCTSTR szKeyName, long lValue, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	LONG ret;
	
	// 打开子项：用 RegCreateKeyEx 打开，若子项不存在则创建它
	HKEY hKey=NULL; 
	ret = RegCreateKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
		0, NULL, 0, KEY_ALL_ACCESS64, 0, &hKey, NULL); 
	if (ret != ERROR_SUCCESS) return false;	// 返回 false 表示失败
	
	// 设置子项键值
	ret = RegSetValueEx(hKey, szKeyName, 0, REG_DWORD, 
		(CONST BYTE *)&lValue, sizeof(lValue) );
	
	// 关闭子项句柄
	RegCloseKey(hKey);
	
	// 返回值
	return ret==ERROR_SUCCESS;	
}


// 在一个子项中设置一个二进制型的键值，如果该项不存在则先创建它
// 位置为 <eRootKey>\software\<mcSubKey>\subKey
// 键值名为 szKeyName（="" 表示默认键值），值为 pBuff 地址开始的、iBuffLen 个字节
// 成功返回 True，失败返回 False
bool RegSetValueBin( LPCTSTR szSubKey, LPCTSTR szKeyName, const char * pBuff, const int iBuffLen, ERegControlKeys eRootKey/*=mcRootKey*/ )
{
	LONG ret;

	// 打开子项：用 RegCreateKeyEx 打开，若子项不存在则创建它
	HKEY hKey=NULL; 
	ret = RegCreateKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
		0, NULL, 0, KEY_ALL_ACCESS64, 0, &hKey, NULL); 
	if (ret != ERROR_SUCCESS) return false;	// 返回 false 表示失败

	// 设置子项键值
	ret = RegSetValueEx(hKey, szKeyName, 0, REG_BINARY, 
		(CONST BYTE *)pBuff, (DWORD)iBuffLen );

	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值
	return ret==ERROR_SUCCESS;	
}


// 删除 szSubKey 子项下的一个注册表键值
bool RegDelValue(LPCTSTR szSubKey, LPCTSTR szKeyName, ERegControlKeys eRootKey/*=mcRootKey*/)
{
	LONG ret;
	
	// 打开子项 
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)eRootKey, MakeSubKey(szSubKey), 
		0, KEY_ALL_ACCESS64, &hKey);
	if (ret != ERROR_SUCCESS) return 0;	// 返回 0 表示失败

	// 删除键值
	ret = RegDeleteValue(hKey, szKeyName);

	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值
	return ret==ERROR_SUCCESS;	
}


// 删除 注册表的任意位置 子项下的一个注册表键值
bool RegDelValueCtrl( ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName )
{
	LONG ret;

	// 打开子项 
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)controlKey, szSubKey, 
		0, KEY_ALL_ACCESS64, &hKey);
	if (ret != ERROR_SUCCESS) return 0;	// 返回 0 表示失败

	// 删除键值
	ret = RegDeleteValue(hKey, szKeyName);

	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值
	return ret==ERROR_SUCCESS;	
}

// 删除 注册表的任意位置 即<controlKey>\<szSubKey> 下面的一个子项
// szSubKey 可以是多级路径，但只删除最后一级子项
// 如果 subKey 中指定的最后一级子项中还包含下一级子项，则不能被删除
//   只有不包括下一级子项的子项才能被删除
bool RegDelCtrl( ERegControlKeys controlKey, LPCTSTR szSubKey )
{
	return RegDeleteKey((HKEY)controlKey, szSubKey)==ERROR_SUCCESS; 
}


// 从 注册表的任意位置 读取字符串类型的键值，由函数返回，出错或其他返回""
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 结果字符串空间由本模块自动开辟，由 HM 自动管理
LPTSTR RegGetValueStringCtrl(ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName)
{
	LPTSTR szResult = NULL;		// 结果字符串指针
	LONG ret;
	
	// 打开子项，仅使用读键值数据权限 KEY_QUERY_VALUE
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)controlKey, szSubKey, 
		0, KEY_QUERY_VALUE | 256, &hKey);	// | 256：兼容 64 位操作系统
	if (ret != ERROR_SUCCESS)
	{
		*m_szEmptyString = 0;
		return m_szEmptyString;	// 返回空字符串
	}
	
	// 获得子项中键值名称为 keyName 的键值的数据类型和缓冲区长度
	DWORD dwType=0, dwLen=0; 
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, NULL, &dwLen);
	if (dwLen<0 || ret!= ERROR_SUCCESS)
		{ *m_szEmptyString = 0; return m_szEmptyString;	} // 返回空字符串
	// 准备缓冲区 szResult
	szResult = new TCHAR[dwLen+1];
	HM.AddPtr(szResult);
	// 用缓冲区 szResult 获取键值
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, (LPBYTE)szResult, &dwLen);
	if (ret != ERROR_SUCCESS) 
		{ *m_szEmptyString = 0; return m_szEmptyString;	} // 返回空字符串
	// 关闭子项句柄
	RegCloseKey(hKey);		
	// 返回字符串
	return szResult;
}


// 从 注册表的任意位置 读取长整数类型的键值，由函数返回，出错或其他返回0
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
long RegGetValueLongCtrl(ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName)
{
	long lResult=0;
	LONG ret;
	
	// 打开子项 
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)controlKey, szSubKey, 
		0, KEY_QUERY_VALUE | 256, &hKey);	// | 256：兼容 64 位操作系统
	if (ret != ERROR_SUCCESS) return 0;		// 返回 0 表示失败
	
	// 获取长整数的键值类型 => dwType、长度 => dwLen
	DWORD dwType=0, dwLen=0; 
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, NULL, &dwLen);
	
	// 获取长整数的键值到变量 lResult
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, (LPBYTE)&lResult, &dwLen);
	if (ret != ERROR_SUCCESS) lResult=0;	// 出错：准备返回 0
	
	// 关闭子项句柄
	RegCloseKey(hKey);
	
	// 返回值 lResult
	return lResult;
}


// 读取注册表二进制类型的键值，读取到的二进制字节空间
//   由本函数自动开辟、由 HM 自动管理，从 ptrBuff 返回此空间首地址
// 函数返回此空间的字节个数，出错返回 0
// 位置为 <eRootKey>\software\<mcSubKey>\subKey
// szKeyName 为键值名称，=""表示获取默认值
long RegGetValueBin( LPCTSTR szSubKey, LPCTSTR szKeyName, char * ptrBuff, ERegControlKeys eRootKey/*=mcRootKey*/ )
{
	long lResult=0;
	LONG ret;

	// 打开子项 
	HKEY hKey=NULL; 
	ret = RegOpenKeyEx((HKEY)eRootKey, szSubKey, 
		0, KEY_QUERY_VALUE | 256, &hKey);	// | 256：兼容 64 位操作系统
	if (ret != ERROR_SUCCESS) return 0;		// 返回 0 表示失败

	// 获得子项中键值名称为 szKeyName 的键值的数据类型和缓冲区长度
	DWORD dwType=0, dwLen=0; 
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, NULL, &dwLen);
	if (dwLen<0 || ret!= ERROR_SUCCESS) { RegCloseKey(hKey); return 0;}
	// 准备缓冲区 bytBuff
	char * bytBuff = new char [dwLen+1];
	HM.AddPtr(bytBuff);
	// 用缓冲区 bytBuff 获取键值
	ret = RegQueryValueEx(hKey, szKeyName, 0, &dwType, (LPBYTE)bytBuff, &dwLen);
	if (ret != ERROR_SUCCESS) { RegCloseKey(hKey); return 0;}

	// 关闭子项句柄
	RegCloseKey(hKey);		

	// 返回字节个数
	return dwLen;
}


// 建立一个文件关联
bool RegCreateAssoFile(LPTSTR szFileExp, 
					   LPTSTR szClassName /*= NULL*/, 
					   LPTSTR szDescrip /*= NULL*/,
					   LPTSTR szIconFile /*= NULL*/, 
					   LPTSTR szOpenCommand /*= NULL*/, 
					   LPTSTR szPrintCommand /*= NULL*/)
{
	// 默认返回值
	bool retVal = true;	

	// 准备扩展名文本到 szFileExp
	if (szFileExp == NULL) return false;
	if ( *szFileExp != TEXT('.') )
		szFileExp = StrAppend(TEXT("."), szFileExp);

	// 准备类型文本到 szClassName （用于子项名称）
	if (szClassName == NULL)
		szClassName = StrAppend(szFileExp+1, TEXT("file"));	// 去掉点，例如：txtfile
	if ( *(szClassName+lstrlen(szClassName)-1)==TEXT('\\') )// 若最后一个字符为 “\”，去掉它
		*(szClassName+lstrlen(szClassName)-1)=TEXT('\0');	

	// 建立扩展名子项
	if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
		szFileExp, TEXT(""), szClassName) ) retVal=false;

	// 建立 shell\open\command 子项表示打开文件关联
	if (szOpenCommand)
	{
		if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
			StrAppend(szClassName, TEXT("\\shell\\open\\command")),
			TEXT(""), szOpenCommand) ) retVal=false;
	}

	// 建立 shell\print\command 子项表示打印文件关联
	if (szPrintCommand)
	{
		if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
			StrAppend(szClassName, TEXT("\\shell\\print\\command")),
			TEXT(""), szPrintCommand) ) retVal=false;
	}

	// 建立 descrip 键值
	if (szDescrip)
	{
		if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
			szClassName, TEXT(""), szDescrip) ) retVal=false;
	}

	// 建立 图标文件 子项
	if (szIconFile)
	{
		if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
			StrAppend(szClassName, TEXT("\\DefaultIcon")), 
			TEXT(""), szIconFile) ) retVal=false;
	}

	// 返回 retVal 的值
	return retVal;
}	

// 删除一个文件关联的注册表信息
// 只能删除通过 RegCreateAssoFile 建立的文件关联信息，
//   不能随意删除注册表中的其他信息
bool RegDelAssoFile(LPCTSTR szFileExp)
{
	LPTSTR szClassName;
	
	// 默认返回值
	bool retVal = true;	

	if (szFileExp == NULL) return false;
	// 准备扩展名文本到 szFileExp
	if ( *szFileExp != TEXT('.') )
		szFileExp = StrAppend(TEXT("."), szFileExp);
	
	// 获得类型文本 => szClassName （用于子项名称）
	szClassName = RegGetValueStringCtrl(eHKEY_CLASSES_ROOT, szFileExp, TEXT(""));


	// 删除扩展名子项
	if (! RegDelCtrl(eHKEY_CLASSES_ROOT, szFileExp)) retVal=false;

	// 删除图标子项
	if (! RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(szClassName,TEXT("\\DefaultIcon")))) retVal=false;
	
	// 删除文件打开关联子项
	RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(szClassName,TEXT("\\shell\\open\\command")));
	RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(szClassName,TEXT("\\shell\\open")));

	// 删除文件打印关联子项
	RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(szClassName,TEXT("\\shell\\print\\command")));
	RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(szClassName,TEXT("\\shell\\print")));

	// 删除 shell 子项
	if (! RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(szClassName,TEXT("\\shell")))) retVal=false;

	// 删除类型子项
	if (! RegDelCtrl(eHKEY_CLASSES_ROOT,szClassName)) retVal=false;

	// 返回 retVal 的值
	return retVal;
}

bool RegSetValueStringCtrl( ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName, LPCTSTR szValue )
{
	LONG ret;

	// 打开子项：用 RegCreateKeyEx 打开，若子项不存在则创建它
	HKEY hKey=NULL; 
	ret = RegCreateKeyEx((HKEY)controlKey, szSubKey, 
		0, NULL, 0, KEY_ALL_ACCESS64, 0, &hKey, NULL); 
	if (ret != ERROR_SUCCESS) return false;	// 返回 false 表示失败

	// 设置子项键值
	ret = RegSetValueEx(hKey, szKeyName, 0, REG_SZ, 
		(CONST BYTE *)szValue, (lstrlen(szValue)+1) * sizeof(TCHAR) );

	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值
	return ret==ERROR_SUCCESS;	
}

bool RegSetValueLongCtrl( ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName, long lValue )
{
	LONG ret;

	// 打开子项：用 RegCreateKeyEx 打开，若子项不存在则创建它
	HKEY hKey=NULL; 
	ret = RegCreateKeyEx((HKEY)controlKey, szSubKey, 
		0, NULL, 0, KEY_ALL_ACCESS64, 0, &hKey, NULL); 
	if (ret != ERROR_SUCCESS) return false;	// 返回 false 表示失败

	// 设置子项键值
	ret = RegSetValueEx(hKey, szKeyName, 0, REG_DWORD, 
		(CONST BYTE *)&lValue, sizeof(lValue) );

	// 关闭子项句柄
	RegCloseKey(hKey);

	// 返回值
	return ret==ERROR_SUCCESS;	
}

// 建立一个与“所有文件”都能打开的关联
// szClassName 为注册表中 HKEY_CLASSES_ROOT\*\shell\className 的 className
// szDescrip 为“用……打开”的文字说明
// szCmdExe 为打开命令行
bool RegCreateAllAssoFile( LPCTSTR szClassName, LPCTSTR szDescrip, LPCTSTR szCmdExe )
{
	if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(TEXT("*\\shell\\"), szClassName), 
		TEXT(""), szDescrip)) return false;
	if (! RegSetValueStringCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(TEXT("*\\shell\\"), szClassName, TEXT("\\Command")),
		TEXT(""), szCmdExe)) return false;
}

// 删除一个与“所有文件”都能打开的关联
// szClassName 为注册表中 HKEY_CLASSES_ROOT\*\shell\className 的 className
bool RegDelAllAssoFile( LPCTSTR szClassName )
{
	RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(TEXT("*\\shell\\"), szClassName, TEXT("\\Command")));
	RegDelCtrl(eHKEY_CLASSES_ROOT, 
		StrAppend(TEXT("*\\shell\\"), szClassName));
	return true;
}




