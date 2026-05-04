#pragma once

#include "common/image_widget.h"
#include <memory>
#include "warper/warper.h"

namespace USTC_CG
{
// Image component for warping and other functions
class WarpingWidget : public ImageWidget
{
   public:
    explicit WarpingWidget(
        const std::string& label,
        const std::string& filename);
    virtual ~WarpingWidget() noexcept = default;

    void draw() override;

    // Simple edit functions
    void invert();
    void mirror(bool is_horizontal, bool is_vertical);
    void gray_scale();
    void warping();
    void restore();

    // Enumeration for supported warping types.
    // HW2_TODO: more warping types.
    enum WarpingType
    {
        kDefault = 0,
        kFisheye = 1,
        kIDW = 2,
        kRBF = 3,
    };
    // Warping type setters.
    void set_default();
    void set_fisheye();
    void set_IDW();
    void set_RBF();

    // Point selecting interaction
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();
    void set_mesh_acceleration(bool enable) {
        use_mesh_acceleration_ = enable;
    }
    void set_rbf_kernel_type(int type);

   private:
    static constexpr int MESH_GRID_SIZE = 20; // 网格加速的划分尺寸
    // Store the original image data
    std::shared_ptr<Image> back_up_;
    // The selected point couples for image warping
    std::vector<ImVec2> start_points_, end_points_;
    int rbf_kernel_type_ = 0; 

    ImVec2 start_, end_;
    bool flag_enable_selecting_points_ = false;
    bool draw_status_ = false;
    WarpingType warping_type_;
    bool use_mesh_acceleration_ = false; // 是否开启网格加速
    inline void bilinear_sample(const unsigned char* src_data, int width, int height, int channels,
                            float src_x, float src_y, 
                            unsigned char* dst_data, int dst_idx) const;
    // 逐像素渲染
    void apply_pixel_warping(const std::shared_ptr<Warper>& warper, 
                            const unsigned char* src_data, unsigned char* dst_data, 
                            int width, int height, int channels) const;

    // 网格加速渲染
    void apply_mesh_warping(const std::shared_ptr<Warper>& warper, 
                            const unsigned char* src_data, unsigned char* dst_data, 
                            int width, int height, int channels) const;

    private:
        // A simple "fish-eye" warping function
        std::pair<int, int> fisheye_warping(int x, int y, int width, int height);
};
}  // namespace USTC_CG