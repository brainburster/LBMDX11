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
static const float k = 1.6f;

void collision(uint2 pos)
{
    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    uint i = 0;
    float rho = 0;
    float2 u = { 0.0f, 0.0f };
    float vu[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    
	[unroll(9)]
    for (i = 0; i < 9; i++)
    {
        f[i] = f_out[uint3(pos, i)];
        rho += f[i];
        u += v[i] * f[i];
    }

    u /= rho;

    if (pos.x == 0)
    {
        u = float2(0.07f, 0);
        rho = 1 / (1 - u.x) * (f[3] + f[4] + f[5] + (f[6] + f[7] + f[8]) * 2);
    }
    
    if (length(u) > 0.57735)
    {
        u = normalize(u) * 0.57735;
    }
    
    const float usqr = 1.5 * dot(u, u);
    float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        vu[i] = 3 * dot(u, v[i]);
    }
   
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        eq[i] = rho * w[i] * (1 + vu[i] + 0.5 * vu[i] * vu[i] - usqr);
    }
    
    if (pos.x == 0)
    {
       [unroll(3)]
        for (i = 0; i < 3; i++)
        {
            f[i] = eq[i] + f[8 - i] - eq[8 - i];
        }
    }
    
    [unroll(9)]
    for (i = 0; i < 9; i++)
    {
        f_in[uint3(pos, i)] =  (1 - k) * f[i] + k * eq[i];
    }
}

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 pos = DTid.xy;
    collision(pos);
}
