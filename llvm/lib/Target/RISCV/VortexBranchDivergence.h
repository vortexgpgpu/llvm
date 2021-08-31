#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"

namespace vortex {
using namespace llvm;

class isSourceOfDivergenceHandler {
private:
    DenseSet<const Value *> DV_;

public:
    bool eval(const Value *V);
};

}