#include "header.hlsli"

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const int2 pos = DTid.xy;

    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    uint i = 0;
    float rho = 1e-20f;
    float2 u = { 0.0f, 0.0f };
    
    
	[unroll(9)]
    for (i = 0; i < 9; i++)
    {
        f[i] = f_out[uint3(pos, i)];
    }
    
    
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        rho += f[i];
        u += c[i] * f[i];
    }

    u /= rho;

    u = clamp(u, -0.7f, 0.7f); //step(length(u),0.7f) * u;
    
    const float usqr = 1.5 * dot(u, u);
    float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        float cu = 3.f * dot(u, c[i]);
        eq[i] = rho * w[i] * (1.f + cu + 0.5f * cu * cu - usqr);
    }
    
    float s = 0.f;
    float2x2 S;
    [unroll(2)]
    for (uint aa = 0; aa < 2; aa++)
    {
        [unroll(2)]
        for (uint bb = 0; bb < 2; bb++)
        {
            s = 0.f;
            [unroll(9)]
            for (i = 0; i < 9; i++)
            {
                s += c[i][aa] * c[i][bb] * (f[i] - eq[i]);
            }
            S[aa][bb] = s;
        }
    }
    
    s = S[0][0] * S[0][0] + 2 * S[0][1] * S[1][0] + S[1][1] * S[1][1];
            
    float tau2 = tau + 0.5f * (-tau + sqrt(pow(tau, 2) + 1.62f * sqrt(s)));
    float k = 1.f / tau2;
    
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        //float f_p = (f[i] + f[8 - i]) * 0.5f;
        //float f_n = (f[i] - f[8 - i]) * 0.5f;
        //float f_eq_p = (eq[i] + eq[8 - i]) * 0.5f;
        //float f_eq_n = (eq[i] - eq[8 - i]) * 0.5f;
        //f_in[uint3(pos, i)] = f[i] - k * (f_p - f_eq_p) - 0.9f * (f_n - f_eq_n);
        f[i] = (1.f - k) * f[i] + k * eq[i]; //
    }
    
        
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        f_in[uint3(pos, i)] = f[i];
    }
}
