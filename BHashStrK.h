
// ===============================================================
// CBHashStrK：字符串型 key 的哈希表类
// 支持每个元素中有 1 个 long 型的数据、 2 个 long 型的附加数据、
//    1 个 double 型的附加数据 和 2 个 字符串的附加数据
//
// 可能抛出的异常：
// throw (unsigned char)7;	// 超出内存
// throw (unsigned char)5;	// 无效的过程调用或参数：键值错误，
//                          // 如试图添加已存在的同样键值的新元素，访问键不存在的元素
// throw (unsigned char)9;	// 下标越界：无法分配新数据空间
//
// ---- 可以使用 Index 的方式遍历所有哈希表元素 ----
//     for (i = 1; i<=hash.Count; i++)
//         cout<<hash.KeyFromIdx(i)
//     
//     for (i = 1; i<=hash.Count; i++)
//         cout<<hash.ItemFromIdx(i)
//     
// 注意 Index 并不与数据对应，随着增删和重排，数据的 Index 都可能会变化
// 但在同一时刻，Index 相同的一套数据（Key,Data,DataLong,DataString）是同一套
// ===============================================================

#pragma once
#pragma warning(disable:4996) // 关闭 warning C4996: 如 'wcscpy': This function or variable may be unsafe. Consider using wcscpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.

#include <windows.h>
#include <tchar.h>



class CBHashStrK
{
private:
	typedef struct _MemType
	{
		LPTSTR Key;			// 指向本类管理的用 new 开辟的一块内存区，每个 mem[] 在被删除时，先 delete [] .key
		int Data;
		long DataLong;
		long DataLong2;
		double DataDouble;
		LPTSTR DataStr;
		LPTSTR DataStr2;
		bool Used;
		int Index;		// mArrTable[] 数组的 mArrTable[index] 元素，是保存本 MemType 数据
						//   所在的 mem[] 中的下标（index>0）或在 mem2[] 中的下标（index<0）
						//   mArrTableCount == memUsedCount + memUsedCount2 时且 index !=0 时 有效
						//   在 RefreshArrTable 中设置此成员

	} MemType;
	
	static const int	mcIniMemSize;  			// 初始 mem[] 的大小
	static const int	mcMaxItemCount; 		// 最多元素个数（可扩大此值到 long 表示的范围之内）
	static const float	mcExpandMaxPort;  		// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
	static const int	mcExpandCountThres; 	// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
	static const int	mcExpandCountThresMax;	// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
	static const int	mcExpandBigPer; 		// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
	static const int	mcExpandMem2Per; 		// 每次扩大 mem2[] 的大小
	static const int	mcSeqMax; 				// 顺序检索最大值

private:
	MemType * mem;					// 动态数组指针，但数组不使用 [0] 的元素
	int memCount, memUsedCount;		// 动态数组最大下标，mem[] 数组下标为 [0] ～ [memCount]。哈希表已用元素个数
	MemType * mem2;					// 空间冲突的元素的保存空间，顺序使用
	int memCount2, memUsedCount2;	// mem2[] 数组下标为 [0] ～ [memCount2]，其中 [0] 不用，已使用空间为 [1] ～ [memUsedCount2]
	int mTravIdxCurr;				// 用 NextXXX 方法遍历的当前 index，正值表示 mem[] 中的下标，负值表示 mem2[] 中的下标
	
	// 支持通过给定下标 Index 访问一个哈希表数据，mArrTable 指向动态数组，
	//   数组元素保存：所有哈希表数据依遍历顺序所在的 mem[] 中的下标（>0）
	//   或 mem2[] 中的下标。
	// 遍历一次，将所有哈希表数据 mem[] 或 mem2[] 的下标存于此数组，
	//   以后不需重复遍历，直接通过给定下标 Index 访问一个哈希表数据。
	//   mArrTableCount != memUsedCount + memUsedCount2 为标志，
	//   如 !=，标志要重新刷新遍历。注意重新遍历后，元素顺序可能会重排。
	//   各哈希表数据的 Index 并不是一直不变的
	int * mArrTable;
	int mArrTableCount;

public:
	bool KeyCaseSensitive;			// 是否 key 要区分大小写（默认不分大小写）

public:
	CBHashStrK(int memSize=0);		// memSize=0 则开辟初始 mcIniMemSize 个空间，否则开辟 memSize 个空间，memSize 应比实际数据个数大一些
	~CBHashStrK();
	void AlloMem(int memSize);		// 事先可用此函数定义足够大的空间，以减少以后自动扩大空间的次数，提高效率
	bool Add(int data, LPCTSTR key=0, long dataLong=0, long dataLong2=0, LPCTSTR dataStr=NULL, LPCTSTR dataStr2=NULL, double dataDouble=0.0, bool raiseErrorIfNotHas=true);	// 添加元素
	bool Remove(LPCTSTR key, bool raiseErrorIfNotHas=true);					// 删除元素
	
	// 根据 key 获得元素、附加数据
	int Item(LPCTSTR key, bool raiseErrorIfNotHas=true);
	long ItemLong(LPCTSTR key, bool raiseErrorIfNotHas=true);
	long ItemLong2(LPCTSTR key, bool raiseErrorIfNotHas=true);
	double ItemDouble(LPCTSTR key, bool raiseErrorIfNotHas=true);
	LPTSTR ItemStr(LPCTSTR key, bool raiseErrorIfNotHas=true);
	LPTSTR ItemStr2(LPCTSTR key, bool raiseErrorIfNotHas=true);

	// 根据 key 设置元素、附加数据
	bool ItemSet(LPCTSTR key, int vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemLongSet(LPCTSTR key, long vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemLong2Set(LPCTSTR key, long vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemDoubleSet(LPCTSTR key, double vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemStrSet(LPCTSTR key, LPCTSTR vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemStr2Set(LPCTSTR key, LPCTSTR vNewValue, bool raiseErrorIfNotHas=true);


	// 根据 index 获得元素、附加数据
	int ItemFromIndex(int index, bool raiseErrorIfNotHas=true);	
	long ItemLongFromIndex(int index, bool raiseErrorIfNotHas=true);
	long ItemLong2FromIndex(int index, bool raiseErrorIfNotHas=true);
	double ItemDoubleFromIndex(int index, bool raiseErrorIfNotHas=true);
	LPTSTR ItemStrFromIndex(int index, bool raiseErrorIfNotHas=true);
	LPTSTR ItemStr2FromIndex(int index, bool raiseErrorIfNotHas=true);

	// 根据 index 设置元素、附加数据（但不能设置 Key，Key为只读）
	bool ItemFromIndexSet(int index, int vNewValue, bool raiseErrorIfNotHas=true);	
	bool ItemLongFromIndexSet(int index, long vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemLong2FromIndexSet(int index, long vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemDoubleFromIndexSet(int index, double vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemStrFromIndexSet(int index, LPCTSTR vNewValue, bool raiseErrorIfNotHas=true);
	bool ItemStr2FromIndexSet(int index, LPCTSTR vNewValue, bool raiseErrorIfNotHas=true);

	LPTSTR IndexToKey(int index, bool raiseErrorIfNotHas=true);
	int KeyToIndex(LPCTSTR key, bool raiseErrorIfNotHas=true);


	bool IsKeyExist(LPCTSTR key);			// 判断某个 key 的元素是否存在
	void Clear(void);						// 清除所有元素，重定义 mcIniMemSize 个存储空间
	
	void StartTraversal();					// 开始一个遍历过程
	int NextItem(bool &bRetNotValid);		// 遍历过程开始后，不断调用此函数，获得每个元素，直到 bRetNotValid 返回 true
	long NextItemLong(bool &bRetNotValid);	// 遍历过程开始后，不断调用此函数，获得每个元素的附加数据，直到 bRetNotValid 返回 true
	long NextItemLong2(bool &bRetNotValid);
	double NextItemDouble(bool &bRetNotValid);
	LPTSTR NextItemStr(bool &bRetNotValid);
	LPTSTR NextItemStr2(bool &bRetNotValid);

	LPTSTR NextKey(bool &bRetNotValid);		// 遍历过程开始后，不断调用此函数，获得每个元素的 key，直到 bRetNotValid 返回 true

	int Count(void);						// 返回共有元素个数


private:
	int AlloMemIndex(LPCTSTR Key, bool CanExpandMem=true );		// 根据 Key 分配一个 mem[] 中的未用存储空间，返回 mem[] 数组下标
	int FindSeqIdx(LPCTSTR key, int fromIndex, int toIndex);	// 找 mem[] 中键为 key 的元素下标，仅查找空间下标为从 fromIndex 开始、到 toIndex 结束的空间
	void ReLocaMem(int preMemCountTo);							// 重新分配 mem[], mem2[] 的各元素的地址，mem2[] 的某些元素可能被重新移动到 mem[]
	void ExpandMem(void);										// 重定义 mem[] 数组大小，扩大 mem[] 的空间
	int TraversalGetNextIdx(void);								// 用 NextXXX 方法遍历时，返回下一个（Next）的 mem[]下标（返回值>0），或 mem2[] 的下标（返回值<0），或已遍历结束（返回值=0）
	int AlloSeqIdx(int fromIndex, int toIndex);					// 找 mem[] 中一个没使用的空间，仅查找空间下标为从 fromIndex 开始、到 toIndex 结束的空间
	bool RefreshArrTable();										// 遍历哈希表，将数据下标存入 mArrTable[]，设置 mArrTableCount 为数据个数（返回成功或失败）

	int RedimArrMemType(MemType * &arr, int toUBound=-1, int uboundCurrent=-1, bool preserve=false);	// 重定义 一个 MemType 类型的数组（如可以是 lMem[] 或 lMem2[]）的大小，新定义空间自动清零

	int GetMemIndexFromKey(LPCTSTR key, bool raiseErrorIfNotHas=true);	// 从 Key 获得数据在 mem[] 中的下标（返回值>0）或在 mem2[] 中的下标（返回值<0），出错返回 0
	int GetMemIndexFromIndex(int index, bool raiseErrorIfNotHas=true);	// 从 index 获得数据在 mem[] 中的下标（返回值>0）或在 mem2[] 中的下标（返回值<0），出错返回 0

	long KeyStringToLong(LPCTSTR key);		// 将一个 字符串类型的 key 转换为一个长整数
	void SaveItemString(TCHAR ** ptrSaveTo, LPCTSTR ptrNewString);			// 用 new 开辟新字符串空间，把 key 指向的字符串拷贝到新空间；ptrSaveTo 是一个保存字符串地址的指针变量的地址，其指向的指针变量将保存“用 new 开辟的新字符串空间的地址”，即让 “*ptrSaveTo = 新空间地址”
	bool CompareKey(LPCTSTR key1, LPCTSTR key2);		// 根据 KeyCaseSensitive 属性，比较两 key 的字符串是否相等：相等返回True，不等返回False
};

