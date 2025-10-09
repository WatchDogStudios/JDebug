#include <PvdRenderer/PvdRendererPCH.h>

#include <PvdRenderer/PvdRendererModule.h>

#include <PvdRenderer/Components/PvdBodyComponent.h>

NS_IMPLEMENT_WORLD_MODULE(nsPvdRendererModule);

nsPvdRendererModule::nsPvdRendererModule(nsWorld* pWorld)
  : nsWorldModule(pWorld)
{
}

nsPvdRendererModule::~nsPvdRendererModule() = default;

void nsPvdRendererModule::Initialize()
{
  nsPvdBodyComponent::ComponentManagerType::GetTypeId();
}

void nsPvdRendererModule::Deinitialize() {}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_PvdRendererModule);
