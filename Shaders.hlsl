cbuffer OffsetBuffer : register(b0)
{
    float4 offset;
}

cbuffer ColorBuffer : register(b1)
{
    float4 color;
}



struct VS_INPUT
{
    float4 pos : POSITION;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = input.pos + offset;
    
    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    return color;
}