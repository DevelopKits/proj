#ifndef __SCANOPCODE_H__
#define __SCANOPCODE_H__

#include <iostream>
#include <cstdio>
#include <cstdlib>
using namespace std;

#include <windows.h>

union Mem
{
	DWORD date32;
	WORD date16[2];
	BYTE data8[4];
};

/************************************************************************/
/* 函数说明：查找特征码
/* process: 要查找的进程
/* markCode: 特征码字符串,不能有空格
/* distinct：特征码首地址离目标地址的距离 负数在特征码在上
/* offset: 返回目标地址
/* size: 设置返回数据为几个BYTE 1 2 3 4
/* ordinal: 特征码出现的次数
/* beginAddr: 开始搜索地址
/* endAddr: 结束地址
/* ret:返回目标地址的内容
/************************************************************************/
DWORD ScanOpcode(HANDLE process, const char *markCode, 
				  int distinct,
				  LPDWORD offset = NULL,
				  DWORD size = 4,
				  DWORD ordinal = 1,
				  DWORD beginAddr = 0x00400000, DWORD endAddr = 0x7FFFFFFF)
{
	//每次读取游戏内存数目的大小
	const DWORD pageSize = 4096;

	////////////////////////处理特征码/////////////////////
	//特征码长度不能为单数
	if (strlen(markCode) % 2 != 0) return 0;
	//特征码长度
	int len = strlen(markCode) / 2;
	//将特征码转换成byte型
	BYTE *m_code = new BYTE[len];
	memset(m_code,0,len);
	for (int i = 0; i < len; i++){
		char c[] = {markCode[i*2], markCode[i*2+1], '\0'};
		m_code[i] = (BYTE)::strtol(c, NULL, 16);
	}

	/////////////////////////查找特征码/////////////////////
	BOOL _break = FALSE;
	//用来保存在第几页中的第几个找到的特征码
	int curPage = 0;
	int curIndex = 0;
	//每页读取4096个字节
	BYTE *page = new BYTE[pageSize+len-1];
	DWORD tmpAddr = beginAddr;
	DWORD ord = 0;
	while (tmpAddr <= endAddr - len){
		if( ::ReadProcessMemory(process, (LPCVOID)tmpAddr, page, pageSize+len-1, 0) )
		{
			//在该页中查找特征码
			for (int i = 0; i < pageSize; i++){
				for (int j = 0; j < len; j++){
					//如果是??则不处理
					if( m_code[j] == 0 )
					{
						continue;
					}
					//只要有一个与特征码对应不上则退出循环
					if (m_code[j] != page[i + j])break;
					//找到退出所有循环
					if (j == len - 1){
						ord++;
						if(ord != ordinal)
							break;
						_break = TRUE;
						curIndex = i; // 特征码的首地址偏移
						break;
					}
				}
				if (_break) break;
			}
		}

		if (_break) break;
		curPage++;
		tmpAddr += pageSize;
	}
	// 一个也没找到
	if(tmpAddr > endAddr - len)
		return 0;
	// 生成目标地址
	DWORD offsetaddr = curPage * pageSize + curIndex + beginAddr + distinct;
	if(offset != NULL){
		*offset = offsetaddr;
	}
	// 返回地址内容
	DWORD base;
	::ReadProcessMemory(process, (LPVOID)offsetaddr, &base, 4, 0);
	delete m_code;
	delete page;
	return base & 0xFFFFFFFF>>(4-size)*8;
}

// 查找内存地址
DWORD ScanAddr(HANDLE process, const char *markCode,
			   int distinct,
			   DWORD size = 4,
			   DWORD ordinal = 1,
			   DWORD beginAddr = 0x00400000, DWORD endAddr = 0x7FFFFFFF)
{
	DWORD addr = NULL;
	ScanOpcode(process, markCode, distinct, &addr, size, ordinal, beginAddr, endAddr);
	return addr;
}

// 查找立即数
DWORD ScanImme(HANDLE process, const char *markCode,
			   int distinct,
			   DWORD size = 4,
			   DWORD ordinal = 1,
			   DWORD beginAddr = 0x00400000, DWORD endAddr = 0x7FFFFFFF)
{
	return ScanOpcode(process, markCode, distinct, NULL, size, ordinal, beginAddr, endAddr);
}

// 查找地址call
DWORD ScanCall(HANDLE process, const char *markCode,
			   int distinct,
			   DWORD size = 4,
			   DWORD ordinal = 1,
			   DWORD beginAddr = 0x00400000, DWORD endAddr = 0x7FFFFFFF)
{
	DWORD addr = NULL;
	DWORD cont = ScanOpcode(process, markCode, distinct, &addr, size, ordinal, beginAddr, endAddr);
	return addr - 1 + 5 + cont;
}



#endif // __SCANOPCODE_H__