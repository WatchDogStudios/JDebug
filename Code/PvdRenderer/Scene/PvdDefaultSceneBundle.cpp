#include <PvdRenderer/PvdRendererPCH.h>
#include <PvdRenderer/Scene/PvdDefaultSceneBundle.h>
#include <PvdRenderer/Components/PvdBodyComponent.h>
#include <PvdRenderer/PvdRendererModule.h>

void nsPvdDefaultSceneBundle::Register(nsWorld& world)
{
  world.GetOrCreateModule<nsPvdRendererModule>();
  world.GetOrCreateComponentManager<nsPvdBodyComponentManager>();
}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_Scene_PvdDefaultSceneBundle);
