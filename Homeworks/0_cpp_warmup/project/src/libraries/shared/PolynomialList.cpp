#include "PolynomialList.h"
#include<fstream>
#include<iostream>
#include <assert.h>

using namespace std;
const double eps=1e-10;

PolynomialList::PolynomialList(const PolynomialList& other) {
    m_Polynomial=other.m_Polynomial;
}

PolynomialList::PolynomialList(const string& file) {
    ReadFromFile(file);
}

PolynomialList::PolynomialList(const double* cof, const int* deg, int n) {
    assert(n>=0);

    for (int i=0;i<n;++i){
        AddOneTerm(Term(deg[i],cof[i]));
    }
}

PolynomialList::PolynomialList(const vector<int>& deg, const vector<double>& cof) {
    assert(deg.size()==cof.size());

    for (int i=min(deg.size(),cof.size())-1;i>=0;--i){
        AddOneTerm(Term(deg[i],cof[i]));
    }
}

double PolynomialList::coff(int i) const {
    for (const Term& term:m_Polynomial){
        if (term.deg==i) return term.cof;
        if (term.deg<i) break;
    }
    return 0.0;
}

double& PolynomialList::coff(int i) {
    return AddOneTerm(Term(i,0.0)).cof;
}

void PolynomialList::compress() {
    auto it=m_Polynomial.begin();
    while (it!=m_Polynomial.end()){
        if (fabs(it->cof)<eps){
            it=m_Polynomial.erase(it);
        }
        else{
            ++it;
        }
    }
}

PolynomialList PolynomialList::operator+(const PolynomialList& right) const {
    PolynomialList poly(*this);
    for (const auto& term:right.m_Polynomial){
        poly.AddOneTerm(term);
    }
    poly.compress();
    return poly;
}

PolynomialList PolynomialList::operator-(const PolynomialList& right) const {
    PolynomialList poly(*this);
    for (const auto& term:right.m_Polynomial){
        poly.AddOneTerm(Term(term.deg,-term.cof));
    }
    poly.compress();
    return poly;
}

PolynomialList PolynomialList::operator*(const PolynomialList& right) const {
    PolynomialList poly;
    for (const auto& term1:this->m_Polynomial){
        for (const auto& term2:right.m_Polynomial){
            poly.AddOneTerm(Term(term1.deg+term2.deg,term1.cof*term2.cof));
        }
    }
    poly.compress();
    return poly;
}

PolynomialList& PolynomialList::operator=(const PolynomialList& right) {
    m_Polynomial=right.m_Polynomial;
    return *this;
}

void PolynomialList::Print() const {
    if (m_Polynomial.empty()){
        cout<<0<<endl;
        return;
    }
    for (auto it=m_Polynomial.begin();it!=m_Polynomial.end();++it){
        if (it!=m_Polynomial.begin()){
            cout<<" ";
            if (it->cof>0){
                cout<<"+";
            }
        }
        cout<<it->cof;
        if (it->deg>0){
            cout<<"x^"<<it->deg;
        }
    }
    cout<<endl;
}

bool PolynomialList::ReadFromFile(const string& file) {
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
    m_Polynomial.clear();
    for (int i=0;i<nTerm;++i){
        int deg;
        double cof;
        fin>>deg>>cof;
        m_Polynomial.push_back(Term(deg,cof));
    }
    fin.close();
    compress();
    return true;
}

PolynomialList::Term& PolynomialList::AddOneTerm(const Term& term) {
    auto it=m_Polynomial.begin();
    for (;it!=m_Polynomial.end();++it){
        if (it->deg==term.deg){
            it->cof+=term.cof;
            return *it;
        }
        if (it->deg>term.deg) break;
    }
    return *(m_Polynomial.insert(it,term));
}