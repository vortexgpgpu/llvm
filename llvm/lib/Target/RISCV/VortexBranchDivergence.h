#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

namespace vortex {
using namespace llvm;

class DivergenceTracker {
private:
    DenseSet<const Value *> DV_;

public:
    DivergenceTracker(const Function &function);

    bool eval(const Value *V);
};

}