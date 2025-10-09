#pragma once

#include <PvdRenderer/PvdRendererDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <JVDSDK/Recording/JvdShape.h>
#include <RendererCore/Meshes/MeshRenderData.h>

struct NS_PVDRENDERER_DLL nsPvdRenderDataCategories
{
  static nsRenderData::Category Body;
};

class NS_PVDRENDERER_DLL nsPvdBodyRenderData final : public nsMeshRenderData
{
  NS_ADD_DYNAMIC_REFLECTION(nsPvdBodyRenderData, nsMeshRenderData);

public:
  nsPvdBodyRenderData();

  void SetShape(nsEnum<nsJvdShapeType> shape, const nsVec3& dimensions);
  const nsVec3& GetShapeDimensions() const { return m_vShapeDimensions; }

  nsEnum<nsJvdShapeType> m_Shape = nsJvdShapeType::Unknown;
  nsVec3 m_vShapeDimensions = nsVec3::MakeZero();

  nsUInt64 m_uiBodyId = 0;
  nsVec3 m_vLinearVelocity = nsVec3::MakeZero();
  nsVec3 m_vAngularVelocity = nsVec3::MakeZero();
  float m_fMass = 0.0f;
  bool m_bSleeping = false;

  virtual void FillBatchIdAndSortingKey() override;
};
