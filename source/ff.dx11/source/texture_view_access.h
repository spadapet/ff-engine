#pragma once

namespace ff::dx11
{
    class texture_view_access : public ff::dxgi::texture_view_access_base
    {
    public:
        static const texture_view_access& get(const ff::dxgi::texture_view_base& obj);

        virtual ID3D11ShaderResourceView* dx11_texture_view() const = 0;
    };
}
