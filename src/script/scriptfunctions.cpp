#include "script.h"

// circular dependency with the Ctrasaction. it is a problem tho that the globals is reuiritg the script file. 
//bool CheckSig(std::vector<unsigned char> vchSig, std::vector<unsigned char> vchPubKey, CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);


typedef vector<unsigned char> valtype;
static const valtype vchFalse(0);
static const valtype vchZero(0);
static const valtype vchTrue(1, 1);
static const CBigNum bnZero(0);
static const CBigNum bnOne(1);
static const CBigNum bnFalse(0);
static const CBigNum bnTrue(1);

bool CastToBool(const valtype& vch)
{
    return (CBigNum(vch) != bnZero);
}

void MakeSameSize(valtype& vch1, valtype& vch2)
{
    // Lengthen the shorter one
    if (vch1.size() < vch2.size())
        vch1.resize(vch2.size(), 0);
    if (vch2.size() < vch1.size())
        vch2.resize(vch1.size(), 0);
}

//
// Script is a Stack Machine that evaluates a predicate returning a bool indicating valid or not. this is similar to Forth and it is very similar to the original bitcoin stack machine
// There are no loops.
//


#define stacktop(i) (stack.at(stack.size()+(i)))
#define altstacktop(i) (altstack.at(altstack.size()+(i)))

//bool EvalScript(const CScript& script, const C) // TODO: wait let's proceed with the construction of the globlas. get got what we need.