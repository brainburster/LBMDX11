#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

static const float k[3] = { 1.6f, 0.5f, 1.9f };

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    
    uint i = 0;
    uint j = 0;
    
    [unroll]
    for (j = 0; j < 3; j++)
    {
        float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        float2 u = float2(
            f_in[j][uint3(pos, 10)],
            f_in[j][uint3(pos, 11)]
        );
        float2 a = float2(
            f_out[j][uint3(pos, 10)],
            f_out[j][uint3(pos, 11)]
        );
        
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f[i] = f_in[j][uint3(pos, i)];
        }
        
        float rho = f_in[j][uint3(pos, 9)];
        a /= rho + 1e-20f;

        float2 v = u + a * 0.5f;
        float max_speed = 0.6f;
        if (length(v) > max_speed)
        {
            v = normalize(v) * max_speed;
        }
        if (length(u) > max_speed)
        {
            u = normalize(u) * max_speed;
        }
        //v = clamp(v, -max_speed, max_speed);
        //u = clamp(u, -max_speed, max_speed);
        
        float u_sqr = 1.5f * dot(u, u);
        float v_sqr = 1.5f * dot(v, v);
        
        [unroll]
        for (i = 0; i < 9; i++)
        {
            float cu = 3.f * dot(c[i], u);
            float cv = 3.f * dot(c[i], v);
            
            float f_eq = rho * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
            float f_eq2 = rho * w[i] * (1.0f + cv - v_sqr + 0.5f * cv * cv);
            f[i] = (1.f - k[j]) * f[i] + (k[j] - 1.f) * f_eq + f_eq2;
        }

        //[unroll]
        //for (i = 0; i < 9; i++)
        //{
        //    float cu = 3.f * dot(c[i], u);
        //    float cv = 3.f * dot(c[i], v);
            
        //    //float f_eq = rho * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
        //    float f_eq2 = rho * w[i] * (1.0f + cv - v_sqr + 0.5f * cv * cv);
        //    float S = w[i] * rho * dot(3 * (c[i] - v) + 9 * dot(c[i], v) * c[i], a);
        //    f[i] = (1.f - k[j]) * f[i] + k[j] * f_eq2 + (1 - 0.5f * k[j]) * S;
        //}
        
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f_out[j][uint3(pos, i)] = f[i];
        }
    }
}