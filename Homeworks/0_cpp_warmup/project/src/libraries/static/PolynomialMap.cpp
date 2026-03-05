#include "PolynomialMap.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

using namespace std;
const double eps=1e-10;

PolynomialMap::PolynomialMap(const PolynomialMap& other) {
    m_Polynomial=other.m_Polynomial;
}

PolynomialMap::PolynomialMap(const string& file) {
    ReadFromFile(file);
}

PolynomialMap::PolynomialMap(const double* cof, const int* deg, int n) {
    assert(n>=0);

    for (int i=0;i<n;++i){
        coff(deg[i])=cof[i];
    }
}

PolynomialMap::PolynomialMap(const vector<int>& deg, const vector<double>& cof) {
	assert(deg.size() == cof.size());

    for (int i=min(deg.size(),cof.size())-1;i>=0;--i){
        coff(deg[i])=cof[i];
    }
}

double PolynomialMap::coff(int i) const {
	auto target = m_Polynomial.find(i);
    if (target == m_Polynomial.end())
        return 0.;

    return target->second;
}

double& PolynomialMap::coff(int i) {
	return m_Polynomial[i];
}

void PolynomialMap::compress() {
    for (auto it = m_Polynomial.begin(); it != m_Polynomial.end(); ) {
        if (fabs(it->second) < eps) {
            it = m_Polynomial.erase(it); 
        } else {
            ++it;
        }
    }
}

PolynomialMap PolynomialMap::operator+(const PolynomialMap& right) const {
    auto poly(*this);
    for (const auto& term:right.m_Polynomial){
        poly.coff(term.first)+=term.second;
    }
    poly.compress();
    return poly;
}

PolynomialMap PolynomialMap::operator-(const PolynomialMap& right) const {
    auto poly(*this);
    for (const auto& term:right.m_Polynomial){
        poly.coff(term.first)-=term.second;
    }
    poly.compress();
    return poly;
}

PolynomialMap PolynomialMap::operator*(const PolynomialMap& right) const {
    PolynomialMap poly;
    for (const auto& term1:this->m_Polynomial){
        for (const auto& term2:right.m_Polynomial){
            poly.coff(term1.first+term2.first)+=term1.second*term2.second;
        }
    }
    poly.compress();
    return poly;
}

PolynomialMap& PolynomialMap::operator=(const PolynomialMap& right) {
    m_Polynomial=right.m_Polynomial;
    return *this;
}

void PolynomialMap::Print() const {
    if (m_Polynomial.empty()){
        cout<<0<<endl;
        return;
    }
    for (auto it=m_Polynomial.begin();it!=m_Polynomial.end();++it){
        if (it!=m_Polynomial.begin()){
            cout<<" ";
            if (it->second>0){
                cout<<"+";
            }
        }
        cout<<it->second;
        if (it->first>0){
            cout<<"x^"<<it->first;
        }
    }
    cout<<endl;
}

bool PolynomialMap::ReadFromFile(const string& file) {
    m_Polynomial.clear();
    ifstream fin(file);
    if (!fin.is_open()){
        cout<<"Error:Can't open file"<<endl;
        return false;
    }
    char type;
    int nTerm;
    fin>>type>>nTerm;
    assert(type=='P');

    if (type!='P'){
        cout<<"Error: Invalid input format. Missing symbol \"P\"."<<endl;
        return false;
    }

    for (int i=0;i<nTerm;++i){
        int first;
        double second;
        fin>>first>>second;
        coff(first)=second;
    }
    fin.close();
    compress();
    return true;
}
