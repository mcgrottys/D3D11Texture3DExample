struct PixelShaderInput
{
    float4 outPosition : SV_POSITION;
    float3 outTexCoord : TEXCOORD0;
};

[maxvertexcount(3)]
void main(triangle PixelShaderInput input[3], inout TriangleStream<PixelShaderInput> TriStream)
{
    for (int i = 0; i < 3; ++i)
    {
        TriStream.Append(input[i]);
    }
}
