#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    uint i = 1;
    uint j = 0;
    float psi[2] = { f_out[0][uint3(pos, 9)], f_out[1][uint3(pos, 9)] };
    float rho[2] = { f_in[0][uint3(pos, 9)], f_in[1][uint3(pos, 9)] };
    float2 F[2] = { { 0.f, 0.f }, { 0.f, 0.f } };
    
    const float2x2 g =
    {
        -4.f, 0.35f,
        0.35f, -0.36f
    };
    
    [unroll]
    for (j = 0; j < 2; j++)
    {
        if (rho[j] > 0.f)
        {
            for (i = 1; i < 9; i++)
            {
                float _psi0 = f_out[0][uint3(pos + c[i], 9)];
                float _psi1 = f_out[1][uint3(pos + c[i], 9)];
                F[j] += (g[j][0] * _psi0 + g[j][1] * _psi1) * c[i] * w[i];
            }
            F[j] *= -psi[j];
            F[j] += float2(0.f, 0.01f) * rho[j];
        }
        f_out[j][uint3(pos, 10)] = F[j].x;
        f_out[j][uint3(pos, 11)] = F[j].y;
    }
}
