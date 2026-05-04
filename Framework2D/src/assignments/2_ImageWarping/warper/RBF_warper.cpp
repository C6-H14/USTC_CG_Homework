#include "RBF_warper.h"
#include <cmath>

namespace USTC_CG
{
    float RBFWarper::radial_basis_function(float r) const {
        switch(rbf_type_) {
            case RBF_MULTIQUADRIC:
                return std::sqrt(RBF_RADIUS_SQ + r);
            case RBF_GAUSSIAN:
                return std::exp(-r/RBF_GAUSSIAN_SQ);
            case RBF_THIN_PLATE:
                return (r < 1e-6f) ? 0.0f : (0.5f *  r / TPS_SCALE * std::log( r / TPS_SCALE+1.0f));
            case RBF_INVERSE_QUADRATIC:
                return RBF_RADIUS_IVSQ / (RBF_RADIUS_IVSQ + r);
        }
        return r;
    }
    
    float RBFWarper::tikhonov_lambda() const {
        switch(rbf_type_) {
            case RBF_MULTIQUADRIC:
                return 0.01f;
            case RBF_GAUSSIAN:
                return 1e-6;
            case RBF_THIN_PLATE:
                return 10.0f;
            case RBF_INVERSE_QUADRATIC:
                return 1e-6;
        }
        return 1e-6;
    }

    float RBFWarper::dis(float dx, float dy) const {
        return dx * dx + dy * dy;
    }
    void RBFWarper::build(const std::vector<ImVec2>& start_pts, 
                      const std::vector<ImVec2>& end_pts) {
        Warper::build(start_pts, end_pts);
        int N = start_points_.size();
    
        if (N == 0){
            return;
        }

        Eigen::MatrixXf A(N, N);
        Eigen::VectorXf bx(N);
        Eigen::VectorXf by(N);
        float TIKHONOV_LAMBDA = tikhonov_lambda();

        for (int i = 0; i < N; ++i) {
            bx(i) = end_points_[i].x - start_points_[i].x;
            by(i) = end_points_[i].y - start_points_[i].y;
            for (int j = 0; j < N; ++j) {
                float dx = start_points_[i].x - start_points_[j].x;
                float dy = start_points_[i].y - start_points_[j].y;
                float dist_sq = dis(dx,dy);
                
                A(i, j) = radial_basis_function(dist_sq);
            }
            A(i, i) += TIKHONOV_LAMBDA; 
        }

        alpha_x_ = A.colPivHouseholderQr().solve(bx);
        alpha_y_ = A.colPivHouseholderQr().solve(by);
    }

    ImVec2 RBFWarper::warp(const ImVec2& p) const {
        if (start_points_.empty()) {
            return p;
        }

        float delta_x = 0.0f;
        float delta_y = 0.0f;

        for (size_t i = 0; i < start_points_.size(); ++i) {
            float dx = p.x - start_points_[i].x;
            float dy = p.y - start_points_[i].y;
            float dist_sq = dis(dx,dy);

            float rbf_val = radial_basis_function(dist_sq);

            delta_x += alpha_x_(i) * rbf_val;
            delta_y += alpha_y_(i) * rbf_val;
        }

        return ImVec2(p.x + delta_x, p.y + delta_y);
    }
}