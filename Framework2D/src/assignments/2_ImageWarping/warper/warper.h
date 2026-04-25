// HW2_TODO: Please implement the abstract class Warper
// 1. The Warper class should abstract the **mathematical mapping** involved in
// the warping problem, **independent of image**.
// 2. The Warper class should have a virtual function warp(...) to be called in
// our image warping application.
//    - You should design the inputs and outputs of warp(...) according to the
//    mathematical abstraction discussed in class.
//    - Generally, the warping map should map one input point to another place.
// 3. Subclasses of Warper, IDWWarper and RBFWarper, should implement the
// warp(...) function to perform the actual warping.
#pragma once
#include <vector>
#include <imgui.h>

namespace USTC_CG
{
class Warper
{
   public:
    virtual ~Warper() = default;

    virtual void build(const std::vector<ImVec2>& start_pts, 
                       const std::vector<ImVec2>& end_pts) 
    {
        start_points_ = start_pts;
        end_points_ = end_pts;
    }
    virtual ImVec2 warp(const ImVec2& p) const = 0;
        protected:
        std::vector<ImVec2> start_points_;
        std::vector<ImVec2> end_points_;
    };
    

}  // namespace USTC_CG