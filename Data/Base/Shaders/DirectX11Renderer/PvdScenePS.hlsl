struct PSInput
{
  float4 Position : SV_Position;
  float3 WorldPosition : TEXCOORD0;
  float4 Color : COLOR0;
};

float4 mainPS(PSInput input) : SV_Target0
{
  return input.Color;
}
