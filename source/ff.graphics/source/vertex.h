#pragma once

namespace ff::vertex
{
    struct line_geometry
    {
        DirectX::XMFLOAT2 position[4]; // adjacency at 0 and 3
        DirectX::XMFLOAT4 color[2];
        float thickness[2];
        float depth;
        UINT matrix_index;

        static const std::array<ff_dx::D3D_INPUT_ELEMENT_DESC, 10>& layout();
    };

    struct circle_geometry
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 inside_color;
        DirectX::XMFLOAT4 outside_color;
        float radius;
        float thickness;
        UINT matrix_index;

        static const std::array<ff_dx::D3D_INPUT_ELEMENT_DESC, 6>& layout();
    };

    struct triangle_geometry
    {
        DirectX::XMFLOAT2 position[3];
        DirectX::XMFLOAT4 color[3];
        float depth;
        UINT matrix_index;

        static const std::array<ff_dx::D3D_INPUT_ELEMENT_DESC, 8>& layout();
    };

    struct sprite_geometry
    {
        DirectX::XMFLOAT4 rect;
        DirectX::XMFLOAT4 uv_rect;
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT2 scale;
        DirectX::XMFLOAT3 position;
        float rotate;
        UINT texture_index;
        UINT matrix_index;

        static const std::array<ff_dx::D3D_INPUT_ELEMENT_DESC, 8>& layout();
    };
}
