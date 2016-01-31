//===----------------------------------------------------------------------===//
//
// This file decribes the dynamic mutation analysis IR instrumenter pass
// 
// Add by Wang Bo. OCT 21, 2015
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/AccMut/DFA.h"

#include "llvm/ADT/DepthFirstIterator.h"


#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include<fstream>
#include<sstream>


using namespace llvm;
using namespace std;

map<string, vector<Edge*> *> DFA::TotalCFG;

//map<BasicBlock *, vector<Value*>> DFA::IN;
//map<BasicBlock *, vector<Value*>> DFA::OUT;


DFA::DFA(Module *M) : FunctionPass(ID) {
	this->TheModule = M;
}

bool DFA::runOnFunction(Function & F){
	if(TotalCFG[F.getName()] == NULL || TotalCFG[F.getName()]->size() == 0 ){
		vector<Edge *> *v = new vector<Edge *>();
		TotalCFG[F.getName()] = v;
		drawCFG(F);
	}
	dumpCFG(TotalCFG[F.getName()]);
	dumpRD(F);
	//constantPro(F);
	liveAnalysis(F);
	return false;
}

void DFA::drawCFG(Function& F){
	for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
	  BasicBlock *B = I;
	  TerminatorInst *Last = B->getTerminator();
	  for (unsigned i = 0; i < Last->getNumSuccessors(); ++i) {
      	BasicBlock *Succ = Last->getSuccessor(i);
		Edge *e = new Edge;
		e->from = B;
		e->to = Succ;
		TotalCFG[F.getName()]->push_back(e);
	  }
	}
}

void DFA::constantPro(Function &F){
	for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
		BasicBlock *B = I;
		for(BasicBlock::iterator BI = B->begin(); BI !=  B->end(); ++BI){
			if(!BI->isBinaryOp()){
				continue;
			}
			Value* c = constInstFolding(&(*BI));
			if(!c){
				errs()<<"FOLDING : \n";
				errs()<<"\t"<<*BI<<"\n";
				errs()<<"TO CONST : \n";
				errs()<<"\t"<<*c;
				BI->replaceAllUsesWith(c);
			}
		}
	}
}



Constant* DFA::constInstFolding(Instruction *I){
	if (I->isBinaryOp()) {
		/*
		if((ConstantExpr *ce0 = dyn_cast<ConstantExpr>(I->getOperand(0))) && (ConstantExpr *ce1 = dyn_cast<ConstantExpr>(I->getOperand(1)))){
			return ConstantExpr::get(I->getOpcode(), ce0, ce1);
		}*/
		ConstantExpr *ce0 = dyn_cast<ConstantExpr>(I->getOperand(0));
		ConstantExpr *ce1 = dyn_cast<ConstantExpr>(I->getOperand(1));
		if(ce0 != NULL  && ce1 != NULL){
			return ConstantExpr::get(I->getOpcode(), ce0, ce1);
		}
	}
	return NULL;
}

void DFA::dumpCFG(vector<Edge*>* edges){
	errs()<<"CFG : \n";
	for(vector<Edge*>::iterator i = edges->begin(); i != edges->end(); i++){
		errs()<<"\t"<<(*i)->from->getName()<<" --> "<<(*i)->to->getName()<<"\n";
	}
	errs()<<"\n";
}

void DFA::dumpRD(Function& F){
	BasicBlock *Entry = F.begin();
	SmallPtrSet<BasicBlock*,16> Visited;
	for (BasicBlock *B : depth_first_ext(Entry, Visited)){
		for(BasicBlock::iterator BI = B->begin(); BI !=  B->end(); ++BI){
			if(BI->use_empty()){
				continue;
			}
			errs()<<"ALL USE OF DEF : "<<*BI<<"\n";
			for(auto it = BI->use_begin(); it != BI->use_end(); it++){
				errs()<<"\t"<<*(it->getUser())<<"\n";
			}
		}
	}
}

void DFA::liveAnalysis(Function &F){
	
	SmallPtrSet<BasicBlock*,16> Visited;
	BasicBlock *Entry = F.begin();
	BasicBlock *Exit = &(F.back());
	map<BasicBlock*, SmallPtrSet<Instruction*,16> > ALLUSE;
	map<BasicBlock*, SmallPtrSet<Instruction*,16> > ALLDEF;
	map<BasicBlock*, SmallPtrSet<Instruction*,16> > ALLIN;
	map<BasicBlock*, SmallPtrSet<Instruction*,16> > ALLOUT;
	
	//init use and def
	for (BasicBlock *B : inverse_depth_first_ext(Exit, Visited)){
		errs()<<"GETTING USE & DEF :"<<B->getName()<<"\n";
		SmallPtrSet<Instruction*,16> use;
		SmallPtrSet<Instruction*,16> def;
		for(BasicBlock::iterator BI = B->begin(); BI !=  B->end(); ++BI){
			if( ! BI->isBinaryOp()){//only analysis binop
				continue;
			}
			
			Instruction *op0 = dyn_cast<Instruction>(BI->getOperand(0));
			Instruction *op1 = dyn_cast<Instruction>(BI->getOperand(1));

			if(op0 == NULL || op1 == NULL){
				continue;
			}

			errs()<<*op0<<"        OP0 \n";
			errs()<<*op1<<"        OP1 \n";
			
			use.insert(&(*BI));
			if(op0->getParent() != B){
				def.insert(op0);
				errs()<<*op0<<"HIT OP0 \n";
			}
			
			if(op1->getParent() != B){
				
				errs()<<*op1<<"HIT OP1 \n";
				def.insert(op1);
			}			
		}
		
		ALLUSE[B] = use;
		ALLDEF[B] = def;
		//errs()<<"SIZE : "<<use.size()<<"  "<<def.size()<<"\n";
	}

	bool changed;
	do{
		changed = false;
		for(BasicBlock *BB : inverse_depth_first_ext(Exit, Visited)){
			errs()<<BB->getName()<<" -----------\n";
			if(!ALLIN.count(BB)){
				SmallPtrSet<Instruction*,16> in;
				ALLIN[BB] = in;
			}
			if(!ALLOUT.count(BB)){
				SmallPtrSet<Instruction*,16> out;
				ALLOUT[BB] = out;
			}
			SmallPtrSet<Instruction*,16> out_ori = ALLOUT[BB];
			

			TerminatorInst *Last = BB->getTerminator();
			for (unsigned i = 0; i < Last->getNumSuccessors(); ++i) {
				BasicBlock *Succ = Last->getSuccessor(i);
				for(auto it = ALLOUT[Succ].begin(); it != ALLOUT[Succ].end(); it++){
					if(!ALLOUT[BB].count(*it)){
						changed = true;
					}
					ALLOUT[BB].insert(*it);
				}
			}

			
			for(auto it = ALLUSE[BB].begin(); it != ALLUSE[BB].end(); it++){
				ALLOUT[BB].insert(*it);
			}
			for(auto it = ALLDEF[BB].begin(); it != ALLDEF[BB].end(); it++){
				if(ALLOUT[BB].count(*it)){
					ALLOUT[BB].erase(*it);
				}
			}
			if(ALLOUT[BB].size() != out_ori.size()){
				changed = true;
			}else{
				for(auto it = ALLOUT[BB].begin(); it != ALLOUT[BB].end(); it++){
					if(!out_ori.count(*it)){
						changed = true;
						break;
					}
				}				
			}
		}		
	}while(changed);

	for (BasicBlock *B : depth_first_ext(Entry, Visited)){
		//if(ALLOUT[B].size() != 0){
			errs()<<"LIVENESS OF BB: "<<B->getName()<<"\n";
			for(auto it = ALLOUT[B].begin(); it != ALLOUT[B].end(); it++){
				errs()<<*it<<"\n";
			}
		//}
	}
	
}

void DFA::removeDeadBlocks(Function& F){
	SmallPtrSet<BasicBlock*, 8> Reachable;
	
	for (BasicBlock *BB : depth_first_ext(&F, Reachable)){
	  (void)BB;
	}
	vector<BasicBlock*> DeadBlocks;
	for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
	  if (!Reachable.count(I)) {
		BasicBlock *BB = I;
		DeadBlocks.push_back(BB);
		while (PHINode *PN = dyn_cast<PHINode>(BB->begin())) {
		  PN->replaceAllUsesWith(Constant::getNullValue(PN->getType()));
		  BB->getInstList().pop_front();
		}
		for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI)
		  (*SI)->removePredecessor(BB);
		BB->dropAllReferences();
	  }
	
	for (unsigned i = 0, e = DeadBlocks.size(); i != e; ++i) {
	  DeadBlocks[i]->eraseFromParent();
	}

}
	
/*------------------reserved begin-------------------*/
void DFA::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char DFA::ID = 0;
/*-----------------reserved end --------------------*/
