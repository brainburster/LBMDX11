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
    static const float omega = 1.6f;
    
    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f[i] = f_in[uint3(index, i)];
    }
    
    if (index.x==799)
    {
        f[0] = f_in[uint3(index + int2(-1, 0), 0)];
        f[1] = f_in[uint3(index + int2(-1, 0), 1)];
        f[2] = f_in[uint3(index + int2(-1, 0), 2)];
    }
    //else if (index.y == 0)
    //{
    //    f[2] = f_in[uint3(index + int2(0, 1), 2)];
    //    f[5] = f_in[uint3(index + int2(0, 1), 5)];
    //    f[8] = f_in[uint3(index + int2(0, 1), 8)];
    //}
    //else if (index.y == 599)
    //{
    //    f[0] = f_in[uint3(index + int2(0, -1), 0)];
    //    f[3] = f_in[uint3(index + int2(0, -1), 3)];
    //    f[6] = f_in[uint3(index + int2(0, -1), 6)];
    //}
    
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
    
    if (index.x == 0)
    {
        u = float2(0.25f, 0);
        rho = 1 / (1 - u.x) * (f[3] + f[4] + f[5] + (f[0] + f[1] + f[2]) * 2);
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
        for (uint i = 6; i < 9; i++)
        {
            f[i] = eq[i] + f[8 - i] - eq[8 - i];
            f_in[uint3(index, i)] = f[i];
        }
    }
    
    [unroll(9)]
    for (uint n = 0; n < 9; n++)
    {
        f_out[uint3(index, n)] = saturate(f[n] - omega * (f[n] - eq[n]));
    }
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 index = DTid.xy;
    collision(index);
}
