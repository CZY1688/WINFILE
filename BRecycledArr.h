//////////////////////////////////////////////////////////////////////
// 本文件包括两个类的定义：
// CBRecycledArr：带回收站的数组类
// CBRecycledArrInt：整型版带回收站的数组类
//
// 可组织一批数据，数据类型可同时为：
//    一个字符串、两个整数、一个单精度数、一个双精度数
// 可随意 Add、Remove，自动维护 index 为 1～Count 范围，以便通过数组
//   的方式访问各元素；也支持通过遍历的方法访问各元素
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>



//////////////////////////////////////////////////////////////////////
// CBRecycledArr：带回收站的数组类
//
// 可组织一批数据，数据类型可同时为：
//    一个字符串、两个整数、一个单精度数、一个双精度数
// 可随意 Add、Remove，自动维护 index 为 1～Count 范围，以便通过数组
//   的方式访问各元素；也支持通过遍历的方法访问各元素
//////////////////////////////////////////////////////////////////////


// 可能 throw 的错误：
// throw (unsigned char)5;			// 无效的过程调用或参数
class CBRecycledArr  
{
private:
	typedef struct _MemType
	{
		LPTSTR DataString;	// 字符串数据
		int DataInt;		// 附加整型数据1
		int DataInt2;		// 附加整型数据2
		float DataFloat;	// 附加单精度数据
		double DataDouble;	// 附加双精度数据
		bool Used;
	} MemType;
	
	static const int	mcIniMemSize;  			// 初始 mem[] 的大小
	static const int	mcMaxItemCount; 		// 最多元素个数（可扩大此值到 long 表示的范围之内）
	static const float	mcExpandMaxPort;  		// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
	static const int	mcExpandCountThres; 	// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
	static const int	mcExpandCountThresMax;	// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
	static const int	mcExpandBigPer; 		// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
	static const int	mcRecyExpandPer;		// 扩大 recycles[] 空间时，每次扩大的大小
	
private:
	MemType * mem;					// 动态数组指针，但数组不使用 [0] 的元素
	int memCount, memUsedCount;		// 动态数组最大下标，mem[] 数组下标为 [0] ～ [memCount]。已用[0] ～ [memUsedCount]（其中可能有删除的元素，删除的元素下标在 recycles 中保存）
	int * recycles;					// 指向一个整型数组，数组各元素保存 mem[] 中已删除的元素的下标；该数组被维护从小到大排序
									// recycles[] 数组下标为 [0] ～ [recyclesCount]，其中 [0] 不用；用 [1] - [recyUsedCount]，其余为可用空余空间
	int recyCount, recyUsedCount;	// recycles[] 数组元素下标最大值
	int mTravIdxCurr;				// 用 NextXXX 方法遍历的当前 index
	
public:
	CBRecycledArr(int initSize = 0);	// initSize = 0 时，初始开辟 mcIniMemSize 大小的空间，否则开辟 initSize 大小的空间
	~CBRecycledArr(); 
	
	int Add(LPCTSTR dataString = 0, int dataInt = 0, int dataInt2 = 0, float dataFloat = 0.0, double dataDouble = 0.0);  // 添加新数据，返回新数据被保存到的 mem[] 中的下标（>0），出错返回0
	bool Remove(int index);				// 删除一个数据，index 为要删除数据的下标；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count
	LPTSTR Item(int index, bool bRaiseErrIfNotHas=false);		// 根据下标 index，返回一个字符串数据；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count；字符串的内存空间由本类自动管理，主调程序不必干预；如主调程序修改了返回指针的指向内容，则本类对象内部内容也同时发生变化
	int ItemInt(int index, bool bRaiseErrIfNotHas=false);		// 根据下标 index，返回一个整型附加数据；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count
	int ItemInt2(int index, bool bRaiseErrIfNotHas=false);		// 根据下标 index，返回一个整型附加数据2；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count
	float ItemFloat(int index, bool bRaiseErrIfNotHas=false);	// 根据下标 index，返回一个 float 型附加数据；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count
	double ItemDouble(int index, bool bRaiseErrIfNotHas=false);	// 根据下标 index，返回一个 double 型附加数据；删除数据后，后续数据 index 会自动调整；使 index 总为 1～Count
	int Count();						// 返回当前拥有的数据个数
	void Clear();						// 删除所有数据
	
	void StartTraversal();				// 开始一次遍历
	LPTSTR NextItem( bool &bRetNotValid );			// 遍历的下一个数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int NextDataInt( bool &bRetNotValid );			// 遍历的下一个附加整型数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int NextDataInt2( bool &bRetNotValid );			// 遍历的下一个附加整型数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	float NextDataFloat( bool &bRetNotValid );		// 遍历的下一个附加 float 型数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	double NextDataDouble( bool &bRetNotValid );	// 遍历的下一个附加整 double 数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	
private:
	void ExpandMem(void);
	int RedimArrMemType(MemType * &arr, int toUBound=-1, int uboundCurrent=-1, bool preserve=false);		// 重定义 一个 MemType 类型的数组（如可以是 mem[]）的大小，新定义空间自动清零
	int RedimArrInt( int * &arr, int toUBound=-1, int uboundCurrent=-1, bool preserve=false );				// 重定义 一个 int 类型的数组（如可以是 recycles）的大小，新定义空间自动清零
	int FindPosInSortedRecycle(int itemToFind);		// 用二分查找法在数组 recycles 中查找元素 itemToFind 的位置
	int UserIndexToMemIndex(int index);				// 根据用户 index，获得在 mem[] 中的下标；删除数据后，后续数据的用户 index 会自动调整；使用户 index 总为 1～Count
};




//////////////////////////////////////////////////////////////////////
// CBRecycledArrInt：整型版带回收站的数组类
//
// 可组织一批数据，数据类型可同时为：
//    两个整数、一个单精度数、一个双精度数
// 可随意 Add、Remove，自动维护 index 为 1～Count 范围，以便通过数组
//   的方式访问各元素；也支持通过遍历的方法访问各元素
//////////////////////////////////////////////////////////////////////

// 可能 throw 的错误：
// throw (unsigned char)5;			// 无效的过程调用或参数
class CBRecycledArrInt  
{
private:
	typedef int DataType;			// 数据的类型
	typedef int DataIntType;		// 附加整型数据的类型
	typedef float DataFloatType;	// 附加单精度数据的类型
	typedef double DataDoubleType;	// 附加双精度数据的类型
	
	typedef struct _MemType
	{
		DataType Data;
		DataIntType DataInt;
		DataFloatType DataFloat;
		DataDoubleType DataDouble;
		bool Used;
	} MemType;
	
	static const int	mcIniMemSize;  			// 初始 mem[] 的大小
	static const int	mcMaxItemCount; 		// 最多元素个数（可扩大此值到 long 表示的范围之内）
	static const float	mcExpandMaxPort;  		// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
	static const int	mcExpandCountThres; 	// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
	static const int	mcExpandCountThresMax;	// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
	static const int	mcExpandBigPer; 		// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
	static const int	mcRecyExpandPer;		// 扩大 recycles[] 空间时，每次扩大的大小
	
private:
	MemType * mem;					// 动态数组指针，但数组不使用 [0] 的元素
	int memCount, memUsedCount;		// 动态数组最大下标，mem[] 数组下标为 [0] ～ [memCount]。已用[0] ～ [memUsedCount]（其中可能有删除的元素，删除的元素下标在 recycles 中保存）
	int * recycles;					// 指向一个整型数组，数组各元素保存 mem[] 中已删除的元素的下标
	// recycles[] 数组下标为 [0] ～ [recyclesCount]，其中 [0] 不用；用 [1] - [recyUsedCount]，其余为可用空余空间
	int recyCount, recyUsedCount;	// recycles[] 数组元素下标最大值
	int mTravIdxCurr;				// 用 NextXXX 方法遍历的当前 index
	
public:
	CBRecycledArrInt(int initSize = 0);	// initSize = 0 时，初始开辟 mcIniMemSize 大小的空间，否则开辟 initSize 大小的空间
	~CBRecycledArrInt(); 
	
	int Add(DataType data, DataIntType dataInt = 0, DataFloatType dataFloat = 0.0, DataDoubleType dataDouble = 0.0);  // 添加新数据，返回新数据被保存到的 mem[] 中的下标（>0），出错返回0
	bool Remove(int index);				// 删除一个数据，index 为要删除数据的下标。
	DataType Item(int index, bool bRaiseErrIfNotHas=false);				// 根据下标 index，返回一个数据
	DataIntType ItemInt(int index, bool bRaiseErrIfNotHas=false);		// 根据下标 index，返回一个整型附加数据
	DataFloatType ItemFloat(int index, bool bRaiseErrIfNotHas=false);	// 根据下标 index，返回一个 float 型附加数据
	DataDoubleType ItemDouble(int index, bool bRaiseErrIfNotHas=false);	// 根据下标 index，返回一个 double 型附加数据
	int Count();						// 返回当前拥有的数据个数
	void Clear();						// 删除所有数据
	
	void StartTraversal();				// 开始一次遍历
	DataType NextItem( bool &bRetNotValid );				// 遍历的下一个数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	DataIntType NextDataInt( bool &bRetNotValid );			// 遍历的下一个附加整型数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	DataFloatType NextDataFloat( bool &bRetNotValid );		// 遍历的下一个附加 float 型数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	DataDoubleType NextDataDouble( bool &bRetNotValid );	// 遍历的下一个附加整 double 数据，若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	
private:
	void ExpandMem(void);
	int RedimArrMemType(MemType * &arr, int toUBound=-1, int uboundCurrent=-1, bool preserve=false);		// 重定义 一个 MemType 类型的数组（如可以是 mem[]）的大小，新定义空间自动清零
	int RedimArrInt( int * &arr, int toUBound=-1, int uboundCurrent=-1, bool preserve=false );				// 重定义 一个 int 类型的数组（如可以是 recycles）的大小，新定义空间自动清零
	
};

