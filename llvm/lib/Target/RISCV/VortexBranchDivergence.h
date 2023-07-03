#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

namespace vortex {
using namespace llvm;

class DivergenceTracker {
public:
    DivergenceTracker(const Function &function);

    bool eval(const Value *V);

private:    
    void initialize();

    DenseSet<const Value *> dv_nodes_;
    DenseSet<const Value *> uv_nodes_;
    const Function* function_;
    bool initialized_;   
};

}