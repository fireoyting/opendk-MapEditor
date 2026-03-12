#ifndef __CTYPEPACKVECTOR_H__
#define __CTYPEPACKVECTOR_H__
//一个vector pack。
// 模组化， 用来读取简单的结构比如vector<alphaSprite>;vector<Palette>到内存
// 方便修改 导出
#include <Windows.h>
#include "SP_PCH.h"
#include <string>
using std::wstring;
template <class TYPE>
class CTypePackVector
{
public:
    CTypePackVector() {}
    ~CTypePackVector() { Release(); }
    DINT        GetSize() const { return m_vData.size(); }
    void Release()
    {
        if (!m_vData.empty()) {
            for (unsigned int i = 0; i < GetSize(); i++)//解决编译警告 添加unsigned int作为循环
            {
                //m_vData[i]->Release();
                delete m_vData[i];//释放每个资源
            }
            m_vData.clear();
            m_vData.shrink_to_fit();
        }


    }
    TYPE* operator [] (WORD n) { return m_vData[n]; }
    TYPE* GetData(WORD n) { return m_vData[n]; }



    void    AddData(TYPE* data) { m_vData.push_back(data); }
    void insertData(int index) { std::swap(&m_vData[index], m_vData.back()); m_vData.pop_back(); }
    void    RemoveData(WORD dataID) { delete m_vData[dataID]; m_vData.erase(&m_vData[dataID]); }
    bool LoadFromFile(ifstream& file);
    bool SaveToFile(ofstream& dataFile, ofstream& indexFile);

    bool LoadFromFile(wchar_t* lpszFilename);//w_char
    bool SaveToFile(PWSTR lpszFilename);

protected:
    vector<TYPE*>   m_vData;

};


template<class TYPE>
bool CTypePackVector<TYPE>::LoadFromFile(wchar_t* lpszFilename)
{
    ifstream file(lpszFilename, ifstream::binary);
    bool re = LoadFromFile(file);
    file.close();

    return re;

}


template<class TYPE>
bool CTypePackVector<TYPE>::LoadFromFile(ifstream& file)
{

    WORD m_Size = 0;
    file.read((char*)&m_Size, 2);

    register int i;

    for (i = 0; i < m_Size; i++)
    {
        TYPE* data = new TYPE;
        m_vData.push_back(data);
        data->LoadFromFile(file);
    }

    return true;




}

template<class TYPE>
bool CTypePackVector<TYPE>::SaveToFile(ofstream& dataFile, ofstream& indexFile)
{

    vector<unsigned int> Vindex;
    DINT m_size = GetSize();
    dataFile.write((const char*)&m_size, 2);
    indexFile.write((const char*)&m_size, 2);
    DINT m_indexi = 0;

    for (DINT i = 0; i < m_size; i++)
    {
        m_indexi = dataFile.tellp();
        m_vData[i]->SaveToFile(dataFile);
        Vindex.push_back(m_indexi);

    }
    for (DINT i = 0; i < Vindex.size(); i++)
    {
        indexFile.write((const char*)&Vindex[i], 4);
    }


    return true;
}



template<class TYPE>
bool CTypePackVector<TYPE>::SaveToFile(PWSTR lpszFilename)
{
    PWSTR lpszindexFilename = new WCHAR[60];
    wcscpy_s(lpszindexFilename, 60, lpszFilename);
    wcscat_s(lpszindexFilename, 60, L"i");
    ofstream dataFile(lpszFilename, ofstream::binary);
    ofstream indexFile(lpszindexFilename, ofstream::binary);

    bool re = SaveToFile(dataFile, indexFile);

    dataFile.close();
    indexFile.close();

    return re;

}


#endif
