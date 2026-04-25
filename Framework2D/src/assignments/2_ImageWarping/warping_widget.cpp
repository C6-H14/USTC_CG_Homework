#include "warping_widget.h"

#include <cmath>
#include <algorithm>
#include "warper/IDW_warper.h"
#include "warper/RBF_warper.h"
#include "warper/warper.h"


namespace USTC_CG
{
using uchar = unsigned char;

WarpingWidget::WarpingWidget(const std::string& label, const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void WarpingWidget::draw()
{
    // Draw the image
    ImageWidget::draw();
    // Draw the canvas
    if (flag_enable_selecting_points_)
        select_points();
}

void WarpingWidget::invert()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            data_->set_pixel(
                i,
                j,
                { static_cast<uchar>(255 - color[0]),
                  static_cast<uchar>(255 - color[1]),
                  static_cast<uchar>(255 - color[2]) });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::mirror(bool is_horizontal, bool is_vertical)
{
    Image image_tmp(*data_);
    int width = data_->width();
    int height = data_->height();

    if (is_horizontal)
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i,
                        j,
                        image_tmp.get_pixel(width - 1 - i, height - 1 - j));
                }
            }
        }
        else
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(width - 1 - i, j));
                }
            }
        }
    }
    else
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(i, height - 1 - j));
                }
            }
        }
    }

    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::gray_scale()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            uchar gray_value = (color[0] + color[1] + color[2]) / 3;
            data_->set_pixel(i, j, { gray_value, gray_value, gray_value });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
inline void WarpingWidget::bilinear_sample(const unsigned char* src_data, int width, int height, int channels,
                            float src_x, float src_y, 
                            unsigned char* dst_data, int dst_idx) const 
{
    src_x = std::clamp(src_x, 0.0f, static_cast<float>(width - 1));
    src_y = std::clamp(src_y, 0.0f, static_cast<float>(height - 1));

    int x0 = static_cast<int>(src_x);
    int y0 = static_cast<int>(src_y);
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1); 

    float u = src_x - x0;
    float v = src_y - y0;

    int idx00 = (y0 * width + x0) * channels;
    int idx10 = (y0 * width + x1) * channels;
    int idx01 = (y1 * width + x0) * channels;
    int idx11 = (y1 * width + x1) * channels;

    for (int ch = 0; ch < channels; ++ch) {
        float c00 = src_data[idx00 + ch];
        float c10 = src_data[idx10 + ch];
        float c01 = src_data[idx01 + ch];
        float c11 = src_data[idx11 + ch];

        float top_mix = c00 + u * (c10 - c00);
        float bot_mix = c01 + u * (c11 - c01);
        
        float final_color = top_mix + v * (bot_mix - top_mix);

        dst_data[dst_idx + ch] = static_cast<unsigned char>(final_color + 0.5f);
    }
}
void WarpingWidget::warping()
{
    if (!data_ || data_->width() <= 0 || data_->height() <= 0) { //无图可变形
        return;
    }
    // 没有画任何线
    if (start_points_.empty() && warping_type_ != kFisheye) {
        return; 
    }

    // Create a new image to store the result
    Image warped_image(*data_);
    std::memset(warped_image.data(), 0, warped_image.width() * warped_image.height() * warped_image.channels());
    std::shared_ptr<Warper> warper = nullptr;
    int width = data_->width();
    int height = data_->height();
    int channels = data_->channels();

    switch (warping_type_)
    {
        case kDefault: break;
        case kFisheye:
        {
            // Example: (simplified) "fish-eye" warping
            // For each (x, y) from the input image, the "fish-eye" warping
            // transfer it to (x', y') in the new image: Note: For this
            // transformation ("fish-eye" warping), one can also calculate the
            // inverse (x', y') -> (x, y) to fill in the "gaps".
            for (int y = 0; y < data_->height(); ++y)
            {
                for (int x = 0; x < data_->width(); ++x)
                {
                    // Apply warping function to (x, y), and we can get (x', y')
                    auto [new_x, new_y] =
                        fisheye_warping(x, y, data_->width(), data_->height());
                    // Copy the color from the original image to the result
                    // image
                    if (new_x >= 0 && new_x < data_->width() && new_y >= 0 &&
                        new_y < data_->height())
                    {
                        std::vector<unsigned char> pixel =
                            data_->get_pixel(x, y);
                        warped_image.set_pixel(new_x, new_y, pixel);
                    }
                }
            }
            break;
        }
        case kIDW:{
            warper = std::make_shared<IDWWarper>();
            break;
        }
        case kRBF:{
            std::shared_ptr<RBFWarper> rbf_warper = std::make_shared<RBFWarper>();
            rbf_warper->set_kernel_type(this->rbf_kernel_type_); 
            warper = rbf_warper; 
            break;
        }
        default: break;
    }
    if (warper) 
    {
        //角落固定
        std::vector<ImVec2> augmented_start = start_points_;
        std::vector<ImVec2> augmented_end = end_points_;

        float fw = static_cast<float>(width - 1);
        float fh = static_cast<float>(height - 1);
        ImVec2 corners[4] = {
            ImVec2(0.0f, 0.0f), ImVec2(fw, 0.0f),
            ImVec2(0.0f, fh),   ImVec2(fw, fh)
        };

        for (int i = 0; i < 4; ++i) {
            augmented_start.push_back(corners[i]);
            augmented_end.push_back(corners[i]);
        }

        // 1. 预计算反向映射场 
        warper->build(augmented_end, augmented_start); 

        // 2. 拿到直接内存指针
        const unsigned char* src_data = data_->data(); 
        unsigned char* dst_data = warped_image.data();

        // 3. 执行核心渲染 
        if (use_mesh_acceleration_) {
            apply_mesh_warping(warper, src_data, dst_data, width, height, channels);
        } else {
            apply_pixel_warping(warper, src_data, dst_data, width, height, channels);
        }
    }
    *data_ = std::move(warped_image);
    update();
}

// 逐像素RBF/IDW渲染
void WarpingWidget::apply_pixel_warping(const std::shared_ptr<Warper>& warper, 
                                        const unsigned char* src_data, unsigned char* dst_data, 
                                        int width, int height, int channels) const 
{
    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 注意：最好加上 static_cast<float>，消除 int 到 float 的隐式转换警告
            ImVec2 src_pos = warper->warp(ImVec2(static_cast<float>(x), static_cast<float>(y)));
            int dst_idx = (y * width + x) * channels;
            
            bilinear_sample(src_data, width, height, channels, 
                            src_pos.x, src_pos.y, 
                            dst_data, dst_idx);
        }
    }
}

// 网格细分与双线性插值
void WarpingWidget::apply_mesh_warping(const std::shared_ptr<Warper>& warper, 
                                       const unsigned char* src_data, unsigned char* dst_data, 
                                       int width, int height, int channels) const 
{
    int grid_cols = (width + MESH_GRID_SIZE - 1) / MESH_GRID_SIZE + 1;
    int grid_rows = (height + MESH_GRID_SIZE - 1) / MESH_GRID_SIZE + 1;
    
    // 预计算网格顶点的原图坐标
    std::vector<std::vector<ImVec2>> grid_src_pos(grid_rows, std::vector<ImVec2>(grid_cols));
    for (int r = 0; r < grid_rows; ++r) {
        for (int c = 0; c < grid_cols; ++c) {
            grid_src_pos[r][c] = warper->warp(ImVec2(static_cast<float>(c * MESH_GRID_SIZE), 
                                                     static_cast<float>(r * MESH_GRID_SIZE)));
        }
    }

    // 内部像素极速插值
    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int c = x / MESH_GRID_SIZE;
            int r = y / MESH_GRID_SIZE;
            float u = static_cast<float>(x % MESH_GRID_SIZE) / MESH_GRID_SIZE;
            float v = static_cast<float>(y % MESH_GRID_SIZE) / MESH_GRID_SIZE;

            ImVec2 p00 = grid_src_pos[r][c];         
            ImVec2 p10 = grid_src_pos[r][c + 1];     
            ImVec2 p01 = grid_src_pos[r + 1][c];     
            ImVec2 p11 = grid_src_pos[r + 1][c + 1]; 

            float src_pos_x = (p00.x + u * (p10.x - p00.x)) * (1.0f - v) + 
                              (p01.x + u * (p11.x - p01.x)) * v;
            float src_pos_y = (p00.y + u * (p10.y - p00.y)) * (1.0f - v) + 
                              (p01.y + u * (p11.y - p01.y)) * v;

            int dst_idx = (y * width + x) * channels;
            
            bilinear_sample(src_data, width, height, channels, 
                            src_pos_x, src_pos_y, 
                            dst_data, dst_idx);
        }
    }
}

void WarpingWidget::restore()
{
    *data_ = *back_up_;
    update();
}
void WarpingWidget::set_default()
{
    warping_type_ = kDefault;
}
void WarpingWidget::set_fisheye()
{
    warping_type_ = kFisheye;
}
void WarpingWidget::set_IDW()
{
    warping_type_ = kIDW;
}
void WarpingWidget::set_RBF()
{
    warping_type_ = kRBF;
}
void WarpingWidget::enable_selecting(bool flag)
{
    flag_enable_selecting_points_ = flag;
}
void WarpingWidget::select_points()
{
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    bool is_hovered_ = ImGui::IsItemHovered();
    // Selections
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        start_ = end_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
    }
    if (draw_status_)
    {
        end_ = ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            start_points_.push_back(start_);
            end_points_.push_back(end_);
            draw_status_ = false;
        }
    }
    // Visualization
    auto draw_list = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < start_points_.size(); ++i)
    {
        ImVec2 s(
            start_points_[i].x + position_.x, start_points_[i].y + position_.y);
        ImVec2 e(
            end_points_[i].x + position_.x, end_points_[i].y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }
    if (draw_status_)
    {
        ImVec2 s(start_.x + position_.x, start_.y + position_.y);
        ImVec2 e(end_.x + position_.x, end_.y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
    }
}
void WarpingWidget::init_selections()
{
    start_points_.clear();
    end_points_.clear();
}

std::pair<int, int>
WarpingWidget::fisheye_warping(int x, int y, int width, int height)
{
    float center_x = width / 2.0f;
    float center_y = height / 2.0f;
    float dx = x - center_x;
    float dy = y - center_y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Simple non-linear transformation r -> r' = f(r)
    float new_distance = std::sqrt(distance) * 10;

    if (distance == 0)
    {
        return { static_cast<int>(center_x), static_cast<int>(center_y) };
    }
    // (x', y')
    float ratio = new_distance / distance;
    int new_x = static_cast<int>(center_x + dx * ratio);
    int new_y = static_cast<int>(center_y + dy * ratio);

    return { new_x, new_y };
}
void WarpingWidget::set_rbf_kernel_type(int type) {
        rbf_kernel_type_ = type;
    }
}  // namespace USTC_CG