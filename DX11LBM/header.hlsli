Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 c[9] = { 
        { 1, 1 }, { 1, 0 }, { 1, -1 }, 
        { 0, 1 }, { 0, 0 }, { 0, -1 }, 
        { -1, 1 }, { -1, 0 }, { -1, -1 } };
static const float w[9] =
{
    1.0f / 36, 1.0f / 9, 1.0f / 36,
    1.0f / 9, 4.0f / 9, 1.0f / 9,
    1.0f / 36, 1.0f / 9, 1.0f / 36
};
static const float tau = .501f;