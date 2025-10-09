#include <PvdRenderer/PvdRendererPCH.h>
#include <PvdRenderer/Renderer/PvdRendererPipelines.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>

void nsPvdRendererPipelines::Register()
{
  // Preload the Vulkan-based shader used by the PVD renderer to ensure permutation warm-up.
  nsShaderResourceHandle hShader = nsResourceManager::LoadResource<nsShaderResource>("Shaders/Pvd/PvdBody.nsShader");
  NS_VERIFY(hShader.IsValid(), "Failed to preload PVD body shader resource.");
}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_Renderer_PvdRendererPipelines);
