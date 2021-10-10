#pragma once

namespace ff::graphics
{
    IDWriteFactoryX* write_factory();
    IDWriteInMemoryFontFileLoader* write_font_loader();
}

namespace ff::graphics::defer
{
    void set_full_screen_target(ff::dxgi::target_window_base* target);
    void remove_target(ff::dxgi::target_window_base* target);
    void resize_target(ff::dxgi::target_window_base* target, const ff::window_size& size);
    void validate_device(bool force);
    void full_screen(bool value);
}

namespace ff::internal::graphics
{
    bool init();
    void destroy();
}
