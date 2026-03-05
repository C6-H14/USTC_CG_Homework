// implementation of class DArray
#ifndef DARRAY_INL
#define DARRAY_INL

#include <assert.h>
#include <iostream>
#include "DArray.h"
using namespace std;
// default constructor
template <typename T>
DArray<T>::DArray() {
	Init();
}

// initilize the array
template <typename T>
void DArray<T>::Init() {
    m_nMax=0;
    m_nSize=0;
    m_pData=nullptr;
}

// free the array
template <typename T>
void DArray<T>::Free() {
    m_nMax=0;
    m_nSize=0;
}

// allocate enough memory
template <typename T>
void DArray<T>::Reserve(int nSize){
    if (m_nMax>=nSize){
        return;
    }
    if (m_nMax<=0) m_nMax=1;
    while (m_nMax<nSize)
    {
        assert(m_nMax>=0);
        m_nMax=(int)(m_nMax*increaseNum);
    }
    unique_ptr<T[]> newPointer=make_unique<T[]>(m_nMax);
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
template <typename T>
DArray<T>::DArray(int nSize, T dValue) {
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
    m_nSize=nSize;
}

template <typename T>
DArray<T>::DArray(const DArray& arr) {
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
    m_nSize=arr.m_nSize;
}

// deconstructor
template <typename T>
DArray<T>::~DArray() {
	Free();
}

// display the elements of the array
template <typename T>
void DArray<T>::Print() const {
    cout<<m_nSize<<endl;
    for (int i=0;i<m_nSize;++i)
        cout<<GetAt(i)<<' ';
    cout<<endl;
}

// get the size of the array
template <typename T>
int DArray<T>::GetSize() const {
	if (m_nSize<=0) return 0;
    return m_nSize;
}

// set the size of the array
template <typename T>
void DArray<T>::SetSize(int nSize) {
    assert(nSize>=0);

    if (nSize<=0){
        Free();
    }
    else{
        Reserve(nSize);
        for (int i=m_nSize;i<nSize;++i)
            m_pData[i]=T();
        m_nSize=nSize;
    }
}

// get an element at an index
template <typename T>
const T& DArray<T>::GetAt(int nIndex) const {
    assert(nIndex>=0 && nIndex<=m_nSize);

    static T errorValue{};
    if (nIndex>=m_nSize || nIndex<0){
        return errorValue;
    }
	return m_pData[nIndex];
}

// set the value of an element 
template <typename T>
void DArray<T>::SetAt(int nIndex, T dValue) {
    assert(nIndex>=0 && nIndex<=m_nSize);

    if (nIndex>=m_nSize || nIndex<0){
        return;
    }
    m_pData[nIndex]=dValue;
}

// overload operator '[]'
template <typename T>
T& DArray<T>::operator[](int nIndex) {
	assert(nIndex >= 0 && nIndex < m_nSize);

    static T errorValue{};
    if (nIndex>=m_nSize || nIndex<0){
        return errorValue;
    }
	return m_pData[nIndex];
}

// overload operator '[]'
template <typename T>
const T& DArray<T>::operator[](int nIndex) const {
	assert(nIndex >= 0 && nIndex < m_nSize);

    static T errorValue{};
    if (nIndex>=m_nSize || nIndex<0){
        return errorValue;
    }
	return m_pData[nIndex];
}

// add a new element at the end of the array
template <typename T>
void DArray<T>::PushBack(T dValue) {
    int oldSize=m_nSize;
    SetSize(m_nSize+1);
    assert(m_nSize==oldSize+1);

    if (m_nSize==oldSize){
        return;
    }
    SetAt(m_nSize-1,dValue);
}

// delete an element at some index
template <typename T>
void DArray<T>::DeleteAt(int nIndex) {
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
template <typename T>
void DArray<T>::InsertAt(int nIndex, T dValue) {
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
template <typename T>
DArray<T>& DArray<T>::operator = (const DArray<T>& arr) {
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

#endif