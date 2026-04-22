//////////////////////////////////////////////////////////////////////
// mdlRegOperation.h: 实现注册表操作的模块函数原型及变量常量定义
//
// 支持：
//     需要 BWindows 模块的支持
//////////////////////////////////////////////////////////////////////

#include "BWindows.h"

// 本程序的注册表子项将使用：<mcRootKey>\Software\<mcSubKey>
// 要使用其他子项，可修改此常量值
#define mcSubKey	TEXT("NZKey")		// 未修改此处扣分
#define mcRootKey	eHKEY_CURRENT_USER		// 默认保存在的根项

// 兼容64位操作系统
#define KEY_ALL_ACCESS64 (KEY_ALL_ACCESS | 256)

enum ERegControlKeys		// 注册表项
{
	eHKEY_CLASSES_ROOT = (int)HKEY_CLASSES_ROOT,
	eHKEY_CURRENT_CONFIG = (int)HKEY_CURRENT_CONFIG, 
	eHKEY_CURRENT_USER = (int)HKEY_CURRENT_USER, 
	eHKEY_LOCAL_MACHINE = (int)HKEY_LOCAL_MACHINE, 
	eHKEY_USERS = (int)HKEY_USERS, 
	eHKEY_PERFORMANCE_DATA = (int)HKEY_PERFORMANCE_DATA, 
    eHKEY_DYN_DATA = (int)HKEY_DYN_DATA
};

//////////////////////////////////////////////////////////////////////////
// 默认本程序存储位置下的操作：
//		<mcRootKey>\Software\<mcSubKey>

// 生成子项 szSubKey 的默认路径
// 即生成路径为：Software\<mcSubKey>\<szSubKey>
LPTSTR MakeSubKey(LPCTSTR szSubKey);

// 在 <mcRootKey>\<szSubKey> 下面建立一个子项
// szSubKey 可以是多级路径，将一次性建立路径中的所有子项，如"Software\abc\de\fg"
bool RegCreate(LPCTSTR szSubKey, ERegControlKeys eRootKey=mcRootKey);

// 读取注册表字符串类型的键值，由函数返回，出错或其他返回""
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认值
// 结果字符串空间由本模块自动开辟，由 HM 自动管理
LPTSTR RegGetValueString(LPCTSTR szSubKey, 
						 LPCTSTR szKeyName, 
						 ERegControlKeys eRootKey=mcRootKey);


// 读取注册表长整数类型的键值，由函数返回，出错或其他返回0
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
long RegGetValueLong(LPCTSTR szSubKey, 
					 LPCTSTR szKeyName, 
					 ERegControlKeys eRootKey=mcRootKey);


// 读取注册表二进制类型的键值，读取到的二进制字节空间
//   由本函数自动开辟、由 HM 自动管理，从 ptrBuff 返回此空间首地址
// 函数返回此空间的字节个数，出错返回 0
// 位置为 <eRootKey>\software\<mcSubKey>\subKey
// szKeyName 为键值名称，=""表示获取默认值
long RegGetValueBin(LPCTSTR szSubKey, 
					LPCTSTR szKeyName, 
					char * ptrBuff, 
					ERegControlKeys eRootKey=mcRootKey);

// 在一个子项中设置一个字符串型的键值，如果该项不存在则先创建它
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 要设置的值为 szValue
// 返回值：成功返回 true，失败返回 false
bool RegSetValueString(LPCTSTR szSubKey, 
					   LPCTSTR szKeyName, 
					   LPCTSTR szValue, 
					   ERegControlKeys eRootKey=mcRootKey);


// 在一个子项中设置一个长整数型的键值，如果该项不存在则先创建它
// 位置为 <mcRootKey>\software\<mcSubKey>\<szSubKey>
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 要设置的值为 lValue
// 返回值：成功返回 true，失败返回 false
bool RegSetValueLong(LPCTSTR szSubKey, 
					 LPCTSTR szKeyName, 
					 long lValue, 
					 ERegControlKeys eRootKey=mcRootKey);

// 在一个子项中设置一个二进制型的键值，如果该项不存在则先创建它
// 位置为 <eRootKey>\software\<mcSubKey>\subKey
// 键值名为 szKeyName（="" 表示默认键值），值为 pBuff 地址开始的、iBuffLen 个字节
// 成功返回 True，失败返回 False
bool RegSetValueBin(LPCTSTR szSubKey, 
					LPCTSTR szKeyName, 
					const char * pBuff,
					const int iBuffLen,
					ERegControlKeys eRootKey=mcRootKey);


// 删除 szSubKey 子项下的一个注册表键值
bool RegDelValue(LPCTSTR szSubKey, LPCTSTR szKeyName, ERegControlKeys eRootKey=mcRootKey);

// 删除 <mcRootKey>\Software\<mcSubKey> 下面的一个子项
// szSubKey 可以是多级路径，但只删除最后一级子项
// 如果 subKey 中指定的最后一级子项中还包含下一级子项，则不能被删除
//   只有不包括下一级子项的子项才能被删除
bool RegDel(LPCTSTR szSubKey, ERegControlKeys eRootKey=mcRootKey);


//////////////////////////////////////////////////////////////////////////
// 注册表任意位置下的操作：
//		<controlKey>\<szSubKey>


// 从 注册表的任意位置 读取字符串类型的键值，由函数返回，出错或其他返回""
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 结果字符串空间由本模块自动开辟，由 HM 自动管理
LPTSTR RegGetValueStringCtrl(ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName);

// 从 注册表的任意位置 读取长整数类型的键值，由函数返回，出错或其他返回0
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
long RegGetValueLongCtrl(ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName);

// 在 注册表的任意位置 设置一个字符串型的键值，如果该项不存在则先创建它
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 要设置的值为 szValue
// 返回值：成功返回 true，失败返回 false
bool RegSetValueStringCtrl( ERegControlKeys controlKey, 
							LPCTSTR szSubKey, 
							LPCTSTR szKeyName, 
							LPCTSTR szValue); 

// 在 注册表的任意位置 设置一个长整数型的键值，如果该项不存在则先创建它
// szKeyName 为键值名称，== "" 或 指针==0 表示获取默认键值
// 要设置的值为 lValue
// 返回值：成功返回 true，失败返回 false
bool RegSetValueLongCtrl( ERegControlKeys controlKey, 
						  LPCTSTR szSubKey, 
						  LPCTSTR szKeyName, 
						  long lValue);

// 删除 注册表的任意位置 子项下的一个注册表键值
bool RegDelValueCtrl(ERegControlKeys controlKey, LPCTSTR szSubKey, LPCTSTR szKeyName);

// 删除 注册表的任意位置 即<controlKey>\<szSubKey> 下面的一个子项
// szSubKey 可以是多级路径，但只删除最后一级子项
// 如果 subKey 中指定的最后一级子项中还包含下一级子项，则不能被删除
//   只有不包括下一级子项的子项才能被删除
bool RegDelCtrl(ERegControlKeys controlKey, LPCTSTR szSubKey);


//////////////////////////////////////////////////////////////////////////
// 文件关联操作

// 建立一个文件关联
bool RegCreateAssoFile(LPTSTR szFileExp, 
					   LPTSTR szClassName = NULL, 
					   LPTSTR szDescrip = NULL,
					   LPTSTR szIconFile = NULL, 
					   LPTSTR szOpenCommand = NULL, 
					   LPTSTR szPrintCommand = NULL);


// 删除一个文件关联的注册表信息
// 只能删除通过 RegCreateAssoFile 建立的文件关联信息，
//   不能随意删除注册表中的其他信息
bool RegDelAssoFile(LPCTSTR szFileExp);

// 建立一个与“所有文件”都能打开的关联
// szClassName 为注册表中 HKEY_CLASSES_ROOT\*\shell\className 的 className
// szDescrip 为“用……打开”的文字说明
// szCmdExe 为打开命令行
bool RegCreateAllAssoFile(LPCTSTR szClassName, LPCTSTR szDescrip, LPCTSTR szCmdExe);

// 删除一个与“所有文件”都能打开的关联
// szClassName 为注册表中 HKEY_CLASSES_ROOT\*\shell\className 的 className
bool RegDelAllAssoFile(LPCTSTR szClassName);