struct PixelShaderInput
{
    float4 outPosition : SV_POSITION;
    float3 outTexCoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

[maxvertexcount(12)]
void main(triangle PixelShaderInput input[3], inout TriangleStream<PixelShaderInput> TriStream)
{
    for (int i = 0; i < 3; ++i)
    {
        TriStream.Append(input[i]);
    }
    for (int j = 0; j < 3; ++j)
    {
        TriStream.Append(input[j]);
    }
}
