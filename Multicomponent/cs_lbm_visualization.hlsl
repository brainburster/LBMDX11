#include "cs_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    
    float rho0 = f_in[0][uint3(pos, 9)];
    float rho1 = f_in[1][uint3(pos, 9)];
    float rho2 = f_in[2][uint3(pos, 9)];

    float2 F =
    {
        f_out[0][uint3(pos, 10)] + f_out[1][uint3(pos, 10)],
        f_out[0][uint3(pos, 11)] + f_out[1][uint3(pos, 11)]
    };
    
    
    //float2 u =
    //{
    //    f_in[0][uint3(pos, 10)] + f_in[1][uint3(pos, 10)],
    //    f_in[0][uint3(pos, 11)] + f_in[1][uint3(pos, 11)]
    //};
    
    uav_display[pos] = float4(rho0 * 6.f, rho2 * 0.1f + length(F)/*+ clamp(length(u) * 0.3f,0,0.3f)*/, rho1 * 10.f, uav_display[pos].w);
}
