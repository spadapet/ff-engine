#pragma once

namespace ff
{
    class dx11_texture_view_base;
    enum class sprite_type;

    class sprite_data
    {
    public:
        sprite_data(
            std::string_view name,
            ff::dx11_texture_view_base* view,
            ff::rect_float texture_uv,
            ff::rect_float world,
            ff::sprite_type type);
        sprite_data(sprite_data&& other) noexcept = default;
        sprite_data(const sprite_data& other) = default;

        sprite_data& operator=(sprite_data&& other) noexcept = default;
        sprite_data& operator=(const sprite_data & other) = default;

        std::string_view name() const;
        ff::dx11_texture_view_base* view() const;
        const ff::rect_float& texture_uv() const;
        const ff::rect_float& world() const;
        ff::sprite_type type() const;

        ff::rect_float texture_rect() const;
        ff::point_float scale() const;
        ff::point_float handle() const;

    private:
        std::string_view name_;
        ff::dx11_texture_view_base* view_;
        ff::rect_float texture_uv_;
        ff::rect_float world_;
        ff::sprite_type type_;
    };
}
