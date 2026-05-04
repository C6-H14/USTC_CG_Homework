// HW2_TODO: Implement the IDWWarper class
#pragma once

#include "warper.h"

namespace USTC_CG
{
class IDWWarper : public Warper
{
   public:
    IDWWarper() = default;
    virtual ~IDWWarper() = default;
    ImVec2 warp(const ImVec2& p) const override;

};
}  // namespace USTC_CG