#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

static const float k[3] = { 1.2f, 1.2f, 1.2f };

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const uint2 pos = DTid.xy;
    
    uint i = 0;
    uint j = 0;
    
    [unroll]
    for (j = 0; j < 3; j++)
    {
        float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        float rho = f_in[j][uint3(pos, 9)];
        if (rho != 0)
        {
            float2 u = float2(
                f_in[j][uint3(pos, 10)],
                f_in[j][uint3(pos, 11)]
            ) * 0.99f;
            float2 F_ext = float2(
                f_out[j][uint3(pos, 10)],
                f_out[j][uint3(pos, 11)]
            );
        
            [unroll]
            for (i = 0; i < 9; i++)
            {
                f[i] = f_in[j][uint3(pos, i)];
            }

            F_ext += float2(0.f, 0.01f) * rho;
        
            float2 a = F_ext / rho;
        
            //float2 v = u + a * 0.5f;
            float2 v = u + a;
        
            float max_speed = .57735027f;
        
            if (length(v) > max_speed)
            {
                v = normalize(v) * max_speed;
            }
            if (length(u) > max_speed)
            {
                u = normalize(u) * max_speed;
            }
            
            a = (v - u) * 2.f;
            F_ext = a * rho;
        
            float u_sqr = 1.5f * dot(u, u);
            float v_sqr = 1.5f * dot(v, v);
            const float l = 1.f - k[j];
            [unroll]
            for (i = 0; i < 9; i++)
            {
                float cu = 3.f * dot(c[i], u);
                float cv = 3.f * dot(c[i], v);
            
                float f_eq = rho * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
                float f_eq2 = rho * w[i] * (1.0f + cv - v_sqr + 0.5f * cv * cv);
                f[i] = l * (f[i] - f_eq) + f_eq2;
                //f[i] = (1.f - k[j]) * f[i] + k[j] * f_eq2;
                //f[i] = f_eq2;
            }

            //[unroll]
            //for (i = 0; i < 9; i++)
            //{
            //    float cu = 3.f * dot(c[i], u);
            //    float cv = 3.f * dot(c[i], v);
            //    //float f_eq = rho * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
            //    float f_eq2 = rho * w[i] * (1.0f + cv - v_sqr + 0.5f * cv * cv);
            //    float F = w[i] * dot(3 * (c[i] - v) + 9 * dot(c[i], v) * c[i], F_ext);
            //    f[i] = (1.f - k[j]) * f[i] + k[j] * f_eq2 + (1 - 0.5f * k[j]) * F;
            //}
        }
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f_out[j][uint3(pos, i)] = f[i];
        }
    }
}