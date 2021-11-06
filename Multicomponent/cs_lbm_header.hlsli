// d2q9 velocity sets:
//
//    6    2     5
//      ¨I ¡ü ¨J    
//    3 ¡û  0 ¡ú 1
//      ¨L ¡ý ¨K
//    7    4     8

static const int2 c[9] = { { 0, 0 }, { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 } };
static const float w[9] =
{
    4.f / 9.f,
    1.f / 9.f, 1.f / 9.f, 1.f / 9.f, 1.f / 9.f,
    1.f / 36.f, 1.f / 36.f, 1.f / 36.f, 1.f / 36.f
};
static const uint oppo[9] =
{
    0, 3, 4, 1, 2, 7, 8, 5, 6
};

static const float k = 1.01f;
