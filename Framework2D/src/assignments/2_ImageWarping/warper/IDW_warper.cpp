#include "IDW_warper.h"

namespace USTC_CG
{
    ImVec2 IDWWarper::warp(const ImVec2& p) const {
        if (start_points_.empty() || end_points_.empty() || start_points_.size() != end_points_.size()) {
            // No control points, return the original position
            return p;
        }
        float sumW=0.0f,deltaX=0.0f,deltaY=0.0f;
        for (size_t i = 0; i < start_points_.size(); ++i) {
            float dx = p.x - start_points_[i].x;
            float dy = p.y - start_points_[i].y;
            float dist_sq = dx * dx + dy * dy;

            // 如果刚好踩在控制点上，直接返回目标位置
            if (dist_sq < 1e-6f) {
                return end_points_[i]; 
            }

            float w = 1.0f / dist_sq; 
            sumW += w;
            
            // 累加位移
            deltaX += w * (end_points_[i].x - start_points_[i].x);
            deltaY += w * (end_points_[i].y - start_points_[i].y);  
        }
        float anchor_w = 1.0f / 100000.0f; 
        sumW += anchor_w;
        deltaX /= sumW;
        deltaY /= sumW; 
        return ImVec2(p.x + deltaX, p.y + deltaY);
    }
}