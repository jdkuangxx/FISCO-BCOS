#pragma once

#include <runtime/data_type.h>
#include <tir/node_base.h>

namespace tdc {
namespace tir {

class ExprBase : public NodeBase {
   public:
   protected:
    runtime::DataType dtype_;
};

}  // namespace tir
}  // namespace tdc
