//////////////////////////////////////////////////////////////////////
// 本文件包含2个类的实现：
// CBRecycledArr 带回收站的数组
// CBRecycledArrInt 整型版带回收站的数组
//
//////////////////////////////////////////////////////////////////////

#include "BRecycledArr.h"
#include <memory.h>
#include <string.h>
#include <tchar.h>


//////////////////////////////////////////////////////////////////////
// CBRecycledArr 类的实现：带回收站的数组
//
//////////////////////////////////////////////////////////////////////

// ===================================================================================
// 字符串数据：
//   在 Add 时才 new 空间，开辟 mem[] 的空间时，各 mem[] 元素的字符串指针不 = new
//   在 Remove 时，delete 空间，然后保存到“回收站”。回收站中的内容字符串指针都为NULL
//   在析构时，delete 空间
// ===================================================================================

//////////////////////////////////////////////////////////////////////
// Static 常量定值
//////////////////////////////////////////////////////////////////////
const int   CBRecycledArr::mcIniMemSize = 7;				// 初始 mem[] 的大小
const int   CBRecycledArr::mcMaxItemCount = 100000000;		// 最多元素个数（由于内存原因，2G内存之内，sizeof(MemType)*此值不得超过2G）
const float CBRecycledArr::mcExpandMaxPort = 0.75;			// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
const int   CBRecycledArr::mcExpandCountThres = 10000;		// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
const int   CBRecycledArr::mcExpandCountThresMax = 10000000;// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int   CBRecycledArr::mcExpandBigPer = 1000000;		// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int	CBRecycledArr::mcRecyExpandPer = 100;			// 扩大 recycles[] 空间时，每次扩大的大小
		

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBRecycledArr::CBRecycledArr(int initSize/*=0*/)
{
	// 初始化成员变量的值
	mem = 0;
	memCount = 0; memUsedCount = 0;
	recycles = 0;
	recyCount = 0; recyUsedCount = 0;
	mTravIdxCurr = 0;

	// 开辟初始空间
	if (initSize)
	{
		// 开辟 initSize 大小的空间
		RedimArrMemType(mem, initSize);
		memCount = initSize;
	}
	else
	{
		// 开辟 mcIniMemSize 大小的空间
		RedimArrMemType(mem, mcIniMemSize);
		memCount = mcIniMemSize;
	}
}

CBRecycledArr::~CBRecycledArr()
{
	Clear();
	// Clear() 函数又自动开辟了 mcIniMemSize 大小的空间，现再删除它即可
	delete []mem; memCount=0;
} 





//////////////////////////////////////////////////////////////////////
// 公有函数
//////////////////////////////////////////////////////////////////////

// 添加新数据，返回新数据被保存到的 mem[] 中的下标（>0），出错返回0
int CBRecycledArr::Add( LPCTSTR dataString /*= NULL*/, int dataInt /*= 0*/, int dataInt2 /*= 0*/, float dataFloat /*= 0.0*/, double dataDouble /*= 0.0*/ )
{
	int idx = 0;  // 要将新数据存入 mem[idx]

	// ======== 为新数据分配新空间，获得空间下标 idx ========
	if (recycles!=0 && recyUsedCount>0)
	{
		// 回收站中有空间，使用回收站中的最后一个空间；回收站剩余部分仍维护着从小到大排序
		idx = recycles [recyUsedCount];
		recyUsedCount--;
	}
	else
	{
		// 回收站中没有空间，使用 mem[idx] 中的下一个可用空间
		if (memUsedCount+1 <= memCount)
		{
			memUsedCount++;
			idx = memUsedCount;
		}
		else
		{
			// Redim preserve mem[1 to memCount + 一批] 
			ExpandMem();
			// 扩大空间后再次判断
			if (memUsedCount+1 <= memCount) 
			{
				memUsedCount++;
				idx = memUsedCount;
			}
			else
			{
				// 扩大空间后还不能 memUsedCount <= memCount，无法分配空间，返回出错
				return 0;		
			}
		}
	}

	// ======== 保存新数据 ========
	if (dataString)
	{
		mem[idx].DataString = new TCHAR [_tcslen(dataString) + 1];
		_tcscpy(mem[idx].DataString, dataString);
	}
	else
	{
		mem[idx].DataString = NULL;
	}
	mem[idx].DataInt = dataInt;
	mem[idx].DataInt2 = dataInt2;
	mem[idx].DataFloat = dataFloat;
	mem[idx].DataDouble = dataDouble;
	mem[idx].Used = true;

	// 返回新数据被保存到的 mem[] 中的下标
	return idx;
}


bool CBRecycledArr::Remove( int index )
{
	int idx, i;

	index=UserIndexToMemIndex(index);
	if (index<1 || index>memCount) return false;	// index 不合法
	if (! mem[index].Used) return false;			// 数据已被删除

	// ======== 设置数据值为空 ========
	if (mem[index].DataString)	// 删除该数据的字符串所占用的空间（如果有的话）
	{
		delete [] mem[index].DataString;
		mem[index].DataString = NULL;
	}
	mem[index].DataInt = 0;
	mem[index].DataInt2 = 0;
	mem[index].DataFloat = 0.0;
	mem[index].DataDouble = 0.0;
	mem[index].Used = false;

	// ======== 在回收站中记录该空间已被删除 ========
	// 在回收站中保存记录 index（维护回收站 recycles[] 数组从小到大排序）

	if (recycles==0 || recyUsedCount + 1 > recyCount)
	{
		// 扩大回收站中的空间
		RedimArrInt(recycles, recyCount+mcRecyExpandPer, recyCount, true);
		recyCount = recyCount+mcRecyExpandPer;
	}
	
	// 使用二分查找找到 index 应被保存到的 recycles[] 下标的负数
	idx=FindPosInSortedRecycle(index);	
	
	// 总数 + 1
	recyUsedCount++;

	// 保存新数据
	if (idx<0)
	{
		// 正常情况，index 应被保存到的 recycles[-idx]
		for(i=recyUsedCount; i>-idx; i++)
			recycles[i]=recycles[i-1];
		recycles[-idx] = index;
	}
	else
	{
		// 不正常（如回收站中已经有了值为 index 的这个元素，或二分查找出错）
		// 容错，仍使用 recycles[idx] 保存 index
		recycles[idx] = index;
	}

	// 返回成功
	return true;
}


LPTSTR CBRecycledArr::Item( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	index=UserIndexToMemIndex(index);
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;	
	}

	return mem[index].DataString;
}


int CBRecycledArr::ItemInt( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	index=UserIndexToMemIndex(index);
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;	
	}

	return mem[index].DataInt;
}

int CBRecycledArr::ItemInt2( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	index=UserIndexToMemIndex(index);
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;	
	}
	
	return mem[index].DataInt2;
}

float CBRecycledArr::ItemFloat( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	index=UserIndexToMemIndex(index);
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;	
	}
	
	return mem[index].DataFloat;
}

double CBRecycledArr::ItemDouble( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	index=UserIndexToMemIndex(index);
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;	
	}
	
	return mem[index].DataDouble;
}



void CBRecycledArr::Clear()
{
	// 先删除所有字符串的空间
	int i;
	for(i=1; i<=memCount; i++)
		if (mem[i].DataString)
		{
			delete [] mem[i].DataString;
			mem[i].DataString = NULL;
		}

	// 删除 mem 本身的空间和 回收站 本身的空间
	if (mem) delete []mem;
	if (recycles) delete []recycles;

	mem = 0; memCount = 0; memUsedCount = 0;
	recycles = 0; recyCount = 0; recyUsedCount = 0;
	mTravIdxCurr = 0;

	// 开辟 mcIniMemSize 大小的空间
	RedimArrMemType(mem, mcIniMemSize);
	memCount = mcIniMemSize;
}



void CBRecycledArr::StartTraversal()
{
	mTravIdxCurr = 1;
}


LPTSTR CBRecycledArr::NextItem( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataString;}
	}

	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0;
}


int CBRecycledArr::NextDataInt( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataInt;}
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0;
}

int CBRecycledArr::NextDataInt2( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataInt2;}
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0;
}

float CBRecycledArr::NextDataFloat( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataFloat;}
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0.0;
}

double CBRecycledArr::NextDataDouble( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataDouble; }
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0.0;
}


// 返回当前拥有的数据个数
int CBRecycledArr::Count()
{
	int ct = memUsedCount-recyUsedCount;
	if (ct<0) ct = 0;
	return ct;
}



//////////////////////////////////////////////////////////////////////
// 私有函数
//////////////////////////////////////////////////////////////////////

void CBRecycledArr::ExpandMem( void )
{
	int toCount = 0;
	toCount=memCount;

	if (toCount < 1) toCount = 1;	// 避免 toCount 为0无法扩大空间
	if (toCount < mcExpandCountThres)
	{
		// 如果数据总数“比较少”，就扩增空间为原来的2倍
		toCount = toCount * 2;
	}
	else if (toCount < mcExpandCountThresMax)
	{
		// 如果数据总数已经“有点多”，就扩增空间为原来的1.5倍
		toCount = toCount * 3 / 2;
	}
	else
	{
		// 如果数据总数“很多”，就扩增 mcExpandBigPer 个空间
		toCount = toCount + mcExpandBigPer;
	}
	
	// 重定义数组大小
	// ReDim Preserve mem(1 To toCount)
	RedimArrMemType(mem, toCount, memCount, true);
	memCount = toCount;
}




// 重定义 一个 MemType 类型的数组（如可以是 recycles）的大小，新定义空间自动清零
// arr：为数组指针，可传递：recycles，本函数将修改此指针的指向
// toUBound：为要重定义后数组的上界，定义为：[0] to [toUBound]，为 -1 时不开辟空间，可用于删除原
//	 空间，并 arr 会被设为0
// uboundCurrent：为重定义前数组的上界 [0] to [uboundCurrent]，为 -1 表示尚未开辟过空间为第一次调用
// preserve：保留数组原始数据否则不保留
// 返回新空间上标，即 toUBound
int CBRecycledArr::RedimArrMemType( MemType * &arr, int toUBound/*=-1*/, int uboundCurrent/*=-1*/, bool preserve/*=false*/ )
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


// 重定义 一个 int 类型的数组（如可以是 mem[]）的大小，新定义空间自动清零
// arr：为数组指针，可传递：mem，本函数将修改此指针的指向
// toUBound：为要重定义后数组的上界，定义为：[0] to [toUBound]，为 -1 时不开辟空间，可用于删除原
//	 空间，并 arr 会被设为0
// uboundCurrent：为重定义前数组的上界 [0] to [uboundCurrent]，为 -1 表示尚未开辟过空间为第一次调用
// preserve：保留数组原始数据否则不保留
// 返回新空间上标，即 toUBound
int CBRecycledArr::RedimArrInt( int * &arr, int toUBound/*=-1*/, int uboundCurrent/*=-1*/, bool preserve/*=false*/ )
{
	// 开辟新空间：[0] to [toUBound]
	if (toUBound >= 0)
	{
		int * ptrNew = new int [toUBound + 1];		// +1 为使可用下标最大到 toUBound
		// 新空间清零
		memset(ptrNew, 0, sizeof(int)*(toUBound + 1));
		
		// 将原有空间内容拷贝到新空间
		if (preserve && arr!=0 && uboundCurrent>=0)
		{
			int ctToCpy;										// 保留原有数据，需要拷贝内存的 int 元素个数
			ctToCpy = uboundCurrent;
			if (uboundCurrent>toUBound) ctToCpy = toUBound;		// 取 uboundCurrent 和 toUBound 的最小值
			ctToCpy = ctToCpy + 1;								// 必须 +1，因为 uboundCurrent 和 toUBound 都是数组上界
			memcpy(ptrNew, arr, sizeof(int)*ctToCpy); 
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



// 用二分查找法在数组 recycles 中查找元素 itemToFind 的位置
// 数组 recycles 已按从小到大排序；数据下标要从1开始, recyUsedCount 同时为下标上界
// 函数返回值：
//   1. 如找到元素在数组中存在，返回存在的下标（>0）；
//   2. 如不存在要找的元素，函数返回负数（<0），其绝对值为维护排序该数据要被插入的位置
//      (若要插入的位置在最后一个元素之后，函数返回的负数的绝对值为原最大下标+1)
//   3. 出错返回 0
int CBRecycledArr::FindPosInSortedRecycle( int itemToFind )
{
	int iStart=1, iEnd=recyUsedCount;
	int i;
	
	if (recyUsedCount<1) return -1;	// 数组还没有元素，返回-1，表示第一个数据要被插入到下标为 [1] 的位置（数组下标从1开始）
	while(iStart<=iEnd)
	{
		i = iStart+(iEnd-iStart)/2;
		if (itemToFind > recycles[i])
			iStart = i + 1;
		else if(itemToFind < recycles[i])
			iEnd = i - 1;
		else
			return i;		// 找到元素，下标为 i
	}
	
	// 没有找到，返回要插入的位置：直接返回 iEnd+1 的负数即可
	return -(iEnd+1);
}


// 根据用户 index，获得在 mem[] 中的下标；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count
// 出错返回 0
int CBRecycledArr::UserIndexToMemIndex( int index )
{
	// 获得在 回收站 中记录的那些下标中，比 index 小的有几个 => i
	int i=FindPosInSortedRecycle(index);
	if (i<0) 
		i=-i-1;
	// else 直接返回 index+i
	return index+i;
}








//////////////////////////////////////////////////////////////////////
// CBRecycledArrInt 类的实现：整型版带回收站的数组
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Static 常量定值
//////////////////////////////////////////////////////////////////////
const int   CBRecycledArrInt::mcIniMemSize = 7;					// 初始 mem[] 的大小
const int   CBRecycledArrInt::mcMaxItemCount = 100000000;		// 最多元素个数（由于内存原因，2G内存之内，sizeof(MemType)*此值不得超过2G）
const float CBRecycledArrInt::mcExpandMaxPort = 0.75;			// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
const int   CBRecycledArrInt::mcExpandCountThres = 10000;		// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
const int   CBRecycledArrInt::mcExpandCountThresMax = 10000000;// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int   CBRecycledArrInt::mcExpandBigPer = 1000000;			// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int	CBRecycledArrInt::mcRecyExpandPer = 100;			// 扩大 recycles[] 空间时，每次扩大的大小
		

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBRecycledArrInt::CBRecycledArrInt(int initSize/*=0*/)
{
	// 初始化成员变量的值
	mem = 0;
	memCount = 0; memUsedCount = 0;
	recycles = 0;
	recyCount = 0; recyUsedCount = 0;
	mTravIdxCurr = 0;

	// 开辟初始空间
	if (initSize)
	{
		// 开辟 initSize 大小的空间
		RedimArrMemType(mem, initSize);
		memCount = initSize;
	}
	else
	{
		// 开辟 mcIniMemSize 大小的空间
		RedimArrMemType(mem, mcIniMemSize);
		memCount = mcIniMemSize;
	}
	
}

CBRecycledArrInt::~CBRecycledArrInt()
{
	if (mem) delete []mem;
	mem = 0; memCount = 0; memUsedCount = 0;
	if (recycles) delete []recycles;
	recycles = 0; recyCount = 0; recyUsedCount = 0;

	mTravIdxCurr = 0;
} 





//////////////////////////////////////////////////////////////////////
// 公有函数
//////////////////////////////////////////////////////////////////////

// 添加新数据，返回新数据被保存到的 mem[] 中的下标（>0），出错返回0
int CBRecycledArrInt::Add( DataType data, DataIntType dataInt /*= 0*/, DataFloatType dataFloat /*= 0.0*/, DataDoubleType dataDouble /*= 0.0*/ )
{
	int idx = 0;  // 要将新数据存入 mem[idx]

	// 为新数据分配新空间，获得空间下标 idx
	if (recycles!=0 && recyUsedCount>0)
	{
		// 回收站中有空间，使用回收站中的最后一个空间
		idx = recycles [recyUsedCount];
		recyUsedCount--;
	}
	else
	{
		// 回收站中没有空间，使用 mem[idx] 中的下一个可用空间
		if (memUsedCount+1 <= memCount)
		{
			memUsedCount++;
			idx = memUsedCount;
		}
		else
		{
			// Redim preserve mem[1 to memCount + 一批] 
			ExpandMem();
			// 扩大空间后再次判断
			if (memUsedCount+1 <= memCount) 
			{
				memUsedCount++;
				idx = memUsedCount;
			}
			else
			{
				// 扩大空间后还不能 memUsedCount <= memCount，无法分配空间，返回出错
				return 0;		
			}
		}
	}

	// 保存新数据
	mem[idx].Data = data;
	mem[idx].DataInt = dataInt;
	mem[idx].DataFloat = dataFloat;
	mem[idx].DataDouble = dataDouble;
	mem[idx].Used = true;

	// 返回新数据被保存到的 mem[] 中的下标
	return idx;
}


bool CBRecycledArrInt::Remove( int index )
{
	if (index<1 || index>memCount) return false;	// index 不合法
	if (! mem[index].Used) return false;			// 数据已被删除

	// 设置数据值为空
	mem[index].Data = 0;
	mem[index].DataInt = 0;
	mem[index].DataFloat = 0.0;
	mem[index].DataDouble = 0.0;
	mem[index].Used = false;

	// 在回收站中记录该空间已被删除
	if (recycles==0 || recyUsedCount + 1 > recyCount)
	{
		// 扩大回收站中的空间
		RedimArrInt(recycles, recyCount+mcRecyExpandPer, recyCount, true);
		recyCount = recyCount+mcRecyExpandPer;
	}
	recyUsedCount++;
	recycles[recyUsedCount] = index;

	// 返回成功
	return true;
}


CBRecycledArrInt::DataType CBRecycledArrInt::Item( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;	
	}

	return mem[index].Data;
}

CBRecycledArrInt::DataIntType CBRecycledArrInt::ItemInt( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0;	
	}

	return mem[index].DataInt;
}

CBRecycledArrInt::DataFloatType CBRecycledArrInt::ItemFloat( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;	
	}
	
	return mem[index].DataFloat;
}

CBRecycledArrInt::DataDoubleType CBRecycledArrInt::ItemDouble( int index, bool bRaiseErrIfNotHas/*=false*/ )
{
	if (index<1 || index>memCount)	// index 不合法
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;
	}
	if (! mem[index].Used) 				// 数据已被删除
	{
		if (bRaiseErrIfNotHas) throw (unsigned char)5;			// 无效的过程调用或参数
		return 0.0;	
	}
	
	return mem[index].DataDouble;
}



void CBRecycledArrInt::Clear()
{
	if (mem) delete []mem;
	if (recycles) delete []recycles;

	mem = 0; memCount = 0; memUsedCount = 0;
	recycles = 0; recyCount = 0; recyUsedCount = 0;
	mTravIdxCurr = 0;

	// 开辟 mcIniMemSize 大小的空间
	RedimArrMemType(mem, mcIniMemSize);
	memCount = mcIniMemSize;
}



void CBRecycledArrInt::StartTraversal()
{
	mTravIdxCurr = 1;
}


CBRecycledArrInt::DataType CBRecycledArrInt::NextItem( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].Data;}
	}

	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0;
}


CBRecycledArrInt::DataIntType CBRecycledArrInt::NextDataInt( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataInt;}
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0;
}

CBRecycledArrInt::DataFloatType CBRecycledArrInt::NextDataFloat( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataFloat;}
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0.0;
}

CBRecycledArrInt::DataDoubleType CBRecycledArrInt::NextDataDouble( bool &bRetNotValid )
{
	bRetNotValid = true;	// 预设返回数据成功
	for (; mTravIdxCurr<=memCount; mTravIdxCurr++)
	{
		if (mem[mTravIdxCurr].Used) {mTravIdxCurr++; return mem[mTravIdxCurr-1].DataDouble; }
	}
	
	// 返回数据结束，此时函数返回值无效
	bRetNotValid = false;	
	return 0.0;
}


// 返回当前拥有的数据个数
int CBRecycledArrInt::Count()
{
	int ct = memUsedCount-recyUsedCount;
	if (ct<0) ct = 0;
	return ct;
}



//////////////////////////////////////////////////////////////////////
// 私有函数
//////////////////////////////////////////////////////////////////////

void CBRecycledArrInt::ExpandMem( void )
{
	int toCount = 0;
	if (memCount < mcExpandCountThres)
	{
		// 如果数据总数“比较少”，就扩增空间为原来的2倍
		toCount = memCount * 2;
	}
	else if (memCount < mcExpandCountThresMax)
	{
		// 如果数据总数已经“有点多”，就扩增空间为原来的1.5倍
		toCount = memCount * 3 / 2;
	}
	else
	{
		// 如果数据总数“很多”，就扩增 mcExpandBigPer 个空间
		toCount = memCount + mcExpandBigPer;
	}
	
	// 重定义数组大小
	// ReDim Preserve mem(1 To toCount)
	RedimArrMemType(mem, toCount, memCount, true);
	memCount = toCount;
}




// 重定义 一个 MemType 类型的数组（如可以是 recycles）的大小，新定义空间自动清零
// arr：为数组指针，可传递：recycles，本函数将修改此指针的指向
// toUBound：为要重定义后数组的上界，定义为：[0] to [toUBound]，为 -1 时不开辟空间，可用于删除原
//	 空间，并 arr 会被设为0
// uboundCurrent：为重定义前数组的上界 [0] to [uboundCurrent]，为 -1 表示尚未开辟过空间为第一次调用
// preserve：保留数组原始数据否则不保留
// 返回新空间上标，即 toUBound
int CBRecycledArrInt::RedimArrMemType( MemType * &arr, int toUBound/*=-1*/, int uboundCurrent/*=-1*/, bool preserve/*=false*/ )
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


// 重定义 一个 int 类型的数组（如可以是 mem[]）的大小，新定义空间自动清零
// arr：为数组指针，可传递：mem，本函数将修改此指针的指向
// toUBound：为要重定义后数组的上界，定义为：[0] to [toUBound]，为 -1 时不开辟空间，可用于删除原
//	 空间，并 arr 会被设为0
// uboundCurrent：为重定义前数组的上界 [0] to [uboundCurrent]，为 -1 表示尚未开辟过空间为第一次调用
// preserve：保留数组原始数据否则不保留
// 返回新空间上标，即 toUBound
int CBRecycledArrInt::RedimArrInt( int * &arr, int toUBound/*=-1*/, int uboundCurrent/*=-1*/, bool preserve/*=false*/ )
{
	// 开辟新空间：[0] to [toUBound]
	if (toUBound >= 0)
	{
		int * ptrNew = new int [toUBound + 1];		// +1 为使可用下标最大到 toUBound
		// 新空间清零
		memset(ptrNew, 0, sizeof(int)*(toUBound + 1));
		
		// 将原有空间内容拷贝到新空间
		if (preserve && arr!=0 && uboundCurrent>=0)
		{
			int ctToCpy;										// 保留原有数据，需要拷贝内存的 int 元素个数
			ctToCpy = uboundCurrent;
			if (uboundCurrent>toUBound) ctToCpy = toUBound;		// 取 uboundCurrent 和 toUBound 的最小值
			ctToCpy = ctToCpy + 1;								// 必须 +1，因为 uboundCurrent 和 toUBound 都是数组上界
			memcpy(ptrNew, arr, sizeof(int)*ctToCpy); 
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


