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

    float p = 0.f;
    [unroll(2)]
    for (uint a = 0; a < 2; a++)
    {
        [unroll(2)]
        for (uint b = 0; b < 2; b++)
        {
            [unroll(9)]
            for (i = 0; i < 9; i++)
            {
                p += pow(c[i][a] * c[i][b] * (f[i] - eq[i]), 2);
            }
        }
    }
    
    float tau2 = tau + 0.5f * (-tau + sqrt(pow(tau, 2) + 1.62f * sqrt(p)));
    
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        f_in[uint3(pos, i)] = (1.f - 1.f / tau2) * f[i] + 1.f / tau2 * eq[i];
    }
}
