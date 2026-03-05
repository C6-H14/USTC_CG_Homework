// implementation of class DArray
#include "DArray.h"
#include <assert.h>
#include <iostream>
using namespace std;
// default constructor
DArray::DArray() {
	Init();
}

// initilize the array
void DArray::Init() {
    m_nMax=0;
    m_nSize=0;
    m_pData=nullptr;
}

// free the array
void DArray::Free() {
    m_nMax=0;
    m_nSize=0;
    m_pData.reset();
}

// allocate enough memory
void DArray::Reserve(int nSize){
    if (m_nMax>=nSize){
        return;
    }
    if (m_nMax<=0) m_nMax=1;
    while (m_nMax<nSize)
    {
        assert(m_nMax>=0);
        m_nMax=(int)(m_nMax*increaseNum);
    }
    unique_ptr<double[]> newPointer=make_unique<double[]>(m_nMax);
    assert(newPointer!=nullptr);

    if (newPointer==nullptr){
        return;
    }
    for (int i=0;i<m_nSize;++i){
        newPointer[i]=m_pData[i];
    }
    m_pData=move(newPointer);
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
    assert(nSize>=0);

    if (nSize<=0){
        Free();
        return;
    }
    Reserve(nSize);
    assert(m_pData!=nullptr);
    if (m_pData==nullptr){
        Free();
        return;
    }
    for (int i=0;i<nSize;++i){
        m_pData[i]=dValue;
    }
}

DArray::DArray(const DArray& arr) {
    if (arr.m_nSize<=0){
        Free();
        return;
    }
    Reserve(arr.m_nSize);
    assert(m_pData!=nullptr);
    if (m_pData==nullptr){
        Free();
        return;
    }
    for (int i=0;i<arr.m_nSize;++i){
        m_pData[i]=arr.m_pData[i];
    }
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
    cout<<m_nSize<<endl;
    for (int i=0;i<m_nSize;++i)
        cout<<GetAt(i)<<' ';
    cout<<endl;
}

// get the size of the array
int DArray::GetSize() const {
	if (m_nSize<=0) return 0;
    return m_nSize;
}

// set the size of the array
void DArray::SetSize(int nSize) {
    assert(nSize>=0);

    if (nSize<=0){
        Free();
    }
    else{
        Reserve(nSize);
        for (int i=m_nSize;i<nSize;++i)
            m_pData[i]=0;
        m_nSize=nSize;
    }
}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
    assert(nIndex>=0 && nIndex<=m_nSize);

    static double errorValue=0.0;
    if (nIndex>=m_nSize || nIndex<0){
        return errorValue;
    }
	return m_pData[nIndex];
}

// set the value of an element 
void DArray::SetAt(int nIndex, double dValue) {
    assert(nIndex>=0 && nIndex<=m_nSize);

    if (nIndex>=m_nSize || nIndex<0){
        return;
    }
    m_pData[nIndex]=dValue;
}

// overload operator '[]'
double& DArray::operator[](int nIndex) {
	assert(nIndex >= 0 && nIndex < m_nSize);

    static double errorValue=0.0;
    if (nIndex>=m_nSize || nIndex<0){
        return errorValue;
    }
	return m_pData[nIndex];
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
	assert(nIndex >= 0 && nIndex < m_nSize);

    static double errorValue=0.0;
    if (nIndex>=m_nSize || nIndex<0){
        return errorValue;
    }
	return m_pData[nIndex];
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
    int oldSize=m_nSize;
    SetSize(m_nSize+1);
    assert(m_nSize==oldSize+1);

    if (m_nSize==oldSize){
        return;
    }
    SetAt(m_nSize-1,dValue);
}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
    assert(nIndex>=0 && nIndex<m_nSize);

    if (nIndex<0 || nIndex>=m_nSize) return;
    if (m_nSize==1){
        Free();
        return;
    }
    for (int i=nIndex+1;i<m_nSize;++i){
        m_pData[i-1]=m_pData[i];
    }
   --m_nSize;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
    assert(nIndex>=0 && nIndex<=m_nSize);
    
    if (nIndex<0 || nIndex>m_nSize){
        return;
    }
    int oldArraySize=m_nSize;
    SetSize(m_nSize+1);
    assert(oldArraySize+1==m_nSize);

    if (oldArraySize==m_nSize){
        return;
    }
    for (int i=m_nSize-1;i>nIndex;--i){
        m_pData[i]=m_pData[i-1];
    }
    m_pData[nIndex]=dValue;
}

// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
    if (this==&arr){
        return *this;
    }
	SetSize(arr.GetSize());
    assert(m_nSize==arr.m_nSize);
    
    if (m_nSize!=arr.m_nSize){
        return *this;
    }
    if (m_nSize==0){
        Free();
    }
    else{
        for (int i=0;i<m_nSize;++i){
            m_pData[i]=arr.m_pData[i];
        }
    }
	return *this;
}
