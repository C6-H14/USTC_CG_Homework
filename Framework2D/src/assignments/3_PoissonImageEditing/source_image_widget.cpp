#include "source_image_widget.h"
#include "target_image_widget.h"

#include <algorithm>
#include <cmath>

namespace USTC_CG
{
using uchar = unsigned char;

SourceImageWidget::SourceImageWidget(
    const std::string& label,
    const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        selected_region_mask_ =
            std::make_shared<Image>(data_->width(), data_->height(), 1);
}

void SourceImageWidget::draw()
{
    // Draw the image
    ImageWidget::draw();
    // Draw selected region
    if (flag_enable_selecting_region_)
        select_region();
}

void SourceImageWidget::enable_selecting(bool flag)
{
    flag_enable_selecting_region_ = flag;
}

void SourceImageWidget::select_region()
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
    ImGuiIO& io = ImGui::GetIO();
    // Mouse events
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        mouse_click_event();
    }
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        mouse_right_click_event();
    }
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        mouse_release_event();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // Region Shape Visualization
    if (selected_shape_)
    {
        Shape::Config s = { .bias = { position_.x, position_.y },
                            .line_color = { 255, 0, 0, 255 },
                            .line_thickness = 2.0f };
        selected_shape_->draw(s);
    }   
    if (!current_points_.empty()) 
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        switch (draw_mode_)
        {
            case kPolygon:
            case kFreehand:
            {
                for (size_t i = 0; i < current_points_.size() - 1; ++i) {
                    ImVec2 p1(current_points_[i].x + position_.x, current_points_[i].y + position_.y);
                    ImVec2 p2(current_points_[i+1].x + position_.x, current_points_[i+1].y + position_.y);
                    draw_list->AddLine(p1, p2, IM_COL32(255, 0, 0, 255), 2.0f);
                    
                    if (draw_mode_ == kPolygon) {
                        draw_list->AddCircleFilled(p1, 3.0f, IM_COL32(0, 255, 0, 255)); // 画绿点
                    }
                }
                // 多边形提示封口的半透明绿线
                if (draw_mode_ == kPolygon && current_points_.size() > 2) {
                    ImVec2 p_last(current_points_.back().x + position_.x, current_points_.back().y + position_.y);
                    ImVec2 p_first(current_points_[0].x + position_.x, current_points_[0].y + position_.y);
                    draw_list->AddLine(p_last, p_first, IM_COL32(0, 255, 0, 150), 2.0f); 
                }
                break;
            }
            default: break;
        }
    }
}

void SourceImageWidget::clear_selection() {
    draw_status_ = false;
    is_drawing_ = false;
    current_points_.clear();
    selected_shape_.reset();
    
    if (selected_region_mask_) {
        for (int y = 0; y < selected_region_mask_->height(); ++y) {
            for (int x = 0; x < selected_region_mask_->width(); ++x) {
                selected_region_mask_->set_pixel(x, y, {0}); 
            }
        }
    }
}

std::shared_ptr<Image> SourceImageWidget::get_region_mask()
{
    return selected_region_mask_;
}

std::shared_ptr<Image> SourceImageWidget::get_data()
{
    return data_;
}

ImVec2 SourceImageWidget::get_position() const
{
    return start_;
}

void SourceImageWidget::mouse_click_event()
{
    ImVec2 mouse_pos = mouse_pos_in_canvas();

    if (!draw_status_ && !is_drawing_)
    {
        selected_shape_.reset(); // 清空
        current_points_.clear();

        switch (draw_mode_)
        {
            case kDefault: 
                break;
            case kRect:
            {
                draw_status_ = true;
                start_ = end_ = mouse_pos;
                selected_shape_ = std::make_unique<Rect>(start_.x, start_.y, end_.x, end_.y);
                break;
            }
            case kPolygon:
            {
                is_drawing_ = true;
                current_points_.push_back(mouse_pos);
                current_points_.push_back(mouse_pos);
                break;
            }
            case kFreehand:
            {
                is_drawing_ = true;
                current_points_.push_back(mouse_pos);
                break;
            }
        }
    }
    else
    {
        switch (draw_mode_)
        {
            case kPolygon:
            {
                if (is_drawing_)
                {
                    current_points_.back() = mouse_pos;   // 把上一个幽灵点钉死
                    current_points_.push_back(mouse_pos); // 再分裂出一个新的幽灵点
                }
                break;
            }
            default: 
                break;
        }
    }
}
void SourceImageWidget::mouse_move_event()
{
    if (!draw_status_ && !is_drawing_) return;

    ImVec2 mouse_pos = mouse_pos_in_canvas();

    switch (draw_mode_)
    {
        case kRect:
        {
            if (draw_status_)
            {
                end_ = mouse_pos;
                if (selected_shape_)
                    selected_shape_->update(end_.x, end_.y);
            }
            break;
        }
        case kPolygon:
        {
            if (is_drawing_)
            {
                current_points_.back() = mouse_pos; // 永远只让幽灵点跟着鼠标走
            }
            break;
        }
        case kFreehand:
        {
            if (is_drawing_ && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                // 距离超过 3 像素才加点，防止几万个点瞬间撑爆扫描线内存
                ImVec2 last_p = current_points_.back();
                float dx = mouse_pos.x - last_p.x;
                float dy = mouse_pos.y - last_p.y;
                if (dx * dx + dy * dy > 9.0f)
                {
                    current_points_.push_back(mouse_pos);
                }
            }
            break;
        }
        case kDefault:
        default:
            break;
    }
}
void SourceImageWidget::mouse_release_event()
{
    if (!draw_status_ && !is_drawing_) return;

    switch (draw_mode_)
    {
        case kRect:
        {
            if (draw_status_ && selected_shape_)
            {
                draw_status_ = false;
                update_selected_region();
            }
            break;
        }
        case kPolygon:
        {
            break; 
        }
        case kFreehand:
        {
            if (is_drawing_)
            {
                is_drawing_ = false; // 画笔松开即结束！
                if (current_points_.size() > 2)
                {
                    update_mask_from_polygon(current_points_);
                }
                else 
                { 
                    current_points_.clear(); 
                }
            }
            break;
        }
        case kDefault:
        default:
            break;
    }
}void SourceImageWidget::mouse_right_click_event()
{
    // 如果正在画多边形，右键代表“结束并封口”
    if (draw_mode_ == kPolygon && is_drawing_)
    {
        is_drawing_ = false;
        
        if (!current_points_.empty()) {
            current_points_.pop_back();
        }

        if (current_points_.size() > 2) {
            update_mask_from_polygon(current_points_);
        } else {
            current_points_.clear();
        }
    }
    else if (!is_drawing_ && !draw_status_)
    {
        selected_shape_.reset();
        current_points_.clear();
        
        if (selected_region_mask_) {
            for (int y = 0; y < selected_region_mask_->height(); ++y) {
                for (int x = 0; x < selected_region_mask_->width(); ++x) {
                    selected_region_mask_->set_pixel(x, y, {0}); 
                }
            }
        }
    }
}
ImVec2 SourceImageWidget::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    // The position should not be out of the canvas
    const ImVec2 mouse_pos_in_canvas(
        std::clamp<float>(io.MousePos.x - position_.x, 0, (float)image_width_),
        std::clamp<float>(
            io.MousePos.y - position_.y, 0, (float)image_height_));
    return mouse_pos_in_canvas;
}

void SourceImageWidget::update_selected_region()
{
    if (selected_shape_ == nullptr)
        return;
    // HW3_TODO(Optional): The selected_shape_ call its get_interior_pixels()
    // function to get the interior pixels. For other shapes, you can implement
    // their own get_interior_pixels()
    std::vector<std::pair<int, int>> interior_pixels =
        selected_shape_->get_interior_pixels();
    // Clear the selected region mask
    for (int i = 0; i < selected_region_mask_->width(); ++i)
        for (int j = 0; j < selected_region_mask_->height(); ++j)
            selected_region_mask_->set_pixel(i, j, { 0 });
    // Set the selected pixels with 255
    for (const auto& pixel : interior_pixels)
    {
        int x = pixel.first;
        int y = pixel.second;
        if (x < 0 || x >= selected_region_mask_->width() || 
            y < 0 || y >= selected_region_mask_->height())
            continue;
        selected_region_mask_->set_pixel(x, y, { 255 });
    }
}
void SourceImageWidget::update_mask_from_polygon(const std::vector<ImVec2>& points) {
    if (points.size() < 3) return;

    // 1. 初始化 Mask 为全黑 (0)
    int width = selected_region_mask_->width();
    int height = selected_region_mask_->height();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            selected_region_mask_->set_pixel(x, y, {0});
        }
    }

    // 2. 找到多边形的 Bounding Box，优化扫描范围
    int min_y = height, max_y = 0;
    int min_x = width, max_x = 0;
    for (const auto& p : points) {
        min_y = std::min(min_y, static_cast<int>(p.y));
        max_y = std::max(max_y, static_cast<int>(p.y));
        min_x = std::min(min_x, static_cast<int>(p.x));
        max_x = std::max(max_x, static_cast<int>(p.x));
    }
    min_y = std::max(0, min_y); max_y = std::min(height - 1, max_y);
    min_x = std::max(0, min_x); max_x = std::min(width - 1, max_x);

    // 3. 逐行扫描
    for (int y = min_y; y <= max_y; ++y) {
        std::vector<int> intersections;

        for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
            float yi = points[i].y;
            float yj = points[j].y;
            float xi = points[i].x;
            float xj = points[j].x;

            if ((yi > y) != (yj > y)) {
                float intersect_x = xi + (y - yi) * (xj - xi) / (yj - yi);
                intersections.push_back(static_cast<int>(intersect_x));
            }
        }

        std::sort(intersections.begin(), intersections.end());

        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int start_x = std::max(min_x, intersections[i]);
            int end_x = std::min(max_x, intersections[i + 1]);
            
            for (int x = start_x; x <= end_x; ++x) {
                selected_region_mask_->set_pixel(x, y, {255}); 
            }
        }
    }
    
    start_ = ImVec2(static_cast<float>(min_x), static_cast<float>(min_y));
}
}  // namespace USTC_CG