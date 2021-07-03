#pragma once

namespace ff
{
    class dx11_device_state;
    class dx11_object_cache;
    class target_window_base;
}

namespace ff::internal
{
    class graphics_child_base;
}

namespace ff::graphics
{
    bool reset(bool force);

    IDXGIFactoryX* dxgi_factory();
    IDWriteFactoryX* write_factory();
    IDWriteInMemoryFontFileLoader* write_font_loader();

    IDXGIDeviceX* dxgi_device();
    IDXGIFactoryX* dxgi_factory_for_device();
    IDXGIAdapterX* dxgi_adapter_for_device();
    D3D_FEATURE_LEVEL dx_feature_level();

    ID3D11DeviceX* dx11_device();
    ID3D11DeviceContextX* dx11_device_context();
    ff::dx11_device_state& dx11_device_state();
    ff::dx11_object_cache& dx11_object_cache();
}

namespace ff::graphics::defer
{
    void set_full_screen_target(ff::target_window_base* target);
    void remove_target(ff::target_window_base* target);
    void resize_target(ff::target_window_base* target, const ff::window_size& size);
    void validate_device(bool force);
    void full_screen(bool value);
}

namespace ff::internal::graphics
{
    bool init();
    void destroy();

    void add_child(ff::internal::graphics_child_base* child);
    void remove_child(ff::internal::graphics_child_base* child);
}
