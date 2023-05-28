#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

static const float k[3] = { 1.6f, 1.2f, 1.8f };
#define SpeedDumping(vel, k) (vel * exp(-k * length(vel)))

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
            ) /** 0.999f*/;
            float2 F_ext = float2(
                f_out[j][uint3(pos, 10)],
                f_out[j][uint3(pos, 11)]
            );
        
            [unroll]
            for (i = 0; i < 9; i++)
            {
                f[i] = f_in[j][uint3(pos, i)];
            }

            F_ext += float2(0.f, 0.002f) * rho;
        
            float2 a = F_ext / rho;
        
            //float2 v = u + a * 0.5f;
            float2 v = u + a;

            v = SpeedDumping(v, 1e-5);
            float max_speed = .57;
        
            if (length(v) > max_speed)
            {
                v = normalize(v) * max_speed;
            }
            if (length(u) > max_speed)
            {
                u = normalize(u) * max_speed;
            }
            
            //a = (v - u) * 2.f;
            //F_ext = a * rho;
        
            float f_eq[9];
            float u_sqr = 1.5f * dot(u, u);
            [unroll]
            for (i = 0; i < 9; i++)
            {
                float cu = 3.f * dot(c[i], u);
                float cv = 3.f * dot(c[i], v);
            
                f_eq[i] = rho * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
            }

            float s = 0.f;
            float2x2 S;
            [unroll(2)]
            for (uint _a = 0; _a < 2; _a++)
            {
                [unroll(2)]
                for (uint _b = 0; _b < 2; _b++)
                {
                    s = 0.f;
                    [unroll(9)]
                    for (i = 0; i < 9; i++)
                    {
                        s += c[i][_a] * c[i][_b] * (f[i] - f_eq[i]);
                    }
                    S[_a][_b] = s;
                }
            }
    
            s = S[0][0] * S[0][0] + 2 * S[0][1] * S[1][0] + S[1][1] * S[1][1];
            
            float tau2 = 1. / k[j] + 0.5f * (-1. / k[j] + sqrt(pow(1. / k[j], 2) + 1.62f * sqrt(s)));
            float k2 = 1.f / tau2;

            float v_sqr = 1.5f * dot(v, v);
            //const float l = 1.f - k[j];
            const float l = 1.f - k2;
            [unroll]
            for (i = 0; i < 9; i++)
            {
                float cu = 3.f * dot(c[i], u);
                float cv = 3.f * dot(c[i], v);
            
                //float f_eq = rho * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
                float f_eq2 = rho * w[i] * (1.0f + cv - v_sqr + 0.5f * cv * cv);
                f[i] = l * (f[i] - f_eq[i]) + f_eq2;
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
