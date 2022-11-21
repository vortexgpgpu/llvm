#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

namespace vortex {
using namespace llvm;

class DivergenceTracker {
private:
    DenseSet<const Value *> dv_nodes_;
    DenseSet<const Value *> uv_nodes_;

public:
    DivergenceTracker(const Function &function);

    bool eval(const Value *V);
};

}