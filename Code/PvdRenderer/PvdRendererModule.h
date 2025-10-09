#pragma once

#include <Core/World/WorldModule.h>
#include <PvdRenderer/PvdRendererDLL.h>

class NS_PVDRENDERER_DLL nsPvdRendererModule : public nsWorldModule
{
  NS_DECLARE_WORLD_MODULE();
  NS_ADD_DYNAMIC_REFLECTION(nsPvdRendererModule, nsWorldModule);

public:
  nsPvdRendererModule(nsWorld* pWorld);
  ~nsPvdRendererModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
};
