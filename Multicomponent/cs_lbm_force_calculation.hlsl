#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    uint i = 1;
    float psi0 = f_out[0][uint3(pos, 9)];
    float rho0 = f_in[0][uint3(pos, 9)];
    float2 F0 = { 0.f, 0.f };
    const float g00 = -8.f;
    for (i = 1; i < 9; i++)
    {
        float _psi0 = f_out[0][uint3(pos+c[i], 9)];
        F0 += g00 * _psi0 * c[i] * w[i];
    }
    F0 *= -psi0;
    F0 += float2(0.f, 0.01f) * rho0;
    f_out[0][uint3(pos, 10)] = F0.x;
    f_out[0][uint3(pos, 11)] = F0.y;
}
