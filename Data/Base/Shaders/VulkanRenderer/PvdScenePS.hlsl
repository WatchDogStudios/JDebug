struct PSInput
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

float4 mainPS(PSInput input) : SV_Target0
{
  return input.Color;
}
