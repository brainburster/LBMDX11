#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

bool is_wall(uint2 pos)
{
    if (pos.y > (600 / 8 - 1) && pos.y < (0u-1u) || pos.x > (800 / 8 - 1))
    {
        return true;
    }
    return uav_display[pos].w < 0.0f;
}

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    uint i = 1;
    uint j = 0;
    
    const float4x4 g =
    {
        -5.2f, 0.6f, 0.1f, -4.f,
        .6f, -0.7f, 0.1f, -.3f,
        0.1f, 0.1f, 0.0f, 0.f,
        -4.f, -.3f, 0.f, 0,
    };
    
    [unroll]
    for (j = 0; j < 3; j++)
    {
        float psi = f_out[j][uint3(pos, 9)];
        float rho = f_in[j][uint3(pos, 9)];
        float2 F = { 0.f, 0.f };
        if (rho > 0.f)
        {
            for (i = 1; i < 9; i++)
            {
                uint2 pos2 = pos + c[i];
                float _psi0 = f_out[0][uint3(pos2, 9)];
                float _psi1 = f_out[1][uint3(pos2, 9)];
                float _psi2 = f_out[2][uint3(pos2, 9)];
                float _psi3 = is_wall(pos2) ? 1.f : 0.f;
                
                F += (g[j][0] * _psi0 + g[j][1] * _psi1 + g[j][2] * _psi2 + g[j][3] * _psi3) * c[i] / pow(length(c[i]), 2.f) * 0.08f;
            }
            F *= -psi;
            F += j < 2 ? float2(0.f, 0.03f) * rho : 0.f;
        }
        f_out[j][uint3(pos, 10)] = F.x;
        f_out[j][uint3(pos, 11)] = F.y;
    }
}
