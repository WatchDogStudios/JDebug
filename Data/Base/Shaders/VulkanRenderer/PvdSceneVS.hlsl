cbuffer SceneViewUniform : register(b0)
{
  row_major float4x4 g_ViewProjection;
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

struct PushConstants
{
  row_major float4x4 Model;
  float4 Color;
};

[[vk::push_constant]] PushConstants g_Push;

VSOutput mainVS(VSInput input)
{
  VSOutput output;

  float4 worldPos = mul(float4(input.Position, 1.0f), g_Push.Model);
  output.WorldPosition = worldPos.xyz;
  output.Position = mul(worldPos, g_ViewProjection);
  output.Color = g_Push.Color;

  return output;
}
