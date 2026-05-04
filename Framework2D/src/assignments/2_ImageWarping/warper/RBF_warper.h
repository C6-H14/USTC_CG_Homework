#pragma once

#include "warper.h"
#include <Eigen/Dense>

namespace USTC_CG
{
class RBFWarper : public Warper
{
   public:
    RBFWarper() = default;
    virtual ~RBFWarper() = default;
    void build(const std::vector<ImVec2>& start_pts, 
               const std::vector<ImVec2>& end_pts) override;
    void setEpsilon(float epsilon) { epsilon_ = epsilon; }
    enum RBFType {
        RBF_MULTIQUADRIC,      // 多二次
        RBF_GAUSSIAN,          // 高斯
        RBF_THIN_PLATE,        // 薄板样条
        RBF_INVERSE_QUADRATIC  // 逆二次
    };
    RBFType rbf_type_ = RBF_MULTIQUADRIC; // 默认使用多二次函数作为径向基函数
    void set_kernel_type(int type) {
        if (type == 0)
            rbf_type_ = RBF_MULTIQUADRIC;
        else if (type == 1)
            rbf_type_ = RBF_INVERSE_QUADRATIC;
        else if (type == 2)
            rbf_type_ = RBF_GAUSSIAN;
        else if (type == 3)
            rbf_type_ = RBF_THIN_PLATE;
        else
            rbf_type_ = RBF_MULTIQUADRIC; // 默认回退到多二次函数
    }

    ImVec2 warp(const ImVec2& p) const override;
    private:
        float radial_basis_function(float r, RBFType type) const;
        float radial_basis_function(float r) const;
        float dis(float dx, float dy) const;

        Eigen::VectorXf alpha_x_; 
        Eigen::VectorXf alpha_y_;
        float epsilon_ = 10.0f;
        static constexpr float RBF_RADIUS_SQ = 800.0f; // RBF 多二次曲面的半径平方
        static constexpr float RBF_GAUSSIAN_SQ = 1000.0f; // 高斯RBF 的半径平方
        static constexpr float RBF_RADIUS_IVSQ = 500.0f; // RBF 逆二次曲面的半径平方
        static constexpr float TPS_SCALE = 100.0f; // 薄板样条RBF 的缩放因子，控制其影响范围
        float tikhonov_lambda() const;
};
}  // namespace USTC_CG