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
    static const float omega = 1.02f;
    
    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f[i] = f_in[uint3(index, i)];
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
    
    outTex[index] = saturate(float4(0, saturate(rho / 3 * 2000), 0, 1) + inTex[index]);
    
    [unroll(9)]
    for (uint n = 0; n < 9; n++)
    {
        f_out[uint3(index, n)] = f[n] - omega * (f[n] - eq[n]);
    }
    
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 index = DTid.xy;
    collision(index);
}
