#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    uint i = 1;
    uint j = 0;
    
    const float2x2 g =
    {
        -4.f, 0.35f,
        0.35f, -0.37f
    };
    
    [unroll]
    for (j = 0; j < 2; j++)
    {
        float psi = f_out[j][uint3(pos, 9)];
        float rho = f_in[j][uint3(pos, 9)];
        float2 F = { 0.f, 0.f };
        if (rho > 0.f)
        {
            for (i = 1; i < 9; i++)
            {
                float _psi0 = f_out[0][uint3(pos + c[i], 9)];
                float _psi1 = f_out[1][uint3(pos + c[i], 9)];
                F += (g[j][0] * _psi0 + g[j][1] * _psi1) * c[i] / pow(length(c[i]), 2.f) * 0.08f;
            }
            F *= -psi;
            F += float2(0.f, 0.01f) * rho;
        }
        f_out[j][uint3(pos, 10)] = F.x;
        f_out[j][uint3(pos, 11)] = F.y;
    }
}
