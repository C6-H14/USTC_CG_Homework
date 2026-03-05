#include <memory>
#ifndef DARRAYH
#define DARRAYH

template<typename T>
// interfaces of Dynamic Array class DArray
class DArray {
public:
	DArray(); // default constructor
	DArray(int nSize, T dValue); // set an array with default values
	DArray(const DArray& arr); // copy constructor
	~DArray(); // deconstructor

	void Print() const; // print the elements of the array

	int GetSize() const; // get the size of the array
	void SetSize(int nSize); // set the size of the array

	const T& GetAt(int nIndex) const; // get an element at an index
	void SetAt(int nIndex, T dValue); // set the value of an element

	T& operator[](int nIndex); // overload operator '[]'
	const T& operator[](int nIndex) const; // overload operator '[]'

	void PushBack(T dValue); // add a new element at the end of the array
	void DeleteAt(int nIndex); // delete an element at some index
	void InsertAt(int nIndex, T dValue); // insert a new element at some index

	DArray& operator = (const DArray& arr); //overload operator '='

private:
	std::unique_ptr<T[]> m_pData; // the pointer to the array memory
	int m_nSize; // the size of the array
	int m_nMax;
    static constexpr double increaseNum=2;

private:
	void Init(); // initilize the array
	void Free(); // free the array
	void Reserve(int nSize); // allocate enough memory
};
#include "DArray.inl"
#endif