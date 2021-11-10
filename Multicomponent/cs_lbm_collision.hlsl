#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

static const float k[2] = { 1.01f, 1.001f };

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    uint i = 0;
    uint j = 0;
    float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    float rho[2] = { 0.f, 0.f };
    float2 F = { 0.f, 0.f };
    float2 u = { 0.f, 0.f };
    
    rho[0] = f_in[0][uint3(pos, 9)];
    rho[1] = f_in[1][uint3(pos, 9)];
    if ((rho[0] + rho[1]) < 0 || uav_display[pos].w < 0)
    {
        for (i = 0; i < 9; i++)
        {
            f_out[0][uint3(pos, i)] = 0.f;
            f_out[1][uint3(pos, i)] = 0.f;
        }
        return;
    }
    
    [unroll]
    for (j = 0; j < 2; j++)
    {
        u = float2(
            f_in[j][uint3(pos, 10)],
            f_in[j][uint3(pos, 11)]
        );
        F = float2(
            f_out[j][uint3(pos, 10)],
            f_out[j][uint3(pos, 11)]
        );

        [unroll]
        for (i = 0; i < 9; i++)
        {
            f[i] = f_in[j][uint3(pos, i)];
        }

        if (rho[j] > 0.0f)
        {
            u += 0.5f * F / rho[j];
        }
    
        float u_sqr = 1.5f * dot(u, u);
        [unroll]
            for (i = 0; i < 9; i++)
            {
                float cu = 3.0 * dot(c[i], u);
                float f_eq = rho[j] * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
                f[i] = (1.0f - k[j]) * f[i] + k[j] * f_eq;
                if (rho[j] > 0.0f)
                {
                    float Si = (1.0f - 0.5f * k[j]) * f_eq / rho[j] * (dot((c[i] - u) * 3.f, F));
                    //float Si = (1.0f - 0.5f * k[j]) * w[i] * dot((c[i] - u) * 3 + (dot(c[i], u) * c[i] * 9), F);
                    f[i] += Si;
                }
            }
        
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f_out[j][uint3(pos, i)] = f[i];
        }
    }
}