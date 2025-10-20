cbuffer SceneViewUniform : register(b0)
{
  row_major float4x4 g_ViewProjection;
};

cbuffer InstanceData : register(b1)
{
  row_major float4x4 g_Model;
  float4 g_Color;
};

struct VSInput
{
  float3 Position : POSITION;
};

struct VSOutput
{
  float4 Position : SV_Position;
  float3 WorldPosition : TEXCOORD0;
  float4 Color : COLOR0;
};

VSOutput mainVS(VSInput input)
{
  VSOutput output;

  float4 worldPos = mul(float4(input.Position, 1.0f), g_Model);
  output.WorldPosition = worldPos.xyz;
  output.Position = mul(worldPos, g_ViewProjection);
  output.Color = g_Color;

  return output;
}
