//////////////////////////////////////////////////////////////////////
// CBMPLoad 类：加载 BMP 文件并可获得其中的所有像素值
// 一个本类对象一次只可加载一个 BMP 文件
//
// 支持：需要 BWindows、mdlFileSys 模块的支持
//////////////////////////////////////////////////////////////////////

#include "BWindows.h"


#pragma once

class CBBMPLoad  
{
public:
	CBBMPLoad(LPCTSTR szBmpFile=NULL);				// lpszBmpFile 为当前要加载的 BMP 文件名（含全路径），也可为空，以后通过调用 LoadBitmapFile 函数时再设置。
	virtual ~CBBMPLoad();
	
	bool LoadBitmapFile(LPCTSTR szBmpFile=NULL);	// 加载一个BMP文件，lpszBmpFile为空时使用已设置的文件名
													// （如在创建对象时通过构造函数设置了），如尚未设置文件名必须
													// 现在设置。lpszBmpFile 如不为空将覆盖先前构造函数时给出的文件名
													// （如果构造函数时也给出的话）
	long Width();									// 返回位图宽度，必须首先 LoadBitmapFile 后才有效
	long Height();									// 返回位图高度，必须首先 LoadBitmapFile 后才有效
	long Type();
	long WidthBytes();
	WORD Planes();
	WORD BitsPixel();
	
	LPTSTR GetErrDesp();							// 返回错误信息（指向对象内部数据字符串的指针）（如果有的话）

	unsigned char ** GetBMPBytes();					// 获得位图的所有像素，返回值 指向 本对象内部的数据空间，为一个指针数组的起始地址，指针数组各元素分别指向像素数据每行的首地址
	void ReleaseResource(bool bFreeSzBmpFile=true);	// 释放资源：bFreeSzBmpFile==true 时，也释放保存文件名的 m_szBmpFile 的资源；否则不释放 m_szBmpFile，而仅释放其他资源

private:
	LPTSTR m_szBmpFile;			// 保存当前加载的 BMP 文件名（含全路径）。将用 new 开辟空间，在对象析构时自动释放空间
	HBITMAP m_hBitmap;			// 位图句柄（为0时表示尚未打开位图）
	unsigned char * m_ptrPixels;// 位图数据，二维数组先行后列排列后组成的字节流，
								// 将来可用 char *m_pLines[] 数组的各元素指向其中各“行”的起始地址，则 
								// 用 m_pLines[row][col] 可访问具体的某个元素
								// 无论位图格式，m_Pixels 一律一个字节（即一个数组元素）存一个像素，故本程序只能8位（含）以下的位图
	unsigned char **m_pLines;	// 指针数组，本身的空间由本对象自动管理，在 LoadBitmapFile 之后将自动设置其各元素值为位图每行像素的起始地址
	BITMAP m_bm;				// 位图信息
	LPTSTR m_ErrDesp;			// 错误信息字符串。将用 new 开辟空间，在对象析构时自动释放空间


	void SetErrDesp(LPCTSTR errDesp);	// 设置错误信息字符串，自动开辟空间，保存到 m_ErrDesp
};
