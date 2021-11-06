Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
static const float w[9] =
{
    1.0f / 36, 1.0f / 9, 1.0f / 36,
    1.0f / 9, 4.0f / 9, 1.0f / 9,
    1.0f / 36, 1.0f / 9, 1.0f / 36
};

void collision(uint2 index)
{
    static const float omega =  1.76f;
    
    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f[i] = f_out[uint3(index, i)];
    }
    
    float rho = 0;
    [unroll(9)]
    for (uint j = 0; j < 9; j++)
    {
        rho += f[j];
    }
    
    float2 u = { 0.0f, 0.0f };
    [unroll(9)]
    for (uint k = 0; k < 9; k++)
    {
        u += v[k] * f[k];
    }

    u /= rho;
    const float g = 0.0001;
    u.y += g*0.5;

    if (index.x == 0)
    {
        u = float2(0.07f, 0);
        rho = 1 / (1 - u.x) * (f[3] + f[4] + f[5] + (f[6] + f[7] + f[8]) * 2);
    }
    
    
    float vu[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    [unroll(9)]
    for (uint l = 0; l < 9; l++)
    {
        vu[l] = 3 * (v[l].x * u.x + v[l].y * u.y);
    }
    
    const float usqr = 1.5 * (u.x * u.x + u.y * u.y);
    
    float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    [unroll(9)]
    for (uint m = 0; m < 9; m++)
    {
        eq[m] = rho * w[m] * (1 + vu[m] + 0.5 * vu[m] * vu[m] - usqr);
    }
    
    if (index.x == 0)
    {
       [unroll(3)]
        for (uint i = 0; i < 3; i++)
        {
            f[i] = eq[i] + f[8 - i] - eq[8 - i];
        }
    }
    float Si = 0;
    [unroll(9)]
    for (uint n = 0; n < 9; n++)
    {
        Si = (1 - 0.5*omega) * w[n] * dot(((v[n] - u) / 3 + dot(v[n],u)*v[n]/9), float2(0.0, g / rho));
        f_in[uint3(index, n)] = saturate((1 - omega) * f[n] + omega * eq[n])+ Si;
    }
}

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 index = DTid.xy;
    collision(index);
}
