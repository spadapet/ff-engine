#pragma once

#include "animation_base.h"
#include "animation_player_base.h"
#include "sprite_base.h"

namespace ff
{
    class texture_view_base;

    class sprite
        : public ff::sprite_base
        , public ff::animation_base
        , public ff::animation_player_base
    {
    public:
        sprite(std::string&& name, const std::shared_ptr<ff::dxgi::texture_view_base>& view, const ff::dxgi::sprite_data& sprite_data);
        sprite(std::string&& name, const std::shared_ptr<ff::dxgi::texture_view_base>& view, ff::rect_float rect, ff::point_float handle, ff::point_float scale, ff::dxgi::sprite_type type);
        sprite(sprite&& other) noexcept = default;
        sprite(const sprite& other) = default;

        sprite& operator=(sprite&& other) noexcept = default;
        sprite& operator=(const sprite& other) = default;

        // sprite_base
        virtual std::string_view name() const override;
        virtual const ff::dxgi::sprite_data& sprite_data() const override;

        // animation_base
        virtual float frame_length() const override;
        virtual float frames_per_second() const override;
        virtual void frame_events(float start, float end, bool include_start, ff::push_base<ff::animation_event>& events) override;
        virtual void draw_frame(ff::dxgi::draw_base& draw, const ff::dxgi::transform& transform, float frame, const ff::dict* params = nullptr) override;
        virtual void draw_frame(ff::dxgi::draw_base& draw, const ff::dxgi::pixel_transform& transform, float frame, const ff::dict* params = nullptr) override;
        virtual ff::value_ptr frame_value(size_t value_id, float frame, const ff::dict* params = nullptr) override;

        // animation_player_base
        virtual void advance_animation(ff::push_base<ff::animation_event>* events) override;
        virtual void draw_animation(ff::dxgi::draw_base& draw, const ff::dxgi::transform& transform) const override;
        virtual void draw_animation(ff::dxgi::draw_base& draw, const ff::dxgi::pixel_transform& transform) const override;
        virtual float animation_frame() const override;
        virtual const ff::animation_base* animation() const override;

    private:
        std::string name_;
        std::shared_ptr<ff::dxgi::texture_view_base> view;
        ff::dxgi::sprite_data sprite_data_;
    };
}
