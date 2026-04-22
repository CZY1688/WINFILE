//////////////////////////////////////////////////////////////////////
// BBMPLoad.cpp: CBBMPLoad 类的实现
//
//////////////////////////////////////////////////////////////////////

#include "BBMPLoad.h"
#include <memory.h>
#include "mdlFileSys.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CBBMPLoad::CBBMPLoad( LPCTSTR szBmpFile/*=NULL*/ )
{
	if (szBmpFile)
	{
		m_szBmpFile = new TCHAR[lstrlen(szBmpFile) + 1];
		lstrcpy(m_szBmpFile, szBmpFile);
	}
	else
	{
		m_szBmpFile = NULL;
	}

	// 其他初始化
	m_ErrDesp = 0;
	m_hBitmap = 0;
	m_ptrPixels = 0;
	m_pLines = 0;
	memset(&m_bm, 0, sizeof(m_bm));
}

CBBMPLoad::~CBBMPLoad()
{
	ReleaseResource();
	delete [] m_szBmpFile;	m_szBmpFile = 0;	// 释放保存 BMP 文件名的 fileBmp 字符串空间
	delete [] m_ErrDesp;	m_ErrDesp = 0;		// 释放保存错误信息的空间
}


//////////////////////////////////////////////////////////////////////
// 公有函数
//////////////////////////////////////////////////////////////////////


bool CBBMPLoad::LoadBitmapFile( LPCTSTR szBmpFile/*=NULL*/ )
{
	int r=0, i=0;

	// 确定 bmp 文件名，存入 m_fileBmp
	if (szBmpFile==NULL)
	{
		if (m_szBmpFile==NULL) return false;	// 没有指定文件名
		// else 否则使用 m_szBmpFile 作为文件名
	}
	else
	{
		// 用新名 szBmpFile 替换 m_szBmpFile
		if (m_szBmpFile) delete [] m_szBmpFile;
		m_szBmpFile = new TCHAR[lstrlen(szBmpFile) + 1];
		lstrcpy(m_szBmpFile, szBmpFile);
	}
	
	// 判断文件是否存在
	if (FMFileExist(m_szBmpFile) != 1)
	{
		SetErrDesp(TEXT("文件不存在。"));		// 设置本对象的错误信息
		return false;
	}
	
	// 如果本对象已经打开过位图，现在先释放它，然后再打开新位图
	if (m_hBitmap) ReleaseResource(false);	// 参数设为 false 表示不释放 m_szBmpFile 的空间

	// 加载 bmp 图片文件（使用原始图片大小、使用设备无关位图），获得位图句柄
	m_hBitmap = (HBITMAP)LoadImage(0, m_szBmpFile, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
	if (m_hBitmap == 0)
	{
		SetErrDesp(TEXT("加载 bmp 图片文件失败。"));	// 设置本对象的错误信息
		return false;
	}


    // 获得位图信息
	if (GetObject(m_hBitmap, sizeof(m_bm), &m_bm) == 0)
	{
		SetErrDesp(TEXT("获得位图信息失败。"));	// 设置本对象的错误信息
		ReleaseResource();						// 释放已加载的位图资源
		return false;
	}


	// 只能处理每像素8位以下的位图
	if (m_bm.bmBitsPixel > 8)
	{
		TCHAR strtmp[300];
		wsprintf(strtmp, TEXT("本程序暂只能处理位深度在8位以下（含8位）的位图（位深度是指每像素占的比特位数），该位图位深度为 %d 位，暂不能被处理。"), m_bm.bmBitsPixel);
		SetErrDesp(strtmp);			// 设置本对象的错误信息
		ReleaseResource();			// 释放已加载的位图资源
		return false;
	}


	// 开辟存储区 m_ptrPixels，将位图数据拷贝到 m_ptrPixels
	// 先将位图数据拷贝入 buffBytes，然后再根据不同的位图格式，拷贝入 m_ptrPixels
	int buffBytesCount = m_bm.bmWidthBytes * m_bm.bmHeight;
	unsigned char * buffBytes = new unsigned char [buffBytesCount];
	buffBytesCount = GetBitmapBits(m_hBitmap, buffBytesCount, buffBytes);
	if (buffBytesCount == 0)
	{
		SetErrDesp(TEXT("无法获取位图图像数据。"));		// 设置本对象的错误信息
		ReleaseResource();							// 释放已加载的位图资源
		return false;
	}
	
	// 开辟 m_Pixels 存储区，无论位图格式，m_Pixels 一律一个字节（即一个数组元素）存一个像素，故本程序只能8位（含）以下的位图
	m_ptrPixels = new unsigned char [m_bm.bmWidth * m_bm.bmHeight];
	// 开辟 m_pLines 指针数组，并使其各个元素指向位图像素的各行（实际为 m_ptrPixels 指向的空间的中间某个部分）
	m_pLines = new unsigned char * [m_bm.bmHeight];
	for (i=0; i<m_bm.bmHeight; i++)
		m_pLines[i] = m_ptrPixels + m_bm.bmWidth*i;

	
	// 从 buffBytes 拷贝入 m_ptrPixels 数组
	if (m_bm.bmBitsPixel >= 8)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 8位位图或以上，每像素占1字节或以上
		// m_bm.bmWidthBytes 为一行扫描行的字节数，由于扫描行有字节对齐，这些字节不一定都是一行有效像素的字节
		// m_bm.bmWidth * m_bm.bmBitsPixel / 8 才是一行像素实际所用的字节数
		////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (m_bm.bmWidthBytes == m_bm.bmWidth * m_bm.bmBitsPixel / 8)
		{
			// ======== 每行字节数都是有效像素，直接拷贝入 m_ptrPixels 即可 ========
			memcpy(m_ptrPixels, buffBytes, buffBytesCount);
		}
		else		// if (m_bm.bmWidthBytes == m_bm.bmWidth)
		{
			// ======== 每行字节数 不一定都是有效像素，需分行拷贝 ========
			int iBytesPerLine = buffBytesCount / m_bm.bmHeight;	// 不用 m_bm.bmWidthBytes，buffBytesCount 现已为 GetBitmapBits 实际获得的字节数
			for (r=0; r<m_bm.bmHeight; r++)
			{
				// 每次循环处理一行
				memcpy(m_ptrPixels + r*m_bm.bmWidth, (void *)(buffBytes + r*iBytesPerLine), m_bm.bmWidth* m_bm.bmBitsPixel / 8);
			}	// end for (int r=1; r<=m_bm.bmHeight; r++)

		}			// end if (m_bm.bmWidthBytes == m_bm.bmWidth)
	}
	else				// if (m_bm.bmBitsPixel == 8)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 8位位图以下，每像素占字节数少于 1 个字节
		////////////////////////////////////////////////////////////////////////////////////////////////////////

		for (r=0; r<m_bm.bmHeight; r++)
		{
			// ============ 每次循环处理一行 ============
			int c=0;		// 列 c 从0开始
			// 当前需要的第 r 行的起始像素所在的字节为 buffBytes[i]，求此 i 值
			i = r * buffBytesCount / m_bm.bmHeight;     // 不用 mbm.bmWidthBytes，buffBytesCount 现已为 GetBitmapBits 实际获得的字节数
			
			// 获得第 r 行的所有列的像素值 => m_ptrPixels[r][ ]
			while(c < m_bm.bmWidth)
			{
				int rk = ((1 << m_bm.bmBitsPixel) - 1) << 8;	// 用 & 按位与运算获得一个像素的“位”数据的掩码
				int rkMoveRight=8;								// 用 And 按位与运算获得一个像素的“位”数据后，还要将之“右移”到最低位所需移动的位数。不能用 unsigned char
				while(c < m_bm.bmWidth)
				{
					rkMoveRight -= m_bm.bmBitsPixel; 
					if (rkMoveRight < 0) break;		// 本字节剩余位数不够
					rk >>= m_bm.bmBitsPixel; 
					*(m_ptrPixels + r*m_bm.bmWidth + c) = (buffBytes[i] & rk) >> rkMoveRight;	// m_ptrPixels[r][c]=...
					c = c + 1;
				}

				i++;
			}	// end do while(c<=m_bm.bmWidth)
		}			// end for (r=0; r<m_bm.bmHeight; r++)
	}					// end if (m_bm.bmBitsPixel == 8)

	return true;
}



// 释放资源
// bFreeSzBmpFile==true 时，也释放保存文件名的 m_szBmpFile 的资源；否则不释放 m_szBmpFile，而仅释放其他资源
void CBBMPLoad::ReleaseResource(bool bFreeSzBmpFile/*=true*/)
{
	// 删除位图对象
	if (m_hBitmap) 
	{
		DeleteObject(m_hBitmap);
		m_hBitmap = 0;
	}

	// 删除保存位图文件字符串的 m_szBmpFile
	if (bFreeSzBmpFile)
	{
		if (m_szBmpFile) delete []m_szBmpFile;
		m_szBmpFile = 0;
	}

	// 释放内存资源
	delete [] m_pLines;		 m_pLines = 0;		// 删除位图每行像素的起始地址所在的 指针数组
	delete [] m_ptrPixels;	 m_ptrPixels = 0;	// 删除位图像素数组
	memset(&m_bm, 0, sizeof(m_bm));				// 清除 m_bm
}















//////////////////////////////////////////////////////////////////////
// 私有函数
//////////////////////////////////////////////////////////////////////

// 设置错误信息字符串，自动开辟空间，保存到 m_ErrDesp
void CBBMPLoad::SetErrDesp( LPCTSTR errDesp )
{ 
	if (m_ErrDesp) delete [] m_ErrDesp;				// 清除原有空间

	m_ErrDesp = new TCHAR [lstrlen(errDesp) + 1];		// 开辟保存错误信息的空间（该空间将在对象析构时释放）
	lstrcpy(m_ErrDesp, errDesp);						// 将错误信息保存到新开辟的空间
}

LPTSTR CBBMPLoad::GetErrDesp()
{
	if (m_ErrDesp==0) 
	{
		// 设为空字符串
		m_ErrDesp = new TCHAR [2];
		*m_ErrDesp = 0;
	}
	return m_ErrDesp;
}

unsigned char ** CBBMPLoad::GetBMPBytes()
{
	return m_pLines;	
}


// 返回位图宽度，必须首先 LoadBitmapFile 后才有效
long CBBMPLoad::Width()
{
	return m_bm.bmWidth;
}

// 返回位图高度，必须首先 LoadBitmapFile 后才有效
long CBBMPLoad::Height()
{
	return m_bm.bmHeight;
}

long CBBMPLoad::Type()
{
	return m_bm.bmType;
}

long CBBMPLoad::WidthBytes()
{
	return m_bm.bmWidthBytes;
}

WORD CBBMPLoad::Planes()
{
	return m_bm.bmPlanes;
}

WORD CBBMPLoad::BitsPixel()
{
	return m_bm.bmBitsPixel;
}











