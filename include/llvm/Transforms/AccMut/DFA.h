//===----------------------------------------------------------------------===//
//
 // This file implements the mutation generator pass
// 
 // Add by Wang Bo. OCT 19, 2015
//
//===----------------------------------------------------------------------===//

#ifndef ACCMUT_DFA_H
#define ACCMUT_DFA_H

#include "llvm/Transforms/AccMut/Config.h"

#include "llvm/IR/Module.h"

#include <map>
#include <string>
#include <vector>

using namespace llvm;
using namespace std;

class Edge;

class DFA: public FunctionPass{
public:
	static char ID;// Pass identification, replacement for typeid
	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	static map<string, vector<Edge*> *> TotalCFG;
	Module *TheModule;
	DFA(Module *M);
	virtual bool runOnFunction(Function &F);
	static void drawCFG(Function& F);
	static void dumpCFG(vector<Edge*>* edges);
	static void constantPro(Function &F);	
	static void dumpRD(Function &F);
	static void liveAnalysis(Function &F);
	static Constant* constInstFolding(Instruction *I);
	static void removeDeadBlocks(Function& F);
	static void scr(Function &F);
private:

};


class Edge{
public:
	BasicBlock* from;
	BasicBlock* to;
};

#endif

