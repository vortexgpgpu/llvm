#include "VortexBranchDivergence.h"

#include "llvm/Support/Debug.h"
#include "RISCV.h"
#include "RISCVSubtarget.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/TargetPassConfig.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicsRISCV.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/ModuleSlotTracker.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

#include "llvm/Analysis/LegacyDivergenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/TargetTransformInfo.h"

using namespace vortex;
using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "vortex-branch-divergence"

//#define LLVM_DEBUG(x) do {x;} while (false)

static cl::opt<bool> RelaxedUniformRegions(
  "vortex-branch-divergence-relaxed-uniform-regions", cl::Hidden,
  cl::desc("Allow relaxed uniform region checks"), cl::init(true)
);

namespace vortex {

using BBValuePair = std::pair<BasicBlock *, Value *>;

using RNVector = SmallVector<RegionNode *, 8>;
using BBVector = SmallVector<BasicBlock *, 8>;
using BranchVector = SmallVector<BranchInst *, 8>;
using BBValueVector = SmallVector<BBValuePair, 2>;

using BBSet = SmallPtrSet<BasicBlock *, 8>;

using PhiMap = MapVector<PHINode *, BBValueVector>;
using BB2BBVecMap = MapVector<BasicBlock *, BBVector>;

using BBPhiMap = DenseMap<BasicBlock *, PhiMap>;
using BBPredicates = DenseMap<BasicBlock *, Value *>;
using PredMap = DenseMap<BasicBlock *, BBPredicates>;
using BB2BBMap = DenseMap<BasicBlock *, BasicBlock *>;

/// Finds the nearest common dominator of a set of BasicBlocks.
///
/// For every BB you add to the set, you can specify whether we "remember" the
/// block.  When you get the common dominator, you can also ask whether it's one
/// of the blocks we remembered.
class NearestCommonDominator {
  DominatorTree *DT;
  BasicBlock *Result = nullptr;
  bool ResultIsRemembered = false;

  /// Add BB to the resulting dominator.
  void addBlock(BasicBlock *BB, bool Remember) {
    if (!Result) {
      Result = BB;
      ResultIsRemembered = Remember;
      return;
    }

    BasicBlock *NewResult = DT->findNearestCommonDominator(Result, BB);
    if (NewResult != Result)
      ResultIsRemembered = false;
    if (NewResult == BB)
      ResultIsRemembered |= Remember;
    Result = NewResult;
  }

public:
  explicit NearestCommonDominator(DominatorTree *DomTree) : DT(DomTree) {}

  void addBlock(BasicBlock *BB) {
    addBlock(BB, /* Remember = */ false);
  }

  void addAndRememberBlock(BasicBlock *BB) {
    addBlock(BB, /* Remember = */ true);
  }

  /// Get the nearest common dominator of all the BBs added via addBlock() and
  /// addAndRememberBlock().
  BasicBlock *result() { return Result; }

  /// Is the BB returned by getResult() one of the blocks we added to the set
  /// with addAndRememberBlock()?
  bool resultIsRememberedBlock() { return ResultIsRemembered; }
};

class StructurizeCFG {

  Type *Boolean;
  ConstantInt *BoolTrue;
  ConstantInt *BoolFalse;
  UndefValue *BoolUndef;

  Function *Func;
  Region *ParentRegion;

  DominatorTree *DT;
  LoopInfo *LI;

  SmallVector<RegionNode *, 8> Order;
  BBSet Visited;

  BBPhiMap DeletedPhis;
  BB2BBVecMap AddedPhis;

  PredMap Predicates;
  BranchVector Conditions;

  BB2BBMap Loops;
  PredMap LoopPreds;
  BranchVector LoopConds;

  RegionNode *PrevNode;

  void orderNodes();

  Loop *getAdjustedLoop(RegionNode *RN);

  unsigned getAdjustedLoopDepth(RegionNode *RN);

  void analyzeLoops(RegionNode *N);

  Value *invert(Value *Condition);

  Value *buildCondition(BranchInst *Term, unsigned Idx, bool Invert);

  void gatherPredicates(RegionNode *N);

  void collectInfos();

  void insertConditions(bool Loops);

  void delPhiValues(BasicBlock *From, BasicBlock *To);

  void addPhiValues(BasicBlock *From, BasicBlock *To);

  void setPhiValues();

  void killTerminator(BasicBlock *BB);

  void changeExit(RegionNode *Node, BasicBlock *NewExit,
                  bool IncludeDominator);

  BasicBlock *getNextFlow(BasicBlock *Dominator);

  BasicBlock *needPrefix(bool NeedEmpty);

  BasicBlock *needPostfix(BasicBlock *Flow, bool ExitUseAllowed);

  void setPrevNode(BasicBlock *BB);

  bool dominatesPredicates(BasicBlock *BB, RegionNode *Node);

  bool isPredictableTrue(RegionNode *Node);

  void wireFlow(bool ExitUseAllowed, BasicBlock *LoopEnd);

  void handleLoops(bool ExitUseAllowed, BasicBlock *LoopEnd);

  void createFlow();

  void rebuildSSA();

public:

  StructurizeCFG(LLVMContext &Context,
                 DominatorTree* DT,
                 LoopInfo* LI);

  bool runOnRegion(Region *R);
};

///////////////////////////////////////////////////////////////////////////////

/// Use the exit block to determine the loop if RN is a SubRegion.
Loop *StructurizeCFG::getAdjustedLoop(RegionNode *RN) {
  if (RN->isSubRegion()) {
    Region *SubRegion = RN->getNodeAs<Region>();
    return LI->getLoopFor(SubRegion->getExit());
  }

  return LI->getLoopFor(RN->getEntry());
}

/// Use the exit block to determine the loop depth if RN is a SubRegion.
unsigned StructurizeCFG::getAdjustedLoopDepth(RegionNode *RN) {
  if (RN->isSubRegion()) {
    Region *SubR = RN->getNodeAs<Region>();
    return LI->getLoopDepth(SubR->getExit());
  }

  return LI->getLoopDepth(RN->getEntry());
}

/// Build up the general order of nodes
void StructurizeCFG::orderNodes() {
  ReversePostOrderTraversal<Region*> RPOT(ParentRegion);
  SmallDenseMap<Loop*, unsigned, 8> LoopBlocks;

  // The reverse post-order traversal of the list gives us an ordering close
  // to what we want.  The only problem with it is that sometimes backedges
  // for outer loops will be visited before backedges for inner loops.
  for (RegionNode *RN : RPOT) {
    Loop *Loop = getAdjustedLoop(RN);
    ++LoopBlocks[Loop];
  }

  unsigned CurrentLoopDepth = 0;
  Loop *CurrentLoop = nullptr;
  for (auto I = RPOT.begin(), E = RPOT.end(); I != E; ++I) {
    RegionNode *RN = cast<RegionNode>(*I);
    unsigned LoopDepth = getAdjustedLoopDepth(RN);

    if (is_contained(Order, *I))
      continue;

    if (LoopDepth < CurrentLoopDepth) {
      // Make sure we have visited all blocks in this loop before moving back to
      // the outer loop.

      auto LoopI = I;
      while (unsigned &BlockCount = LoopBlocks[CurrentLoop]) {
        LoopI++;
        if (getAdjustedLoop(cast<RegionNode>(*LoopI)) == CurrentLoop) {
          --BlockCount;
          Order.push_back(*LoopI);
        }
      }
    }

    CurrentLoop = getAdjustedLoop(RN);
    if (CurrentLoop)
      LoopBlocks[CurrentLoop]--;

    CurrentLoopDepth = LoopDepth;
    Order.push_back(*I);
  }

  // This pass originally used a post-order traversal and then operated on
  // the list in reverse. Now that we are using a reverse post-order traversal
  // rather than re-working the whole pass to operate on the list in order,
  // we just reverse the list and continue to operate on it in reverse.
  std::reverse(Order.begin(), Order.end());
}

/// Determine the end of the loops
void StructurizeCFG::analyzeLoops(RegionNode *N) {
  if (N->isSubRegion()) {
    // Test for exit as back edge
    BasicBlock *Exit = N->getNodeAs<Region>()->getExit();
    if (Visited.count(Exit))
      Loops[Exit] = N->getEntry();

  } else {
    // Test for successors as back edge
    BasicBlock *BB = N->getNodeAs<BasicBlock>();
    BranchInst *Term = cast<BranchInst>(BB->getTerminator());

    for (BasicBlock *Succ : Term->successors())
      if (Visited.count(Succ))
        Loops[Succ] = BB;
  }
}

/// Invert the given condition
Value *StructurizeCFG::invert(Value *Condition) {
  // First: Check if it's a constant
  if (Constant *C = dyn_cast<Constant>(Condition))
    return ConstantExpr::getNot(C);

  // Second: If the condition is already inverted, return the original value
  Value *NotCondition;
  if (match(Condition, m_Not(m_Value(NotCondition))))
    return NotCondition;

  if (Instruction *Inst = dyn_cast<Instruction>(Condition)) {
    // Third: Check all the users for an invert
    BasicBlock *Parent = Inst->getParent();
    for (User *U : Condition->users())
      if (Instruction *I = dyn_cast<Instruction>(U))
        if (I->getParent() == Parent && match(I, m_Not(m_Specific(Condition))))
          return I;

    // Last option: Create a new instruction
    return BinaryOperator::CreateNot(Condition, "", Parent->getTerminator());
  }

  if (Argument *Arg = dyn_cast<Argument>(Condition)) {
    BasicBlock &EntryBlock = Arg->getParent()->getEntryBlock();
    return BinaryOperator::CreateNot(Condition,
                                     Arg->getName() + ".inv",
                                     EntryBlock.getTerminator());
  }

  llvm_unreachable("Unhandled condition to invert");
}

/// Build the condition for one edge
Value *StructurizeCFG::buildCondition(BranchInst *Term, unsigned Idx,
                                      bool Invert) {
  Value *Cond = Invert ? BoolFalse : BoolTrue;
  if (Term->isConditional()) {
    Cond = Term->getCondition();

    if (Idx != (unsigned)Invert)
      Cond = invert(Cond);
  }
  return Cond;
}

/// Analyze the predecessors of each block and build up predicates
void StructurizeCFG::gatherPredicates(RegionNode *N) {
  RegionInfo *RI = ParentRegion->getRegionInfo();
  BasicBlock *BB = N->getEntry();
  BBPredicates &Pred = Predicates[BB];
  BBPredicates &LPred = LoopPreds[BB];

  for (BasicBlock *P : predecessors(BB)) {
    // Ignore it if it's a branch from outside into our region entry
    if (!ParentRegion->contains(P))
      continue;

    Region *R = RI->getRegionFor(P);
    if (R == ParentRegion) {
      // It's a top level block in our region
      BranchInst *Term = cast<BranchInst>(P->getTerminator());
      for (unsigned i = 0, e = Term->getNumSuccessors(); i != e; ++i) {
        BasicBlock *Succ = Term->getSuccessor(i);
        if (Succ != BB)
          continue;

        if (Visited.count(P)) {
          // Normal forward edge
          if (Term->isConditional()) {
            // Try to treat it like an ELSE block
            BasicBlock *Other = Term->getSuccessor(!i);
            if (Visited.count(Other) && !Loops.count(Other) &&
                !Pred.count(Other) && !Pred.count(P)) {

              Pred[Other] = BoolFalse;
              Pred[P] = BoolTrue;
              continue;
            }
          }
          Pred[P] = buildCondition(Term, i, false);
        } else {
          // Back edge
          LPred[P] = buildCondition(Term, i, true);
        }
      }
    } else {
      // It's an exit from a sub region
      while (R->getParent() != ParentRegion)
        R = R->getParent();

      // Edge from inside a subregion to its entry, ignore it
      if (*R == *N)
        continue;

      BasicBlock *Entry = R->getEntry();
      if (Visited.count(Entry))
        Pred[Entry] = BoolTrue;
      else
        LPred[Entry] = BoolFalse;
    }
  }
}

/// Collect various loop and predicate infos
void StructurizeCFG::collectInfos() {
  // Reset predicate
  Predicates.clear();

  // and loop infos
  Loops.clear();
  LoopPreds.clear();

  // Reset the visited nodes
  Visited.clear();

  for (RegionNode *RN : reverse(Order)) {
    LLVM_DEBUG(dbgs() << "Visiting: "
                      << (RN->isSubRegion() ? "SubRegion with entry: " : "")
                      << RN->getEntry()->getName() << " Loop Depth: "
                      << LI->getLoopDepth(RN->getEntry()) << "\n");

    // Analyze all the conditions leading to a node
    gatherPredicates(RN);

    // Remember that we've seen this node
    Visited.insert(RN->getEntry());

    // Find the last back edges
    analyzeLoops(RN);
  }
}

/// Insert the missing branch conditions
void StructurizeCFG::insertConditions(bool Loops) {
  BranchVector &Conds = Loops ? LoopConds : Conditions;
  Value *Default = Loops ? BoolTrue : BoolFalse;
  SSAUpdater PhiInserter;

  for (BranchInst *Term : Conds) {
    assert(Term->isConditional());

    BasicBlock *Parent = Term->getParent();
    BasicBlock *SuccTrue = Term->getSuccessor(0);
    BasicBlock *SuccFalse = Term->getSuccessor(1);

    PhiInserter.Initialize(Boolean, "");
    PhiInserter.AddAvailableValue(&Func->getEntryBlock(), Default);
    PhiInserter.AddAvailableValue(Loops ? SuccFalse : Parent, Default);

    BBPredicates &Preds = Loops ? LoopPreds[SuccFalse] : Predicates[SuccTrue];

    NearestCommonDominator Dominator(DT);
    Dominator.addBlock(Parent);

    Value *ParentValue = nullptr;
    for (std::pair<BasicBlock *, Value *> BBAndPred : Preds) {
      BasicBlock *BB = BBAndPred.first;
      Value *Pred = BBAndPred.second;

      if (BB == Parent) {
        ParentValue = Pred;
        break;
      }
      PhiInserter.AddAvailableValue(BB, Pred);
      Dominator.addAndRememberBlock(BB);
    }

    if (ParentValue) {
      Term->setCondition(ParentValue);
    } else {
      if (!Dominator.resultIsRememberedBlock())
        PhiInserter.AddAvailableValue(Dominator.result(), Default);

      Term->setCondition(PhiInserter.GetValueInMiddleOfBlock(Parent));
    }
  }
}

/// Remove all PHI values coming from "From" into "To" and remember
/// them in DeletedPhis
void StructurizeCFG::delPhiValues(BasicBlock *From, BasicBlock *To) {
  PhiMap &Map = DeletedPhis[To];
  for (PHINode &Phi : To->phis()) {
    while (Phi.getBasicBlockIndex(From) != -1) {
      Value *Deleted = Phi.removeIncomingValue(From, false);
      Map[&Phi].push_back(std::make_pair(From, Deleted));
    }
  }
}

/// Add a dummy PHI value as soon as we knew the new predecessor
void StructurizeCFG::addPhiValues(BasicBlock *From, BasicBlock *To) {
  for (PHINode &Phi : To->phis()) {
    Value *Undef = UndefValue::get(Phi.getType());
    Phi.addIncoming(Undef, From);
  }
  AddedPhis[To].push_back(From);
}

/// Add the real PHI value as soon as everything is set up
void StructurizeCFG::setPhiValues() {
  SmallVector<PHINode *, 8> InsertedPhis;
  SSAUpdater Updater(&InsertedPhis);
  for (const auto &AddedPhi : AddedPhis) {
    BasicBlock *To = AddedPhi.first;
    const BBVector &From = AddedPhi.second;

    if (!DeletedPhis.count(To))
      continue;

    PhiMap &Map = DeletedPhis[To];
    for (const auto &PI : Map) {
      PHINode *Phi = PI.first;
      Value *Undef = UndefValue::get(Phi->getType());
      Updater.Initialize(Phi->getType(), "");
      Updater.AddAvailableValue(&Func->getEntryBlock(), Undef);
      Updater.AddAvailableValue(To, Undef);

      NearestCommonDominator Dominator(DT);
      Dominator.addBlock(To);
      for (const auto &VI : PI.second) {
        Updater.AddAvailableValue(VI.first, VI.second);
        Dominator.addAndRememberBlock(VI.first);
      }

      if (!Dominator.resultIsRememberedBlock())
        Updater.AddAvailableValue(Dominator.result(), Undef);

      for (BasicBlock *FI : From)
        Phi->setIncomingValueForBlock(FI, Updater.GetValueAtEndOfBlock(FI));
    }

    DeletedPhis.erase(To);
  }
  assert(DeletedPhis.empty());

  // Simplify any phis inserted by the SSAUpdater if possible
  bool Changed;
  do {
    Changed = false;

    SimplifyQuery Q(Func->getParent()->getDataLayout());
    Q.DT = DT;
    for (size_t i = 0; i < InsertedPhis.size(); ++i) {
      PHINode *Phi = InsertedPhis[i];
      if (Value *V = SimplifyInstruction(Phi, Q)) {
        Phi->replaceAllUsesWith(V);
        Phi->eraseFromParent();
        InsertedPhis[i] = InsertedPhis.back();
        InsertedPhis.pop_back();
        i--;
        Changed = true;
      }
    }
  } while (Changed);
}

/// Remove phi values from all successors and then remove the terminator.
void StructurizeCFG::killTerminator(BasicBlock *BB) {
  Instruction *Term = BB->getTerminator();
  if (!Term)
    return;

  for (succ_iterator SI = succ_begin(BB), SE = succ_end(BB);
       SI != SE; ++SI)
    delPhiValues(BB, *SI);

  Term->eraseFromParent();
}

/// Let node exit(s) point to NewExit
void StructurizeCFG::changeExit(RegionNode *Node, BasicBlock *NewExit,
                                bool IncludeDominator) {
  if (Node->isSubRegion()) {
    Region *SubRegion = Node->getNodeAs<Region>();
    BasicBlock *OldExit = SubRegion->getExit();
    BasicBlock *Dominator = nullptr;

    // Find all the edges from the sub region to the exit
    for (auto BBI = pred_begin(OldExit), E = pred_end(OldExit); BBI != E;) {
      // Incrememt BBI before mucking with BB's terminator.
      BasicBlock *BB = *BBI++;

      if (!SubRegion->contains(BB))
        continue;

      // Modify the edges to point to the new exit
      delPhiValues(BB, OldExit);
      BB->getTerminator()->replaceUsesOfWith(OldExit, NewExit);
      addPhiValues(BB, NewExit);

      // Find the new dominator (if requested)
      if (IncludeDominator) {
        if (!Dominator)
          Dominator = BB;
        else
          Dominator = DT->findNearestCommonDominator(Dominator, BB);
      }
    }

    // Change the dominator (if requested)
    if (Dominator)
      DT->changeImmediateDominator(NewExit, Dominator);

    // Update the region info
    SubRegion->replaceExit(NewExit);
  } else {
    BasicBlock *BB = Node->getNodeAs<BasicBlock>();
    killTerminator(BB);
    BranchInst::Create(NewExit, BB);
    addPhiValues(BB, NewExit);
    if (IncludeDominator)
      DT->changeImmediateDominator(NewExit, BB);
  }
}

/// Create a new flow node and update dominator tree and region info
BasicBlock *StructurizeCFG::getNextFlow(BasicBlock *Dominator) {
  LLVMContext &Context = Func->getContext();
  BasicBlock *Insert = Order.empty() ? ParentRegion->getExit() :
                       Order.back()->getEntry();
  BasicBlock *Flow = BasicBlock::Create(Context, "Flow", Func, Insert);
  DT->addNewBlock(Flow, Dominator);
  ParentRegion->getRegionInfo()->setRegionFor(Flow, ParentRegion);
  return Flow;
}

/// Create a new or reuse the previous node as flow node
BasicBlock *StructurizeCFG::needPrefix(bool NeedEmpty) {
  BasicBlock *Entry = PrevNode->getEntry();

  if (!PrevNode->isSubRegion()) {
    killTerminator(Entry);
    if (!NeedEmpty || Entry->getFirstInsertionPt() == Entry->end())
      return Entry;
  }

  // create a new flow node
  BasicBlock *Flow = getNextFlow(Entry);

  // and wire it up
  changeExit(PrevNode, Flow, true);
  PrevNode = ParentRegion->getBBNode(Flow);
  return Flow;
}

/// Returns the region exit if possible, otherwise just a new flow node
BasicBlock *StructurizeCFG::needPostfix(BasicBlock *Flow,
                                        bool ExitUseAllowed) {
  if (!Order.empty() || !ExitUseAllowed)
    return getNextFlow(Flow);

  BasicBlock *Exit = ParentRegion->getExit();
  DT->changeImmediateDominator(Exit, Flow);
  addPhiValues(Flow, Exit);
  return Exit;
}

/// Set the previous node
void StructurizeCFG::setPrevNode(BasicBlock *BB) {
  PrevNode = ParentRegion->contains(BB) ? ParentRegion->getBBNode(BB)
                                        : nullptr;
}

/// Does BB dominate all the predicates of Node?
bool StructurizeCFG::dominatesPredicates(BasicBlock *BB, RegionNode *Node) {
  BBPredicates &Preds = Predicates[Node->getEntry()];
  return llvm::all_of(Preds, [&](std::pair<BasicBlock *, Value *> Pred) {
    return DT->dominates(BB, Pred.first);
  });
}

/// Can we predict that this node will always be called?
bool StructurizeCFG::isPredictableTrue(RegionNode *Node) {
  BBPredicates &Preds = Predicates[Node->getEntry()];
  bool Dominated = false;

  // Regionentry is always true
  if (!PrevNode)
    return true;

  for (std::pair<BasicBlock*, Value*> Pred : Preds) {
    BasicBlock *BB = Pred.first;
    Value *V = Pred.second;

    if (V != BoolTrue)
      return false;

    if (!Dominated && DT->dominates(BB, PrevNode->getEntry()))
      Dominated = true;
  }

  // TODO: The dominator check is too strict
  return Dominated;
}

/// Take one node from the order vector and wire it up
void StructurizeCFG::wireFlow(bool ExitUseAllowed,
                              BasicBlock *LoopEnd) {
  RegionNode *Node = Order.pop_back_val();
  Visited.insert(Node->getEntry());

  if (isPredictableTrue(Node)) {
    // Just a linear flow
    if (PrevNode) {
      changeExit(PrevNode, Node->getEntry(), true);
    }
    PrevNode = Node;
  } else {
    // Insert extra prefix node (or reuse last one)
    BasicBlock *Flow = needPrefix(false);

    // Insert extra postfix node (or use exit instead)
    BasicBlock *Entry = Node->getEntry();
    BasicBlock *Next = needPostfix(Flow, ExitUseAllowed);

    // let it point to entry and next block
    Conditions.push_back(BranchInst::Create(Entry, Next, BoolUndef, Flow));
    addPhiValues(Flow, Entry);
    DT->changeImmediateDominator(Entry, Flow);

    PrevNode = Node;
    while (!Order.empty() && !Visited.count(LoopEnd) &&
           dominatesPredicates(Entry, Order.back())) {
      handleLoops(false, LoopEnd);
    }

    changeExit(PrevNode, Next, false);
    setPrevNode(Next);
  }
}

void StructurizeCFG::handleLoops(bool ExitUseAllowed,
                                 BasicBlock *LoopEnd) {
  RegionNode *Node = Order.back();
  BasicBlock *LoopStart = Node->getEntry();

  if (!Loops.count(LoopStart)) {
    wireFlow(ExitUseAllowed, LoopEnd);
    return;
  }

  if (!isPredictableTrue(Node))
    LoopStart = needPrefix(true);

  LoopEnd = Loops[Node->getEntry()];
  wireFlow(false, LoopEnd);
  while (!Visited.count(LoopEnd)) {
    handleLoops(false, LoopEnd);
  }

  // If the start of the loop is the entry block, we can't branch to it so
  // insert a new dummy entry block.
  Function *LoopFunc = LoopStart->getParent();
  if (LoopStart == &LoopFunc->getEntryBlock()) {
    LoopStart->setName("entry.orig");

    BasicBlock *NewEntry =
      BasicBlock::Create(LoopStart->getContext(),
                         "entry",
                         LoopFunc,
                         LoopStart);
    BranchInst::Create(LoopStart, NewEntry);
    DT->setNewRoot(NewEntry);
  }

  // Create an extra loop end node
  LoopEnd = needPrefix(false);
  BasicBlock *Next = needPostfix(LoopEnd, ExitUseAllowed);
  LoopConds.push_back(BranchInst::Create(Next, LoopStart,
                                         BoolUndef, LoopEnd));
  addPhiValues(LoopEnd, LoopStart);
  setPrevNode(Next);
}

/// After this function control flow looks like it should be, but
/// branches and PHI nodes only have undefined conditions.
void StructurizeCFG::createFlow() {
  BasicBlock *Exit = ParentRegion->getExit();
  bool EntryDominatesExit = DT->dominates(ParentRegion->getEntry(), Exit);

  DeletedPhis.clear();
  AddedPhis.clear();
  Conditions.clear();
  LoopConds.clear();

  PrevNode = nullptr;
  Visited.clear();

  while (!Order.empty()) {
    handleLoops(EntryDominatesExit, nullptr);
  }

  if (PrevNode)
    changeExit(PrevNode, Exit, EntryDominatesExit);
  else
    assert(EntryDominatesExit);
}

/// Handle a rare case where the disintegrated nodes instructions
/// no longer dominate all their uses. Not sure if this is really necessary
void StructurizeCFG::rebuildSSA() {
  SSAUpdater Updater;
  for (BasicBlock *BB : ParentRegion->blocks())
    for (Instruction &I : *BB) {
      bool Initialized = false;
      // We may modify the use list as we iterate over it, so be careful to
      // compute the next element in the use list at the top of the loop.
      for (auto UI = I.use_begin(), E = I.use_end(); UI != E;) {
        Use &U = *UI++;
        Instruction *User = cast<Instruction>(U.getUser());
        if (User->getParent() == BB) {
          continue;
        } else if (PHINode *UserPN = dyn_cast<PHINode>(User)) {
          if (UserPN->getIncomingBlock(U) == BB)
            continue;
        }

        if (DT->dominates(&I, User))
          continue;

        if (!Initialized) {
          Value *Undef = UndefValue::get(I.getType());
          Updater.Initialize(I.getType(), "");
          Updater.AddAvailableValue(&Func->getEntryBlock(), Undef);
          Updater.AddAvailableValue(BB, &I);
          Initialized = true;
        }
        Updater.RewriteUseAfterInsertions(U);
      }
    }
}

StructurizeCFG::StructurizeCFG(LLVMContext &Context,
                               DominatorTree* DT,
                               LoopInfo* LI) {
  this->DT = DT;
  this->LI = LI;
  Boolean = Type::getInt1Ty(Context);
  BoolTrue = ConstantInt::getTrue(Context);
  BoolFalse = ConstantInt::getFalse(Context);
  BoolUndef = UndefValue::get(Boolean);
}

/// Run the transformation for each region found
bool StructurizeCFG::runOnRegion(Region *R) {
  if (R->isTopLevelRegion())
    return false;

  Func = R->getEntry()->getParent();
  ParentRegion = R;

  orderNodes();
  collectInfos();
  createFlow();
  insertConditions(false);
  insertConditions(true);
  setPhiValues();
  rebuildSSA();

  // Cleanup
  Order.clear();
  Visited.clear();
  DeletedPhis.clear();
  AddedPhis.clear();
  Predicates.clear();
  Conditions.clear();
  Loops.clear();
  LoopPreds.clear();
  LoopConds.clear();

  return true;
}

///////////////////////////////////////////////////////////////////////////////

class NamePrinter {
private:
  std::unique_ptr<ModuleSlotTracker> MST_;

public:
  void init(Function* function) {
    auto module = function->getParent();
    MST_ = std::make_unique<ModuleSlotTracker>(module);
    MST_->incorporateFunction(*function);
  }

  std::string ValueName(llvm::Value* V) {
    std::string str("V.");
    if (V->hasName()) {    
      str += std::string(V->getName().data(), V->getName().size());
    } else {    
      auto slot = MST_->getLocalSlot(V);
      str += std::to_string(slot);
    }
    return str;
  }

  std::string BBName(llvm::BasicBlock* BB) {
    std::string str("BB.");
    if (BB->hasName()) {    
      str += std::string(BB->getName().data(), BB->getName().size());
    } else {
      auto slot = MST_->getLocalSlot(&BB->front());
      if (slot > 0) {
        str += std::to_string(slot - 1);
      } else {
        str = "";
        dbgs() << "(" << BB->front() << ")";
      }
    }  
    return str;
  }
};

///////////////////////////////////////////////////////////////////////////////

class ReplaceSuccessor {
private:
  DenseMap<std::pair<PHINode*, BasicBlock*>, PHINode*> phi_table_;  
  NamePrinter namePrinter_;

  void findSuccessor(DenseSet<BasicBlock *>& visited, BasicBlock* current, BasicBlock* target, std::vector<BasicBlock*>& out) {
    visited.insert(current);
    auto branch = dyn_cast<BranchInst>(current->getTerminator());
    if (branch) {
      for (auto succ : branch->successors()) {
        if (succ == target) {
          out.push_back(current);
        } else {        
          if (visited.count(succ) == 0) {          
            this->findSuccessor(visited, succ, target, out);
          }      
        }
      }
    }
  }

public:

  void init(Function* function) {
    namePrinter_.init(function);
    phi_table_.clear();
  }

  void findSuccessor(BasicBlock* start, BasicBlock* target, std::vector<BasicBlock*>& out) {    
    DenseSet<BasicBlock *> visited;
    this->findSuccessor(visited, start, target, out);
  }

  bool replaceSuccessor(BasicBlock* BB, BasicBlock* oldSucc, BasicBlock* newSucc) {
    bool found = false;
    auto branch = dyn_cast<BranchInst>(BB->getTerminator());
    if (branch) {
      for (unsigned i = 0, n = branch->getNumSuccessors(); i < n; ++i) {
        auto succ = branch->getSuccessor(i);        
        if (succ == oldSucc) {
          LLVM_DEBUG(dbgs() << "****** replace " << namePrinter_.BBName(BB) << ".succ[" << i << "]: " << namePrinter_.BBName(oldSucc) << " with " << namePrinter_.BBName(newSucc) << "\n");
          branch->setSuccessor(i, newSucc);
          this->replacePhiDefs(oldSucc, BB, newSucc);
          found = true;
        }
      }
    }
    return found;
  }

  void replacePhiDefs(BasicBlock* block, BasicBlock* oldPred, BasicBlock* newPred) {
    // process all phi nodes in old successor
    for (auto II = block->begin(), IE = block->end(); II != IE; ++II) {
      PHINode *phi = dyn_cast<PHINode>(II);
      if (!phi)
        continue;

      for (unsigned op = 0, nOps = phi->getNumOperands(); op != nOps; ++op) {
        if (phi->getIncomingBlock(op) != oldPred)
          continue;        

        PHINode* phi_stub;
        auto key = std::make_pair(phi, newPred);
        auto entry = phi_table_.find(key);
        if (entry != phi_table_.end()) {
          phi_stub = entry->second;
        } else {
          // create corresponding Phi node in new block
          phi_stub = PHINode::Create(phi->getType(), 1, phi->getName(), &newPred->front());
          phi_table_[key] = phi_stub;

          // add new phi to succesor's phi node
          phi->addIncoming(phi_stub, newPred);
        }

        // move phi's operand into new phi node
        Value *del_value = phi->removeIncomingValue(op);
        phi_stub->addIncoming(del_value, oldPred);
      }
    }  
  }
};

///////////////////////////////////////////////////////////////////////////////

struct VortexBranchDivergence0 : public FunctionPass {
public:

  static char ID;
  
  VortexBranchDivergence0();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;
};

///////////////////////////////////////////////////////////////////////////////

class VortexBranchDivergence1 : public FunctionPass {
public:

  static char ID;
  
  VortexBranchDivergence1();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  
  bool runOnFunction(Function &F) override;
};

///////////////////////////////////////////////////////////////////////////////

struct VortexBranchDivergence2 : public FunctionPass {
private:
  DominatorTree *DT_;
  PostDominatorTree *PDT_;
  RegionInfo *RI_;
  ReplaceSuccessor replaceSuccessor_;

  bool separateIPDOM(Region* region, Region* parent);

public:

  static char ID;
  
  VortexBranchDivergence2();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;
};

///////////////////////////////////////////////////////////////////////////////

class VortexBranchDivergence3 : public FunctionPass {
private:

  LegacyDivergenceAnalysis *DA_;
  DominatorTree *DT_;
  PostDominatorTree *PDT_;
  LoopInfo *LI_;
  RegionInfo *RI_;

  std::vector<BasicBlock*> div_blocks_;
  std::vector<Loop*> loops_;

  NamePrinter namePrinter_;

  ReplaceSuccessor replaceSuccessor_;

  Function *tmask_func_;
  Function *pred_func_;
  Function *tmc_func_;  
  Function *split_func_;
  Function *join_func_;
  
  void initialize(Module &M, const RISCVSubtarget &ST);

  void processBranches(LLVMContext* context, Function* function);

  void processLoops(LLVMContext* context, Function* function);

public:

  static char ID;

  VortexBranchDivergence3();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  
  bool runOnFunction(Function &F) override;
};

}

///////////////////////////////////////////////////////////////////////////////

namespace llvm {

void initializeVortexBranchDivergence0Pass(PassRegistry &);
void initializeVortexBranchDivergence1Pass(PassRegistry &);
void initializeVortexBranchDivergence2Pass(PassRegistry &);
void initializeVortexBranchDivergence3Pass(PassRegistry &);

FunctionPass *createVortexBranchDivergence0Pass() {
  return new VortexBranchDivergence0();
}

FunctionPass *createVortexBranchDivergence1Pass() {
  return new VortexBranchDivergence1();
}

FunctionPass *createVortexBranchDivergence2Pass() {
  return new VortexBranchDivergence2();
}

FunctionPass *createVortexBranchDivergence3Pass() {
  return new VortexBranchDivergence3();
}


}

INITIALIZE_PASS(VortexBranchDivergence0, "vortex-branch-divergence-0",
                "Vortex Unify function exit nodes", false, false)

INITIALIZE_PASS_BEGIN(VortexBranchDivergence1, "vortex-branch-divergence-1",
                      "Vortex Structurize controlflow", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LegacyDivergenceAnalysis)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence1, "vortex-branch-divergence-1",
                    "Vortex Structurize controlflow", false, false)

INITIALIZE_PASS_BEGIN(VortexBranchDivergence2, "vortex-branch-divergence-2",
                      "Vortex IPOM Sepaation", false, false)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence2, "vortex-branch-divergence-2",
                    "Vortex IPOM Sepaation", false, false)

INITIALIZE_PASS_BEGIN(VortexBranchDivergence3, "vortex-branch-divergence-3",
                      "Vortex Branch Divergence", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LegacyDivergenceAnalysis)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(VortexBranchDivergence3, "vortex-branch-divergence-3",
                    "Vortex Branch Divergence", false, false)

namespace vortex {

char VortexBranchDivergence0::ID = 0;

StringRef VortexBranchDivergence0::getPassName() const { 
  return "Vortex Unify function exit nodes";
}

VortexBranchDivergence0::VortexBranchDivergence0() : FunctionPass(ID) {
  initializeVortexBranchDivergence0Pass(*PassRegistry::getPassRegistry());
}

void VortexBranchDivergence0::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreservedID(BreakCriticalEdgesID);
  AU.addPreservedID(LowerSwitchID);
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

bool VortexBranchDivergence0::runOnFunction(Function &F) {
  // Check if the Vortex extension is enabled
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();  
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  if (!ST.hasExtVortex())
    return false;

  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling Pass0 ***\n");

  LLVM_DEBUG(dbgs() << "*** before Pass0 changes!\n" << F << "\n");

  bool changed = false;

  std::vector<BasicBlock*> ReturningBlocks;
  std::vector<BasicBlock*> UnreachableBlocks;

  for (BasicBlock &I : F) {
    if (isa<ReturnInst>(I.getTerminator()))
      ReturningBlocks.push_back(&I);
    else if (isa<UnreachableInst>(I.getTerminator()))
      UnreachableBlocks.push_back(&I);
  }

  //
  // Handle return blocks
  //
  BasicBlock* ReturnBlock = nullptr;
  if (ReturningBlocks.empty()) {
    ReturnBlock = nullptr;
  } else if (ReturningBlocks.size() == 1) {
    ReturnBlock = ReturningBlocks.front();
  } else {
    // Otherwise, we need to insert a new basic block into the function, add a PHI
    // nodes (if the function returns values), and convert all of the return
    // instructions into unconditional branches.
    BasicBlock *NewRetBlock = BasicBlock::Create(F.getContext(), "UnifiedReturnBlock", &F);

    PHINode *PN = nullptr;
    if (F.getReturnType()->isVoidTy()) {
      ReturnInst::Create(F.getContext(), nullptr, NewRetBlock);
    } else {
      // If the function doesn't return void... add a PHI node to the block...
      PN = PHINode::Create(F.getReturnType(), ReturningBlocks.size(), "UnifiedRetVal");
      NewRetBlock->getInstList().push_back(PN);
      ReturnInst::Create(F.getContext(), PN, NewRetBlock);
    }

    // Loop over all of the blocks, replacing the return instruction with an
    // unconditional branch.
    for (BasicBlock *BB : ReturningBlocks) {
      // Add an incoming element to the PHI node for every return instruction that
      // is merging into this new block...
      if (PN)
        PN->addIncoming(BB->getTerminator()->getOperand(0), BB);

      BB->getInstList().pop_back();  // Remove the return insn
      BranchInst::Create(NewRetBlock, BB);
    }
    ReturnBlock = NewRetBlock;
    changed = true;
  }

  //
  // Handle unreacheable blocks
  // 
  BasicBlock* UnreachableBlock = nullptr;
  if (UnreachableBlocks.empty()) {
    UnreachableBlock = nullptr;
  } else if (UnreachableBlocks.size() == 1) {
    UnreachableBlock = UnreachableBlocks.front();
  } else {
    UnreachableBlock = BasicBlock::Create(F.getContext(), "UnifiedUnreachableBlock", &F);
    new UnreachableInst(F.getContext(), UnreachableBlock);
    for (BasicBlock *BB : UnreachableBlocks) {
      BB->getInstList().pop_back();  // Remove the unreachable inst.
      BranchInst::Create(UnreachableBlock, BB);
    }
    changed = true;
  }

  // Ensure single exit block
  if (UnreachableBlock && ReturnBlock) {
    
    BasicBlock *NewRetBlock = BasicBlock::Create(F.getContext(), "UnifiedReturnBlock2", &F);
    auto RetType = F.getReturnType();
    PHINode* PN = nullptr; 
    
    if (!RetType->isVoidTy()) {
      // Need to insert PhI node to merge return values from incoming blocks
      PN = PHINode::Create(RetType, ReturningBlocks.size(), "UnifiedRetVal2");      
      NewRetBlock->getInstList().push_back(PN);      

      auto DummyRetValue = llvm::Constant::getNullValue(RetType);
      PN->addIncoming(DummyRetValue, UnreachableBlock);

      PN->addIncoming(ReturnBlock->getTerminator()->getOperand(0), ReturnBlock);
    }

    ReturnInst::Create(F.getContext(), PN, NewRetBlock);

    UnreachableBlock->getInstList().pop_back();
    BranchInst::Create(NewRetBlock, UnreachableBlock);
    
    ReturnBlock->getInstList().pop_back();
    BranchInst::Create(NewRetBlock, ReturnBlock);
    
    ReturnBlock = NewRetBlock;
    changed = true;
  }

  if (changed) {
    LLVM_DEBUG(dbgs() << "*** after Pass0 changes!\n" << F << "\n");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

char VortexBranchDivergence1::ID = 0;

VortexBranchDivergence1::VortexBranchDivergence1() : FunctionPass(ID) {
  initializeVortexBranchDivergence1Pass(*PassRegistry::getPassRegistry());
}

StringRef VortexBranchDivergence1::getPassName() const { 
  return "Vortex Structurize controlflow";
}

void VortexBranchDivergence1::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();  
  AU.addRequiredID(LoopSimplifyID);
  AU.addRequired<RegionInfoPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<LegacyDivergenceAnalysis>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

bool VortexBranchDivergence1::runOnFunction(Function &F) {
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();

  // Check if the Vortex extension is enabled
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  if (!ST.hasExtVortex())
    return false;

  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling Pass1 ***\n");

  LLVM_DEBUG(dbgs() << "*** before Pass1 changes!\n" << F << "\n");

  auto *TTIWP = getAnalysisIfAvailable<TargetTransformInfoWrapperPass>();
  if (TTIWP == nullptr)
    return false;

  auto &context = F.getContext();

  auto RI  = &getAnalysis<RegionInfoPass>().getRegionInfo();
  auto LI  = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto DA  = &getAnalysis<LegacyDivergenceAnalysis>();
  auto DT  = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  auto PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  // Gather the list of regions to structurize
  // these are region that contains divergent branches that do not form a region (single-entry-single-exit)

  DenseSet<Region*> regions;

  for (auto I = df_begin(&F.getEntryBlock()),
            E = df_end(&F.getEntryBlock()); I != E; ++I) {
    auto BB = *I;
    
    auto Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br)
      continue;

    if (Br->isUnconditional())
      continue;
    
    if (DA->isUniform(Br))
      continue;

    auto ipdom = PDT->findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
    if (ipdom == nullptr)
      continue;

    auto loop = LI->getLoopFor(BB);
    if (loop == nullptr 
     || loop->contains(ipdom)) {
      auto region = RI->getRegionFor(BB);
      if (BB != region->getEntry()) {
        // this divergent block doesn't form a region
        if (regions.insert(region).second) {
          LLVM_DEBUG(dbgs() << "*** Structurize region for " << BB->getName() << "\n");
          LLVM_DEBUG(region->dump());
          LLVM_DEBUG(dbgs() << "\n");          
        }
      }
    }    
  }

  if (!regions.empty()) {
    auto SCFG = std::make_unique<StructurizeCFG>(context, DT, LI);
    for (auto region : regions) {      
      SCFG->runOnRegion(region);
    }
    LLVM_DEBUG(dbgs() << "*** after Pass1 changes!\n" << F << "\n");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

char VortexBranchDivergence2::ID = 0;

StringRef VortexBranchDivergence2::getPassName() const { 
  return "Vortex IPDOM Separation";
}

VortexBranchDivergence2::VortexBranchDivergence2() : FunctionPass(ID) {
  initializeVortexBranchDivergence2Pass(*PassRegistry::getPassRegistry());
}

void VortexBranchDivergence2::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<RegionInfoPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

bool VortexBranchDivergence2::runOnFunction(Function &F) {
  // Check if the Vortex extension is enabled
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();  
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  if (!ST.hasExtVortex())
    return false;

  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling Pass2 ***\n");

  RI_ = &getAnalysis<RegionInfoPass>().getRegionInfo();
  DT_ = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT_= &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  bool changed = this->separateIPDOM(RI_->getTopLevelRegion(), nullptr);  

  if (changed) {
    LLVM_DEBUG(dbgs() << "*** after Pass2 changes!\n" << F << "\n");
  }

  return true;
}

bool VortexBranchDivergence2::separateIPDOM(Region* region, Region* parent) {
  bool changed = false;  
  /*if (parent != nullptr) {
    auto p_exitBB = parent->getExit();
    auto r_exitBB = region->getExit();
    if (p_exitBB == r_exitBB) {
      auto function = p_exitBB->getParent();      
      auto &context = function->getContext();
      auto r_topBB = region->getEntry();
      auto stub = BasicBlock::Create(context, "ipdom_stub", function, p_exitBB);
      BranchInst::Create(p_exitBB, stub);
      replaceSuccessor_.replace(r_topBB, p_exitBB, stub, true);
      changed = true;
    } 
  }
  for (auto& subregion : *region) {
    if (this->separateIPDOM(subregion.get(), region))
      changed = true;
  }*/
  return changed;
}

///////////////////////////////////////////////////////////////////////////////

char VortexBranchDivergence3::ID = 1;

VortexBranchDivergence3::VortexBranchDivergence3() : FunctionPass(ID) {
  initializeVortexBranchDivergence3Pass(*PassRegistry::getPassRegistry());
}

StringRef VortexBranchDivergence3::getPassName() const { 
  return "Vortex Handle Branch Divergence";
}

void VortexBranchDivergence3::getAnalysisUsage(AnalysisUsage &AU) const {  
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequiredID(LoopSimplifyID);
  AU.addRequired<RegionInfoPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<LegacyDivergenceAnalysis>();
  AU.addRequired<TargetPassConfig>();
  FunctionPass::getAnalysisUsage(AU);
}

void VortexBranchDivergence3::initialize(Module &M, const RISCVSubtarget &ST) {
  tmask_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmask);
  pred_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_pred);
  tmc_func_   = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_tmc);
  split_func_ = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_split);
  join_func_  = Intrinsic::getDeclaration(&M, Intrinsic::riscv_vx_join);  
}

bool VortexBranchDivergence3::runOnFunction(Function &F) {
  // Check if the Vortex extension is enabled
  const auto &TPC = getAnalysis<TargetPassConfig>();
  const auto &TM = TPC.getTM<TargetMachine>();
  const auto &ST = TM.getSubtarget<RISCVSubtarget>(F);
  if (!ST.hasExtVortex())
    return false;

  LLVM_DEBUG(dbgs() << "*** Vortex Divergent Branch Handling Pass3 ***\n");

  this->initialize(*F.getParent(), ST);

  auto &context = F.getContext();

  RI_ = &getAnalysis<RegionInfoPass>().getRegionInfo();
  LI_ = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DA_ = &getAnalysis<LegacyDivergenceAnalysis>();
  DT_ = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT_= &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  namePrinter_.init(&F);
  replaceSuccessor_.init(&F);
    
  LLVM_DEBUG(dbgs() << "*** Region info:\n");
  LLVM_DEBUG(RI_->getTopLevelRegion()->dump());
  LLVM_DEBUG(dbgs() << "\n");

  for (auto I = df_begin(&F.getEntryBlock()),
            E = df_end(&F.getEntryBlock()); I != E; ++I) {
    auto BB = *I;

    auto Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br)
      continue;

    // only process conditional branches
    if (Br->isUnconditional()) {
      LLVM_DEBUG(dbgs() << "*** skip non-conditional branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    // only process divergent branches
    if (DA_->isUniform(Br)) {
      LLVM_DEBUG(dbgs() << "*** skip uniform branch: " << namePrinter_.BBName(BB) << "\n");
      continue;
    }

    auto ipdom = PDT_->findNearestCommonDominator(Br->getSuccessor(0), Br->getSuccessor(1));
    if (ipdom == nullptr) {
      LLVM_DEBUG(dbgs() << "*** WARNING: block with no IPDOM:\n" << BB << "\n");
      continue;
    }

    auto loop = LI_->getLoopFor(BB);
    if (loop) {
      if (std::find(loops_.begin(), loops_.end(), loop) == loops_.end()) {
        // add new loop to the list
        LLVM_DEBUG(dbgs() << "*** divergent loop: " << namePrinter_.BBName(loop->getHeader()) << "\n");
        loops_.push_back(loop);
      }

      // check if the block is a nested branch      
      if (loop->contains(ipdom)) {
        // add new branch to the list
        LLVM_DEBUG(dbgs() << "*** divergent branch: " << namePrinter_.BBName(BB) << "\n");
        div_blocks_.push_back(BB);
      }
    } else {
      // add new branch to the list
      LLVM_DEBUG(dbgs() << "*** divergent branch: " << namePrinter_.BBName(BB) << "\n");
      div_blocks_.push_back(BB);
    }    
  }

  // apply transformation
  bool changed = false;    
  if (!loops_.empty()) {
    this->processLoops(&context, &F);
    loops_.clear();
    changed = true;

    // update PDT
    PDT_->recalculate(F);
  }

  if (!div_blocks_.empty()) {
    this->processBranches(&context, &F);
    div_blocks_.clear();
    changed = true;
  }

  if (changed) {        
    LLVM_DEBUG(dbgs() << "*** after Pass3 changes!\n" << F << "\n");
  }
  
  return true;
}

void VortexBranchDivergence3::processLoops(LLVMContext* context, Function* function) {
  // traverse the list in reverse order
  for (auto it = loops_.rbegin(), ite = loops_.rend(); it != ite; ++it) {
    auto loop = *it;
    auto header = loop->getHeader();

    auto preheader = loop->getLoopPreheader();
    assert(preheader);

    auto preheader_term = preheader->getTerminator();
    assert(preheader_term);

    auto preheader_br = dyn_cast<BranchInst>(preheader_term);
    assert(preheader_br);

    LLVM_DEBUG(dbgs() << "*** process loop: " << namePrinter_.BBName(header) << "\n");

    // save current thread mask in preheader
    LLVM_DEBUG( dbgs() << "*** backup loop's tmask before preheader branch: " << namePrinter_.BBName(preheader) << "\n");
    auto tmask = CallInst::Create(tmask_func_, "tmask", preheader_br);
    
    // restore thread mask at loop exit blocks
    {
      SmallVector <BasicBlock *, 8> exiting_blocks;
      loop->getExitingBlocks(exiting_blocks); // blocks inside the loop going out

      for (auto exiting_block : exiting_blocks) {
        bool predicated = false;        
        auto branch = dyn_cast<BranchInst>(exiting_block->getTerminator());
        for (auto exit_block : branch->successors()) {      
          if (!loop->contains(exit_block)) {
            if (!predicated) {
              predicated = true;
              // insert a predicate instruction to mask out threads that are exiting the loop
              LLVM_DEBUG(dbgs() << "*** insert loop's predicate before exiting block branch: " << namePrinter_.BBName(exiting_block) << "\n");              
              IRBuilder<> ir_builder(branch);
              auto cond = branch->getCondition();
              auto not_cond = ir_builder.CreateNot(cond, namePrinter_.ValueName(cond) + ".not");
              auto not_cond_i32 = ir_builder.CreateIntCast(not_cond, ir_builder.getInt32Ty(), false, namePrinter_.ValueName(not_cond) + ".i32");
              CallInst::Create(pred_func_, not_cond_i32, "", branch);
            }

            // restore thread mask before corresponding exit blocks
            auto stub = BasicBlock::Create(*context, "loop_exit_stub", function, exit_block);
            LLVM_DEBUG(dbgs() << "*** restore loop's tmask stub " << stub->getName() << " before exit block: " << namePrinter_.BBName(exit_block) << "\n");
            auto stub_br = BranchInst::Create(exit_block, stub);
            PDT_->addNewBlock(stub, exit_block);
            CallInst::Create(tmc_func_, tmask, "", stub_br);        
            replaceSuccessor_.replaceSuccessor(exiting_block, exit_block, stub);
            if (PDT_->dominates(exit_block, exiting_block)) {
              PDT_->changeImmediateDominator(exiting_block, stub);
            }
          }
        }        
      }
    }
  }
}

void VortexBranchDivergence3::processBranches(LLVMContext* context, Function* function) {  
  std::unordered_map<BasicBlock*, BasicBlock*> ipdoms;

  // pre-gather ipdoms for divergent branches
  for (auto bit = div_blocks_.rbegin(), bite = div_blocks_.rend(); bit != bite; ++bit) {
    auto block = *bit;
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);
    ipdoms[block] = PDT_->findNearestCommonDominator(branch->getSuccessor(0), branch->getSuccessor(1));
  }

  // traverse the list in reverse order
  for (auto bit = div_blocks_.rbegin(), bite = div_blocks_.rend(); bit != bite; ++bit) {
    auto block = *bit;
    auto ipdom = ipdoms[block];
    auto branch = dyn_cast<BranchInst>(block->getTerminator());
    assert(branch);

    auto region = RI_->getRegionFor(block);

    LLVM_DEBUG(dbgs() << "*** process branch " << namePrinter_.BBName(block) << ", region=" << region->getNameStr() << "\n");
        
    // insert split instruction before divergent branch
    LLVM_DEBUG(dbgs() << "*** insert split before " << namePrinter_.BBName(block) << "'s branch.\n");
    IRBuilder<> ir_builder(branch);
    auto cond = branch->getCondition();
    auto cond_i32 = ir_builder.CreateIntCast(cond, ir_builder.getInt32Ty(), false, namePrinter_.ValueName(cond) + ".i32");
    CallInst::Create(split_func_, cond_i32, "", branch);

    // insert a join stub block before ipdom
    auto stub = BasicBlock::Create(*context, "join_stub", function, ipdom);
    LLVM_DEBUG(dbgs() << "*** insert join stub " << stub->getName() << " before " << namePrinter_.BBName(ipdom) << "\n");
    auto stub_br = BranchInst::Create(ipdom, stub);
    CallInst::Create(join_func_, "", stub_br);
    PDT_->addNewBlock(stub, ipdom);
    std::vector<BasicBlock*> succs;
    replaceSuccessor_.findSuccessor(block, ipdom, succs);
    for (auto succ : succs) {
      bool found = replaceSuccessor_.replaceSuccessor(succ, ipdom, stub);
      assert(found);
      PDT_->changeImmediateDominator(succ, stub);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

DivergenceTracker::DivergenceTracker(const Function &function) {
  DenseSet<const Value *> dv_annotations;
  DenseSet<const Value *> uv_annotations;

  auto module = function.getParent();

  // Mark all TLS globals as divergent
  for (auto& GV : module->getGlobalList()) {
    if (GV.isThreadLocal()) {
      if (dv_nodes_.insert(&GV).second) {
        LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << GV << "\n");
      }
    }
  }   

  for (auto& BB : function) {
    for (auto& I : BB) {
      if (I.getMetadata("vortex.uniform") != NULL) {
        uv_annotations.insert(&I);
        uv_nodes_.insert(&I);
        LLVM_DEBUG(dbgs() << "*** uniform annotation:\n" << I << "\n");
      } else
      if (I.getMetadata("vortex.divergent") != NULL 
       || I.getMetadata("divergent") != NULL) {
        dv_annotations.insert(&I);
        dv_nodes_.insert(&I);
        LLVM_DEBUG(dbgs() << "*** divergent annotation:\n" << I << "\n");
      } else
      if (auto II = dyn_cast<llvm::IntrinsicInst>(&I)) {
        // process Vortex's annotate("vortex.uniform")) attribute
        if (II->getIntrinsicID() == llvm::Intrinsic::var_annotation) {          
          auto gep = dyn_cast<ConstantExpr>(II->getOperand(1));
          auto gv  = dyn_cast<GlobalVariable>(gep->getOperand(0));
          auto cda = dyn_cast<ConstantDataArray>(gv->getInitializer());
          if (cda->getAsCString() == "vortex.uniform") {
            auto var = II->getOperand(0);
            if (auto AI = dyn_cast<AllocaInst>(var)) {
              uv_annotations.insert(var);
              uv_nodes_.insert(var);
              LLVM_DEBUG(dbgs() << "*** uniform annotation:\n" << *AI << "\n");
            } else 
            if (auto CI = dyn_cast<CastInst>(var)) {
              auto var2 = CI->getOperand(0);              
              uv_annotations.insert(var2);
              uv_nodes_.insert(var2);
              LLVM_DEBUG(dbgs() << "*** uniform annotation:\n" << *var2 << "\n");
            }                        
          } else 
          if (cda->getAsCString() == "vortex.divergent" 
           || cda->getAsCString() == "divergent") {
            auto var = II->getOperand(0);
            if (auto AI = dyn_cast<AllocaInst>(var)) {
              dv_annotations.insert(var);
              dv_nodes_.insert(var);
              LLVM_DEBUG(dbgs() << "*** divergent annotation:\n" << *AI << "\n");
            } else 
            if (auto CI = dyn_cast<CastInst>(var)) {
              auto var2 = CI->getOperand(0);              
              dv_annotations.insert(var2);
              dv_nodes_.insert(var2);
              LLVM_DEBUG(dbgs() << "*** divergent annotation:\n" << *var2 << "\n");
            }                        
          }
        }
      }
    }
  }
   
  for (auto& BB : function) {
    for (auto& I : BB) {
      if (auto SI = dyn_cast<StoreInst>(&I)) {
        auto addr = SI->getPointerOperand();
        if (uv_annotations.count(addr) != 0) {
          auto value = SI->getValueOperand();
          if (auto CI = dyn_cast<CastInst>(value)) {  
            auto src = CI->getOperand(0);
            LLVM_DEBUG(dbgs() << "*** uniform variable:\n" << *src << "\n");
            uv_nodes_.insert(src);
          } else {
            LLVM_DEBUG(dbgs() << "*** uniform variable:\n" << *value << "\n");
            uv_nodes_.insert(value);
          }
        } else
        if (dv_annotations.count(addr) != 0) {
          auto value = SI->getValueOperand();
          if (auto CI = dyn_cast<CastInst>(value)) {  
            auto src = CI->getOperand(0);
            LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *src << "\n");
            dv_nodes_.insert(src);
          } else {
            LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *value << "\n");
            dv_nodes_.insert(value);
          }
        }
      }
    }
  }
}

bool DivergenceTracker::eval(const Value *V) {
  // Mark variable as uniform is specified via aannotation
  if (uv_nodes_.count(V) != 0) {
    LLVM_DEBUG(dbgs() << "*** uniform variable:\n" << *V << "\n");
    return false;
  }

  // Mark variable with divergent is detected as TLS
  if (dv_nodes_.count(V) != 0) {
    LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *V << "\n");
    return true;
  }

  // We conservatively assume all function arguments to potentially be divergent
  if (isa<Argument>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *V << "\n");
    return true;
  }

  // We conservatively assume function return values are divergent
  if (isa<CallInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *V << "\n");
    return true;
  }
    
  // Atomics are divergent because they are executed sequentially: when an
  // atomic operation refers to the same address in each thread, then each
  // thread after the first sees the value written by the previous thread as
  // original value.
  if (isa<AtomicRMWInst>(V) 
   || isa<AtomicCmpXchgInst>(V)) {
    LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *V << "\n");
    return true;  
  }

  // Mark loads from divergent addresses as divergent
  if (auto LD = dyn_cast<LoadInst>(V)) {  
    auto addr = LD->getPointerOperand();
    if (dv_nodes_.count(addr) != 0) {
      LLVM_DEBUG(dbgs() << "*** divergent variable:\n" << *V << "\n");
      return true;
    }
  }

  return false;
}

} // vortex