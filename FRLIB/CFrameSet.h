//----------------------------------------------------------------------
// CFrameSet.h
//----------------------------------------------------------------------
//
// 只允许从 Frame Pack 加载特定的 Frame。
//
// 从 Frame Set 文件中读取信息（用于索引）
// 从 Frame Pack 中读取特定的 Frame。
//
// 在Frame Pack中使用Frame Set Index File的信息
// 在特定位置（文件位置）加载帧。
//
//----------------------------------------------------------------------

#ifndef __CFRAMESET_H__
#define __CFRAMESET_H__

#include "CFrame.h"

template <class Type> class CFrameSet {
public:
  CFrameSet();
  ~CFrameSet();

  //--------------------------------------------------------
  // Init/Release
  //--------------------------------------------------------
  void Init(TYPE_FRAMEID count);
  void Release();

  //--------------------------------------------------------
  // file I/O
  //--------------------------------------------------------
  // FramePack File에서 Frame를 Load한다.
  // indexFile = FilePointer File, packFile = FramePack File
  bool LoadFromFile(ifstream &indexFile, ifstream &packFile);

  //--------------------------------------------------------
  // operator
  //--------------------------------------------------------
  Type &operator[](TYPE_FRAMEID n) { return m_pFrames[n]; }

protected:
  TYPE_FRAMEID m_nFrames; // 帧 ID 数
  Type *m_pFrames;        // 类型集
};

//----------------------------------------------------------------------
// CFrameSet.cpp
//----------------------------------------------------------------------

// #include "CFramePack.h"
// #include "CFrameSet.h"

//----------------------------------------------------------------------
//
// constructor/destructor
//
//----------------------------------------------------------------------
template <class Type> CFrameSet<Type>::CFrameSet() {
  m_nFrames = 0;
  m_pFrames = NULL;
}

template <class Type> CFrameSet<Type>::~CFrameSet() {
  // array를 메모리에서 제거한다.
  Release();
}

//----------------------------------------------------------------------
//
// member functions
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Init
//----------------------------------------------------------------------
template <class Type> void CFrameSet<Type>::Init(TYPE_FRAMEID count) {
  // 개수가 없을 경우
  if (count == 0)
    return;

  // 일단 해제
  Release();

  // 메모리 잡기
  m_nFrames = count;

  m_pFrames = new Type[m_nFrames];
}

//----------------------------------------------------------------------
// Release
//----------------------------------------------------------------------
template <class Type> void CFrameSet<Type>::Release() {
  if (m_pFrames != NULL) {
    // 모든 MFrame를 지운다.
    delete[] m_pFrames;
    m_pFrames = NULL;

    m_nFrames = 0;
  }
}

//----------------------------------------------------------------------
// Load From File
//----------------------------------------------------------------------
// FrameSet IndexFile을 이용해서 FramePack File에서
// 특정 위치의 Frame들을 Load한다.
//----------------------------------------------------------------------
template <class Type>
bool CFrameSet<Type>::LoadFromFile(ifstream &indexFile, ifstream &packFile) {
  TYPE_FRAMEID count;

  //------------------------------------------------------
  // FrameSet의 Frame개수를 읽어들인다.
  //------------------------------------------------------
  indexFile.read((char *)&count, SIZE_FRAMEID);

  long *pIndex = new long[count]; // file position

  //------------------------------------------------------
  // FrameSet IndexFile을 모두 읽어들인다.
  //------------------------------------------------------
  for (TYPE_FRAMEID i = 0; i < count; i++) {
    indexFile.read((char *)&pIndex[i], 4);
  }
  // 这里加载可修改为协程await等,
  // 在IO时候是进行了两次循环,第一次循环加载包文件指针位置, 然后第二个循环seekg
  // 再使用对应的farmes loadforfile方法
  // 因为是单线程代码,注定效率不会很高,在面临1-65535超大的包时一定会卡.
  // 只是FR库还不需要优化, Sp库必须改成协程IO,C++14后完全支持协程 不用造轮子.
  // 不然和老旧的开源gm工具没有区别,卡顿难用
  // 单独使用循环的原因是
  // 总觉得同时访问两个文件的话
  // 可能会变慢...真的是这样吗？ - -;;

  //------------------------------------------------------
  // Frame를 Load할 memory를 잡는다.
  //------------------------------------------------------
  Init(count);

  //------------------------------------------------------
  // Index(File Position)를 이용해서 FramePack에서
  // 특정 Frame들을 Load한다.
  //------------------------------------------------------
  for (i = 0; i < count; i++) {
    packFile.seekg(pIndex[i], ios::beg);
    m_pFrames[i].LoadFromFile(packFile);
  }

  delete[] pIndex;

  return true;
}

//----------------------------------------------------------------------
// FrameSet을 define한다. //定义了静态事物框架和生物框架set类
//
//----------------------------------------------------------------------
typedef CFrameSet<FRAME_ARRAY> CThingFrameSet;
typedef CFrameSet<ACTION_FRAME_ARRAY> CCreatureFrameSet;

#endif
