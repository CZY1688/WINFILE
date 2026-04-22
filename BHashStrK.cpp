//////////////////////////////////////////////////////////////////////
// CBHashStrK: 字符串型键值的哈希表类的实现
//
//////////////////////////////////////////////////////////////////////

#include "BHashStrK.h"
#include <memory.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////
// Static 常量定值
//////////////////////////////////////////////////////////////////////
const int   CBHashStrK::mcIniMemSize = 7;				// 初始 mem[] 的大小
const int   CBHashStrK::mcMaxItemCount = 100000000;		// 最多元素个数（可扩大此值到 long 表示的范围之内）
const float CBHashStrK::mcExpandMaxPort = 0.75;			// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
const int   CBHashStrK::mcExpandCountThres = 10000;		// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
const int   CBHashStrK::mcExpandCountThresMax = 10000000;	// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int   CBHashStrK::mcExpandBigPer = 1000000;			// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int   CBHashStrK::mcExpandMem2Per = 10;				// 每次扩大 mem2[] 的大小
const int   CBHashStrK::mcSeqMax = 5;						// 顺序检索最大值


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CBHashStrK::CBHashStrK(int memSize/*=0*/)
{
	mArrTable=0; mArrTableCount = -1;	// 无缓存数组

	memUsedCount = 0; 
	mem2 = 0;
	memCount2 = 0;
    memUsedCount2 = 0;
	KeyCaseSensitive=false;			// 是否 key 要区分大小写（默认不分大小写）

	if (memSize)
	{
		// 初始定义 memSize 个的 mem[] 空间，无 mem2[] 的空间
		RedimArrMemType(mem, memSize);
		memCount = memSize;
	}
	else
	{
		// 初始定义 mcIniMemSize 个的 mem[] 空间，无 mem2[] 的空间
		RedimArrMemType(mem, mcIniMemSize);
		memCount = mcIniMemSize;
	}
}

CBHashStrK::~CBHashStrK()
{
	Clear();

	// Clear() 函数中，重新开辟了初始大小的 mem[] 空间，再将其删除即可
	if (mem) delete[] mem;
	memCount = 0;
}



//////////////////////////////////////////////////////////////////////
// 公有方法
//////////////////////////////////////////////////////////////////////

void CBHashStrK::AlloMem(int memSize )
{
	/*
	程序初始化时只定义了 mcIniMemSize 大小的 l_Mem[]，以后随使用随自动扩 \
	  大；但若事先知道有多大，可以先用本函数定义足够大以免以后不断 \
	  自动扩大费时；注意这时要比预用的元素个数多定义一些，否则分配空间 \
	  时若空间冲突本类还会自动扩大
	此函数也可用于截断 l_Mem[] 后面没有使用的空间
      注：memSize <= memUsedCount 时，拒绝重新定义，以确保数据不会丢失 
	*/

	if (memSize <= memUsedCount || memSize > mcMaxItemCount) return;
	int iPreMemCount;
	iPreMemCount = memCount;

	// ReDim Preserve mem(1 To memSize)
	RedimArrMemType(mem, memSize, memCount, true);
	memCount = memSize;

	if (iPreMemCount <= memCount) ReLocaMem(iPreMemCount); else ReLocaMem(memCount);

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;

	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;
}

bool CBHashStrK::Add( int data, LPCTSTR key/*=0*/, long dataLong/*=0*/, long dataLong2/*=0*/, LPCTSTR dataStr/*=NULL*/, LPCTSTR dataStr2/*=NULL*/, double dataDouble/*=0.0*/, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	
	// 哈希表中的数据个数最多不能超过 mcMaxItemCount
	if (memUsedCount + memUsedCount2 >= mcMaxItemCount)
    {		
		if (raiseErrorIfNotHas)  throw (unsigned char)7;	// 超出内存
		return false;
	}
	
	// 当前哈希表中不能有相同的“键”存在
	if (IsKeyExist(key))
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)5;	// 无效的过程调用或参数
		return false;
	}
	
	// 通过调用 AlloMemIndex 函数获得一个可用空间的下标：idx
	idx = AlloMemIndex(key);
	
	if (idx > 0)
	{
		// 获得的下标值为正数时，使用 mem[] 数组的空间
		mem[idx].Data = data;
		mem[idx].DataLong = dataLong;
		mem[idx].DataLong2 = dataLong2;
		mem[idx].DataDouble = dataDouble;

		SaveItemString(&(mem[idx].Key), key);		// 保存 key 字符串，动态分配一个空间来保存，新空间地址保存到 mem[idx].Key；如果 mem[idx].Key 现在值不为 0，该函数还兼有先 delete mem[idx].Key 现在指向的空间的功能
		SaveItemString(&(mem[idx].DataStr), dataStr);
		SaveItemString(&(mem[idx].DataStr2), dataStr2);
		
		mem[idx].Used = true;

		memUsedCount = memUsedCount + 1;
	}
	else if (idx < 0)
	{
		// 获得的下标值为负数时，使用 mem2[] 数组的空间，_
		// 空间下标为 idx 的绝对值
		mem2[-idx].Data = data;
		mem2[-idx].DataLong = dataLong;
		mem2[-idx].DataLong2 = dataLong2;
		mem2[-idx].DataDouble = dataDouble;

		SaveItemString(&(mem2[-idx].Key), key);		// 保存 key 字符串，动态分配一个空间来保存，新空间地址保存到 mem[idx].Key；如果 mem[idx].Key 现在值不为 0，该函数还兼有先 delete mem[idx].Key 现在指向的空间的功能
		SaveItemString(&(mem2[-idx].DataStr), dataStr);
		SaveItemString(&(mem2[-idx].DataStr2), dataStr2);

		mem2[-idx].Used = true;

		memUsedCount2 = memUsedCount2 + 1;
	}
	else // idx == 0
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)9;	// 下标越界：无法分配新数据空间
	}
	
	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;
	
	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;

	// 函数返回成功
    return true;
}


int CBHashStrK::Item( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].Data;
	else if (idx < 0)
		return mem2[-idx].Data;
	else
		return 0;

}


long CBHashStrK::ItemLong( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataLong;
	else if (idx < 0)
		return mem2[-idx].DataLong;
	else
		return 0;

}


long CBHashStrK::ItemLong2( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataLong2;
	else if (idx < 0)
		return mem2[-idx].DataLong2;
	else
		return 0;

}

double CBHashStrK::ItemDouble( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataDouble;
	else if (idx < 0)
		return mem2[-idx].DataDouble;
	else
		return 0;

}

LPTSTR CBHashStrK::ItemStr( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataStr;
	else if (idx < 0)
		return mem2[-idx].DataStr;
	else
		return 0;

}

LPTSTR CBHashStrK::ItemStr2( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataStr2;
	else if (idx < 0)
		return mem2[-idx].DataStr2;
	else
		return 0;

}


// 判断一个 Key 是否在当前集合中存在
bool CBHashStrK::IsKeyExist( LPCTSTR key )
{
	int idx;
	idx = GetMemIndexFromKey(key, false);
    return (idx != 0);
}

bool CBHashStrK::Remove( LPCTSTR key, bool raiseErrorIfNotHas/*=True*/ )
{
	int idx;
	
	// 调用 GetMemIndexFromKey 函数获得“键”为 Key 的数据所在空间的下标
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx == 0)
		return false;
	else if (idx > 0)
	{
		// 哈希表中“键”为 Key 的数据在 mem[] 数组中，下标为 idx
		mem[idx].Used = false;
		SaveItemString(&(mem[idx].Key), 0);	// 第二个参数为0，删除 mem[idx].Key 指向的空间，并让 mem[idx].Key=0;
		SaveItemString(&(mem[idx].DataStr), 0); 
		SaveItemString(&(mem[idx].DataStr2), 0); 

		memUsedCount = memUsedCount - 1;
	}
	else
	{
		// idx<0 表示：哈希表中“键”为 Key 的数据在 mem2[] 数组中 \
		// 下标为 idx 的绝对值
		// 删除下标为“-idx”的元素
		SaveItemString(&(mem[-idx].Key), 0);	// 第二个参数为0，删除 mem[-idx].Key 指向的空间，并让 mem[-idx].Key=0;
		SaveItemString(&(mem[-idx].DataStr), 0); 
		SaveItemString(&(mem[-idx].DataStr2), 0); 

		for(int i=-idx; i<= memUsedCount2-1; i++)
			mem2[i] = mem2[i+1];
		mem2[memUsedCount2].Key=0;			// 直接设置为0，因此空间地址已经传递给上一元素
		mem2[memUsedCount2].DataStr=0;		// 直接设置为0，因此空间已经传递给上一元素
		mem2[memUsedCount2].DataStr2=0;		// 直接设置为0，因此空间已经传递给上一元素

		memUsedCount2 = memUsedCount2 - 1;
	}

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;

	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;

	// 函数返回成功
	return true;
}

void CBHashStrK::StartTraversal()
{
	// 开始用 NextXXX ... 方法遍历
    mTravIdxCurr = 1;
}


int CBHashStrK::NextItem( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 Data
    // 若 bRetNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].Data;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].Data;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

long CBHashStrK::NextItemLong( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataLong;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataLong;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}



long CBHashStrK::NextItemLong2( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataLong2;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataLong2;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

double CBHashStrK::NextItemDouble( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataDouble;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataDouble;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

LPTSTR CBHashStrK::NextItemStr( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataStr;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataStr;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

LPTSTR CBHashStrK::NextItemStr2( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataStr2;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataStr2;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}


// 返回的字符串指针，直接指向类内部的 key 数据；因此注意：如若修改了该指针指向的内容，
//   则类内数据的 key 也同时发生变化
LPTSTR CBHashStrK::NextKey( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 String
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].Key;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].Key;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

// 清除所有元素，重定义 mcIniMemSize 个存储空间
void CBHashStrK::Clear( void )
{
	// 清除	按 Index 访问各元素的缓存数组
	if (mArrTable) {delete []mArrTable; mArrTable=0;}
	mArrTableCount=-1;

	// 删除 mem[] 和 mem2[] 中的每个元素的 key 和 字符串数据 指向的空间
	int i;
	for (i=1; i<=memCount; i++)
	{
		if (mem[i].Key) {delete [] mem[i].Key; mem[i].Key=0; }
		if (mem[i].DataStr) {delete [] mem[i].DataStr; mem[i].DataStr=0; }
		if (mem[i].DataStr2) {delete [] mem[i].DataStr2; mem[i].DataStr2=0; }
	}
	for (i=1; i<=memCount2; i++)
	{
		if (mem2[i].Key) {delete [] mem2[i].Key; mem2[i].Key=0; }
		if (mem2[i].DataStr) {delete [] mem2[i].DataStr; mem2[i].DataStr=0; }
		if (mem2[i].DataStr2) {delete [] mem2[i].DataStr2; mem2[i].DataStr2=0; }
	}

	// 删除 mem[] 和 mem2[] 的空间
	delete [] mem; mem=0;
	delete [] mem2; mem2=0;
	memCount = 0; memUsedCount = 0;
	memCount2 = 0; memUsedCount2 = 0;

	// 重新开辟空间
	RedimArrMemType(mem, mcIniMemSize, memCount, false);
	memCount = mcIniMemSize;

	mTravIdxCurr = 0;
}


// 返回共有元素个数
int CBHashStrK::Count( void )
{
	return memUsedCount + memUsedCount2;
}




//////////////////////////////////////////////////////////////////////
// 私有方法
//////////////////////////////////////////////////////////////////////

void CBHashStrK::ReLocaMem( int preMemCountTo )
{
	/*
	重新分配 mem[], mem2[] 的各元素的空间，mem2[] 的某些元素可能被 \
	重新移动到 mem
	将修改 memUsedCount,memUsedCount2, memCount2, mem2[] 的值
	preMemCountTo 只考虑 mem[1 to preMemCountTo]，preMemCountTo 以后的元素被认为 \
	未用，不考虑；但无论如何都考虑 mem2[] 中的所有元素
	*/
	
	// 将 mem[] 中的已使用元素和 mem2[] 中的所有元素先放入 memUsed[] 中， \
	// 把 memUsed[] 定义为足够大，实际 memUsed[] 只使用了 lngUsedCount 个元素
	MemType * memUsed;
	RedimArrMemType(memUsed, preMemCountTo + memUsedCount2);

	int iUsedCount=0;		
	int i;

	// 将 mem[] 中已使用的元素存入 memUsed[]
	for (i=1; i<=preMemCountTo; i++)
		if (mem[i].Used)
		{
			iUsedCount = iUsedCount + 1;
			memUsed[iUsedCount] = mem[i];
		}

	// 将 mem2[] 中的所有元素存入 memUsed[]
	for (i=1; i<=memUsedCount2; i++)
	{
		iUsedCount = iUsedCount + 1;
		memUsed[iUsedCount] = mem2[i];
	}


	/*
	此时 memUsed[1 To lngUsedCount] 中为所有 mem[] \
	中的已使用元素 和 mem2[] 中的所有元素
	*/

	// 清空 mem，也清空了所有 Used 域
	RedimArrMemType(mem, memCount, memCount, false); 
	memUsedCount=0;  // memUsedCount 置0，后面随移动随+1

	// 清空 mem2，也清空了所有 Used 域
	RedimArrMemType(mem2, -1, memCount2, false);
	memCount2 = 0;
	memUsedCount2 = 0; 

	// 逐个把 memUsed[1 To lngUsedCount] 中的元素按新数组大小映射下标存入 mem[]
	int idx;
	for (i=1; i<=iUsedCount; i++)
	{
		idx = AlloMemIndex(memUsed[i].Key, false);
		if (idx > 0)
		{
			mem[idx] = memUsed[i];
			mem[idx].Used = 1;
			memUsedCount = memUsedCount + 1;
		}
		else
		{
			mem2[-idx] = memUsed[i];
			mem2[-idx].Used = 1;
			memUsedCount2 = memUsedCount2 + 1;
		}
	}

	// 删除临时空间 memUsed
	delete [] memUsed; memUsed=0;

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;
	
	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;
}


// 重定义 mem[] 数组大小，扩大 mem[] 的空间
void CBHashStrK::ExpandMem( void )
{
	int iCount, iPreMemCount;

	// 计算哈希表中共有数据总数
	iCount = memUsedCount + memUsedCount2;

	// 取“共有数据总数”和“当前 mem[] 的空间总数”两者的较大值
	if (iCount < memCount) iCount = memCount;

	// 保存扩增空间之前的、原来的 mem[] 的空间总数
	iPreMemCount = memCount;

	if (iCount<1) iCount=1;		// 避免 iCount 为0时，无法扩大空间
	if (iCount < mcExpandCountThres)
	{
		// 如果数据总数“比较少”，就扩增空间为原来的2倍
		iCount = iCount * 2;

	}
	else if (iCount < mcExpandCountThresMax)
	{
		// 如果数据总数已经“有点多”，就扩增空间为原来的1.5倍
		iCount = iCount * 3 / 2;
	}
	else
	{
		// 如果数据总数“很多”，就扩增 mcExpandBigPer 个空间
		iCount = iCount + mcExpandBigPer;
	}

	// 重定义数组大小
	// ReDim Preserve mem(1 To lngCount)
	RedimArrMemType(mem, iCount, memCount, true);
	memCount = iCount;

	// 按新数组大小，重新安排其中所有数据的新位置，参数中要传递
	// 扩增空间之前的、原来的 mem[] 的空间总数
	ReLocaMem(iPreMemCount);

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;
	
	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;
}


// 遍历哈希表，将数据存入 mArrTable()，设置 mArrTableCount 为数据个数（返回成功或失败）
bool CBHashStrK::RefreshArrTable()
{
	int iCount;
	int i,j;
	
	// 计算哈希表中共有数据总数
	iCount = memUsedCount + memUsedCount2;
	
	mArrTableCount=iCount;
	if (mArrTableCount<=0) return false;
	
	if (mArrTable) {delete []mArrTable; mArrTable=0;}
	mArrTable=new int [iCount+1];	// 使数组下标从1开始
	memset(mArrTable, 0, sizeof(int)*(iCount+1));
	
	j=1;
	for (i=1; i<=memCount; i++)
	{
		if (mem[i].Used)
		{
			if (j > iCount) return false;
			
			mArrTable[j] = i;		// 将 哈希表的本数据所在的 mem[] 的下标 i 存入 mArrTable[j]
			mem[i].Index = j;		// 在 哈希表的本数据 的 Index 成员中记录 mArrTable 的下标 j
			
			j=j+1;
		}
	}
	
	for (i=1; i<=memUsedCount2; i++)
	{
		if (mem2[i].Used)
		{
			if (j > iCount) return false;
			
			mArrTable[j] = -i;		// 将 哈希表的本数据所在的 mem2[] 的下标 i （取负）存入 mArrTable[j]
			mem[i].Index = j;		// 在 哈希表的本数据 的 Index 成员中记录 mArrTable 的下标 j
			
			j=j+1;
		}
	}
	
	return true;
}


int CBHashStrK::AlloSeqIdx( int fromIndex, int toIndex )
{
    /*
	 找 mem[] 中一个没使用的空间，从 fromIndex 开始， \
		到 toIndex 结束
	 返回 mem[] 的一个没使用元素的下标，没找到返回 0
	*/
	int i;
	if (fromIndex <= 0)  fromIndex = 1;
	if (toIndex > memCount) toIndex = memCount;
	
	for (i=fromIndex; i<=toIndex; i++)
		if (! mem[i].Used) return i; 

	return 0;
}

int CBHashStrK::AlloMemIndex( LPCTSTR key, bool CanExpandMem/*=true */ )
{
	/* 
	  根据 Key 分配一个 mem[] 中的未用存储空间，返回 mem[] 数组下标
		返回负值表不能在 mem[] 中找到空间：返回值的绝对值为 mem2[] 的 \
		下一个可用下标空间（mem2[]自动Redim），以存入 mem2[]
	  本函数确保返回一个可使用的空间，最差情况返回 mem2[] 中的空间
	  另：本函数不修改 memUsedCount2 的值，但 redim mem2[]
		CanExpandMem=true 时，允许本函数自动扩大 mem[]，否则不会自动扩大
		
		方法：将字符串的 key 转换为整数，如果为负数先取绝对值，然后：
		1. 先用 Key Mod memCount + 1，此 Index -> idxMod
		2. 若上面的元素已经使用，则看Key是否 < cMaxNumForSquare (sqr(2^31)=46340) \
		若 <，则平方 Key，然后 mod memCount + 1； \
		若 >=，则用按位和移位运算，后相加 key 的各个部分，然后 mod memCount + 1
		无论哪种情况，此步 Index -> idxSq
		3. 用 memCount-idxMod+1 -> idxModRev
		4. 用 memCount-idxSq+1 -> idxSqRev
		5. 若上面找到的 Index 都被使用了，则看 Count 是否 > \
		mcExpandMaxPort*Count，若是，若 CanExpandMem=true， \
		则扩大 mem[] 的存储空间，然后递归本过程，重复 1-4 步
		6. 用 idxMod+1,+2,...,+mcSeqMax；用 idxMod-1,-2,...,-mcSeqMax
		7. 再没有，返回负值，绝对值为 mem2[] 的下一个可用空间，以存入 mem2[]
	*/


	const int cMaxNumForSquare = 46340;
	
	int idxMod=0, idxSq=0;
    int idxModRev=0, idxSqRev=0;
    int iCount=0;
    long keyToCalc = KeyStringToLong(key); // 计算用 Key，永远为>0的数
    if (keyToCalc < 0) keyToCalc = 0 - keyToCalc;	// 如果 Key 是负值，则转换为正数计算它的存储空间
    iCount = memUsedCount + memUsedCount2;
    
	if (memCount) 
	{	
		// 1: 先用 Key Mod memCount + 1，此 Index -> idxMod
		idxMod = keyToCalc % memCount + 1;
		if (! mem[idxMod].Used) return idxMod;
    
		// 2: 用 平方Key 后再除法取余，此 Index -> idxSq
		if (keyToCalc <= cMaxNumForSquare)
		{
			idxSq = (keyToCalc * keyToCalc) % memCount + 1;
		}
		else
		{
			int kBitSum=0;
			kBitSum = (keyToCalc & 0xFFFF0000)>>16;
			kBitSum += (keyToCalc & 0xFF00)>>8;
			kBitSum += (keyToCalc & 0xF0)>>4;
			kBitSum += (keyToCalc & 0xF);
			idxSq = kBitSum % memCount + 1;
		}
		if (! mem[idxSq].Used) return idxSq;

		// 3: 尝试倒数第 idxMod 个空间 -> idxModRev
		idxModRev = memCount - idxMod + 1; 
		if (! mem[idxModRev].Used) return idxModRev;
    
		// 4: 尝试倒数第 idxSq 个空间 -> idxSqRev
		idxSqRev = memCount - idxSq + 1;
		if (! mem[idxSqRev].Used) return idxSqRev;
	}
    
    // 5: 如果空间使用百分比超过阈值，就扩大 mem[] 的 空间
    if (CanExpandMem && iCount > mcExpandMaxPort * memCount)
	{
		ExpandMem();  // 扩大 mem[] 的空间
		return AlloMemIndex(key, CanExpandMem); // 递归，重复1-4步
	}
        
    
    int lngRetIdx;
    
    // 6: 从 idxMod 开始向前、向后线性搜索 mcSeqMax 个空间
    int idxMdSta, idxMdEnd; 
    idxMdSta = idxMod - mcSeqMax; idxMdEnd = idxMod + mcSeqMax;
    lngRetIdx = AlloSeqIdx(idxMdSta, idxMod - 1); 
    if (lngRetIdx > 0) return lngRetIdx;
    lngRetIdx = AlloSeqIdx(idxMod + 1, idxMdEnd);
    if (lngRetIdx > 0) return lngRetIdx; 
    

    // 7: 返回负值，绝对值为 mem2[] 的下一个元素，以存入 mem2[]
    if (memUsedCount2 + 1 > memCount2)
    {    
        // ReDim Preserve mem2(1 To mcExpandMem2Per)
		RedimArrMemType(mem2, memCount2 + mcExpandMem2Per, memCount2, true);
		memCount2 = memCount2 + mcExpandMem2Per;
	}

    return -(memUsedCount2 + 1);
}


int CBHashStrK::FindSeqIdx( LPCTSTR key, int fromIndex, int toIndex )
{
	
    // 找 mem[] 中键为Key的元素下标，从 fromIndex 开始， \
	//	到 toIndex 结束
	//	返回 mem[] 的找到键的下标（>0），没找到返回 0

	int i;
	if (fromIndex < 1) fromIndex = 1;
	if (toIndex > memCount) toIndex = memCount;

	for (i=fromIndex; i<=toIndex; i++)
		if ((mem[i].Used) && CompareKey(mem[i].Key, key) )
			return i;
	
	return 0;
}




int CBHashStrK::TraversalGetNextIdx( void )
{
	// 用 NextXXX 方法遍历时，返回下一个（Next）的 mem[]下标（返回值>0）， \
	// 或 mem2[] 的下标（返回值<0），或已遍历结束（返回值=0）
	
	int iRetIdx;
	
	if (mTravIdxCurr > memCount ||
		-mTravIdxCurr > memCount2 ||
		mTravIdxCurr == 0) return 0;
	
	if (mTravIdxCurr > 0)
	{
		//////////// 在 mem[] 中找 ////////////
		while (! mem[mTravIdxCurr].Used)
		{
			mTravIdxCurr = mTravIdxCurr + 1;
			if (mTravIdxCurr > memCount) break;
		}
		
		if (mTravIdxCurr > memCount)
		{
			//// 已遍历结束，看若 mem2[] 中还有数据继续遍历 mem2[] ////
			if (memCount2 > 0)
			{
				// 设置下次遍历 mem2[] 中数据的下标的负数
				mTravIdxCurr = -1;
				// 执行下面的 if mTravIdxCurr < 0 Then 
			}
			else
			{
				// 返回结束
				iRetIdx = 0;
				return iRetIdx;
			}
		}
		else
		{
			//// 返回 mTravIdxCurr ////
			iRetIdx = mTravIdxCurr;
			// 调整下次遍历指针 指向下一个位置（或是 mem[] 的下一个， \
			// 或是 mem2[] 的起始）
			mTravIdxCurr = mTravIdxCurr + 1;
			if (mTravIdxCurr > memCount) if (memCount2 > 0) mTravIdxCurr = -1;
			return iRetIdx;
		}
	}
	
	if (mTravIdxCurr < 0)
	{
		//////////// 在 mem2[] 中找 ////////////
		while (! mem2[-mTravIdxCurr].Used)
		{	
			mTravIdxCurr = mTravIdxCurr - 1;
			if (-mTravIdxCurr > memCount2) break;
		}
		
		if (-mTravIdxCurr > memCount2)
		{
			//// 已遍历结束 ////
			// 返回结束
			iRetIdx = 0; 
		}
		else
		{
			// 返回负值的 mTravIdxCurr
			iRetIdx = mTravIdxCurr;
			// 调整 mTravIdxCurr 的指针
			mTravIdxCurr = mTravIdxCurr - 1;
		}
		return iRetIdx;
	}
	
	return 0;
}




// 重定义 一个 MemType 类型的数组（如可以是 mem[] 或 mem2[]）的大小，新定义空间自动清零
// arr：为数组指针，可传递：mem 或 mem2，本函数将修改此指针的指向
// toUBound：为要重定义后数组的上界，定义为：[0] to [toUBound]，为 -1 时不开辟空间，可用于删除原
//	 空间，并 arr 会被设为0
// uboundCurrent：为重定义前数组的上界 [0] to [uboundCurrent]，为 -1 表示尚未开辟过空间为第一次调用
// preserve：保留数组原始数据否则不保留
// 返回新空间上标，即 toUBound
int CBHashStrK::RedimArrMemType( MemType * &arr, int toUBound/*=-1*/, int uboundCurrent/*=-1*/, bool preserve/*=false*/ )
{
	// 开辟新空间：[0] to [toUBound]
	if (toUBound >= 0)
	{
		MemType * ptrNew = new MemType [toUBound + 1];		// +1 为使可用下标最大到 toUBound
		// 新空间清零
		memset(ptrNew, 0, sizeof(MemType)*(toUBound + 1));
		
		// 将原有空间内容拷贝到新空间
		if (preserve && arr!=0 && uboundCurrent>=0)
		{
			int ctToCpy;										// 保留原有数据，需要拷贝内存的 MemType 元素个数
			ctToCpy = uboundCurrent;
			if (uboundCurrent>toUBound) ctToCpy = toUBound;		// 取 uboundCurrent 和 toUBound 的最小值
			ctToCpy = ctToCpy + 1;								// 必须 +1，因为 uboundCurrent 和 toUBound 都是数组上界
			memcpy(ptrNew, arr, sizeof(MemType)*ctToCpy); 
		}

		// 删除原有空间
		if (arr!=0 && uboundCurrent>=0) delete [] arr;

		// 指针指向新空间
		arr = ptrNew;
		return toUBound;
	}
	else		// if (toUBound < 0)，不开辟空间，删除原有空间
	{
		if(arr!=0 && uboundCurrent>=0) delete [] arr;
		arr = 0;
		return 0;
	}
	
}


// 用 new 开辟新字符串空间，把 ptrNewString 指向的字符串拷贝到新空间；
//   ptrSaveTo 是一个保存字符串地址的指针变量的地址，其指向的指针变量将保存
//   “用 new 开辟的新字符串空间的地址”，即让 “*ptrSaveTo = 新空间地址”
// 兼有释放“*ptrSaveTo”所指向的空间的功能
//   如 ptrSaveTo 参数可被传递 &(mem[i].key) 即指针的指针；ptrNewString 可被传递新的 key
//   以完成“mem[i].key=key”的操作，本函数修改 mem[i].key 的内容：
//   先删除它旧指向的空间，再让它指向新空间
// 如果 key 为空指针，仅释放“*ptrSaveTo”所指向的空间
void CBHashStrK::SaveItemString( TCHAR ** ptrSaveTo, LPCTSTR ptrNewString )
{
	// 注意 ptrSaveTo 是个二级指针
	if (ptrSaveTo==0) return;  // 没有保存的位置
	
	// 如果 ptrSaveTo 指向的指针变量不为“空指针”，表示要保存之处已正
	//   保存着一个以前开辟的空间地址，应先删除以前开辟的空间
	if (*ptrSaveTo != 0) {delete [] (*ptrSaveTo); *ptrSaveTo=0; }

	if (ptrNewString)
	{
		// 开辟新空间，保存 ptrNewString 这个字符串到新空间
		TCHAR * p = new TCHAR [_tcslen(ptrNewString)+1];
		_tcscpy(p, ptrNewString);
		
		// 使 *ptrSaveTo 指向新空间
		*ptrSaveTo = p;
	}
}


// 从 Key 获得数据在 mem[] 中的下标（返回值>0）或在 mem2[] 中的下标（返回值<0），出错返回 0
int CBHashStrK::GetMemIndexFromKey( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	const long cMaxNumForSquare = 46340;  // sqrt(2^31)=46340
    
    int idxMod=0, idxSq=0;
    int idxModRev=0, idxSqRev=0;
    long keyToCalc = KeyStringToLong(key); // 计算用 Key，永远为>=0的数
    if (keyToCalc < 0) keyToCalc = 0 - keyToCalc;

	if (memCount)
	{
		// 1: 先用 Key Mod memCount + 1，此 Index -> idxMod
		idxMod = keyToCalc % memCount + 1;
		if (mem[idxMod].Used && CompareKey(mem[idxMod].Key, key)) 
			return idxMod;
    
		// 2: 用 平方Key后再除法取余，此 Index -> idxSq
		if (keyToCalc <= cMaxNumForSquare)
		{
			idxSq = (keyToCalc * keyToCalc) % memCount + 1;
		}
		else
		{
			int kBitSum=0;
			kBitSum = (keyToCalc & 0xFFFF0000)>>16;
			kBitSum += (keyToCalc & 0xFF00)>>8;
			kBitSum += (keyToCalc & 0xF0)>>4;
			kBitSum += (keyToCalc & 0xF);
			idxSq = kBitSum % memCount + 1;
		}
		if (mem[idxSq].Used && CompareKey(mem[idxSq].Key, key))
			return idxSq;
    
		// 3: 尝试倒数第 idxMod 个空间 -> idxModRev
		idxModRev = memCount - idxMod + 1;
		if (mem[idxModRev].Used && CompareKey(mem[idxModRev].Key, key))
		   return idxModRev;
    
		// 4: 尝试倒数第 idxSq 个空间 -> idxSqRev
		idxSqRev = memCount - idxSq + 1;
		if (mem[idxSqRev].Used && CompareKey(mem[idxSqRev].Key, key))
		   return idxSqRev;
	}

    int lngRetIdx;
    
    // 6: 从 idxMod 开始向前、向后线性搜索 mcSeqMax 个空间
    int idxMdSta, idxMdEnd;
    idxMdSta = idxMod - mcSeqMax; idxMdEnd = idxMod + mcSeqMax;
    lngRetIdx = FindSeqIdx(key, idxMdSta, idxMod - 1);
    if (lngRetIdx > 0)  return  lngRetIdx;
    lngRetIdx = FindSeqIdx(key, idxMod + 1, idxMdEnd);
    if (lngRetIdx > 0)  return  lngRetIdx;

    // 7: 再查看 mem2[] 中的元素有没有
    for (int i=1; i<=memUsedCount2; i++)
        if (mem2[i].Used && CompareKey(mem2[i].Key, key)) return -i;
	
	if (raiseErrorIfNotHas) throw (unsigned char)5;	// 无效的过程调用或参数
	return 0;
}

// 从 index 获得数据在 mem[] 中的下标（返回值>0）或在 mem2[] 中的下标（返回值<0），出错返回 0
int CBHashStrK::GetMemIndexFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	if (mArrTableCount != memUsedCount + memUsedCount2) RefreshArrTable(); // 刷新数组缓冲
	if (index<1 || index>mArrTableCount)
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)5;	// 无效的过程调用或参数
		return 0;
	}
	
	int idx=mArrTable[index];
	if (idx==0) 
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)7;	// 超出内存（mArrTable[index]意外为0）
		return 0;
	}
	else
		return idx;
}


// 将一个 字符串类型的 key 转换为一个长整数
// 方法是：取前16个字符、后16个字符，将这32个字符的ASCII码相加求和返回
long CBHashStrK::KeyStringToLong( LPCTSTR key )
{
	// 方法是：取前16个字符、后16个字符，将这32个字符的ASCII码相加求和返回
    const int c_CalcCharNumHead = 16;
    const int c_CalcCharNumTail = 16;

	long lResult=0;
	int iHeadToPos=0;	// 实际要取前几个字符
	int iTailToPos=0;	// 实际要取后几个字符
	int i;

	TCHAR * keyLoc = new TCHAR[_tcslen(key)+1];		// 计算用 key
	_tcscpy(keyLoc, key);							// 拷贝 key 字符串到新开辟的空间做副本，因 key 可能要被按照 KeyCaseSensitive 属性转换大小写
	if (! KeyCaseSensitive)  _tcsupr(keyLoc);		// 如果不区分大小写，将 key 转换为 大写
	
	// 初始化累加 ASCII 码的结果
    lResult = 0;

	// 累加前 c_CalcCharNumHead 个字符的 ASCII 码（不足 c_CalcCharNumHead 的做到字符串结束为止）
	iHeadToPos = c_CalcCharNumHead;
	if ((int)_tcslen(keyLoc) < iHeadToPos) iHeadToPos = _tcslen(keyLoc);
	for(i=0; i<iHeadToPos; i++)
		lResult += *(keyLoc+i);

	// 累加后 c_CalcCharNumTail 个字符的 ASCII 码（不足 c_CalcCharNumTail 的做到字符串结束为止）
	iTailToPos = c_CalcCharNumTail;
	if ((int)_tcslen(keyLoc) - iHeadToPos < iTailToPos) iTailToPos = _tcslen(keyLoc) - iHeadToPos;
	for(i = (int)_tcslen(keyLoc) - iTailToPos; i<(int)_tcslen(keyLoc); i++)
		lResult += *(keyLoc+i);

	// 释放空间
	delete []keyLoc;


	// 返回结果：累加的 ASCII 码
	return lResult;
}


// 根据 lKeyCaseSensitive 属性，比较两 key 的字符串是否相等
// 相等返回True，不等返回False
bool CBHashStrK::CompareKey(LPCTSTR key1, LPCTSTR key2 )
{
	if (key1==0 || key2==0) return false;
	if (KeyCaseSensitive)
		return lstrcmp(key1, key2)==0;
	else
	{
		return lstrcmpi(key1, key2)==0;
	}
}


// ---------------- 以 Index 返回数据属性(包括Key，但Key为只读) ----------------
// 注：随着数据增删，Index 可能会变化。某数据的 Index 并不与数据一一对应

int CBHashStrK::ItemFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].Data;
	else if (ii < 0)
		return mem2[-ii].Data;
	else
		return 0;
}

long CBHashStrK::ItemLongFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataLong;
	else if (ii < 0)
		return mem2[-ii].DataLong;
	else
		return 0;
}

long CBHashStrK::ItemLong2FromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataLong2;
	else if (ii < 0)
		return mem2[-ii].DataLong2;
	else
		return 0;		
}

double CBHashStrK::ItemDoubleFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataDouble;
	else if (ii < 0)
		return mem2[-ii].DataDouble;
	else
		return 0;
}

LPTSTR CBHashStrK::ItemStrFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataStr;
	else if (ii < 0)
		return mem2[-ii].DataStr;
	else
		return 0;	
}

LPTSTR CBHashStrK::ItemStr2FromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataStr2;
	else if (ii < 0)
		return mem2[-ii].DataStr2;
	else
		return 0;
}

LPTSTR CBHashStrK::IndexToKey( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].Key;
	else if (ii < 0)
		return mem2[-ii].Key;
	else
		return 0;		
}


int CBHashStrK::KeyToIndex( LPCTSTR key, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	if (mArrTableCount != memUsedCount + memUsedCount2) RefreshArrTable(); // 刷新数组缓冲
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].Index;
	else if (ii < 0)
		return mem2[-ii].Index;
	else
		return 0;		
}



// ---------------- 以 Key 设置数据属性 ----------------

bool CBHashStrK::ItemSet( LPCTSTR key, int vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].Data = vNewValue;
	else if (ii < 0)
		mem2[-ii].Data = vNewValue;
	return true;	
}

bool CBHashStrK::ItemLong2Set( LPCTSTR key, long vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong2 = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong2 = vNewValue;
	return true;	
}

bool CBHashStrK::ItemLongSet( LPCTSTR key, long vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong = vNewValue;
	return true;	
}

bool CBHashStrK::ItemDoubleSet( LPCTSTR key, double vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataDouble = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataDouble = vNewValue;
	return true;	
}

bool CBHashStrK::ItemStrSet( LPCTSTR key, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr), vNewValue);
	return true;	
}

bool CBHashStrK::ItemStr2Set( LPCTSTR key, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr2), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr2), vNewValue);
	return true;	
}


// ---------------- 以 Index 设置数据属性(Key为只读不能设置Key) ----------------
// 注：随着数据增删，Index 可能会变化。某数据的 Index 并不与数据一一对应

bool CBHashStrK::ItemFromIndexSet( int index, int vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].Data = vNewValue;
	else if (ii < 0)
		mem2[-ii].Data = vNewValue;
	return true;	
}

bool CBHashStrK::ItemLongFromIndexSet( int index, long vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong = vNewValue;
	return true;	
}

bool CBHashStrK::ItemLong2FromIndexSet( int index, long vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong2 = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong2 = vNewValue;
	return true;	
}

bool CBHashStrK::ItemDoubleFromIndexSet( int index, double vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataDouble = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataDouble = vNewValue;
	return true;	
}

bool CBHashStrK::ItemStrFromIndexSet( int index, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr), vNewValue);
	return true;	
}

bool CBHashStrK::ItemStr2FromIndexSet( int index, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr2), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr2), vNewValue);
	return true;
}





