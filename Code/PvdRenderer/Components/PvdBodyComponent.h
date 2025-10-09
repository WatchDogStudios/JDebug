#pragma once

#include <PvdRenderer/PvdRendererDLL.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <PvdRenderer/Renderer/PvdRenderData.h>

using nsPvdBodyComponentManager = nsComponentManager<class nsPvdBodyComponent, nsBlockStorageType::FreeList>;

/// \brief Simple render component that publishes Jolt PVD body debug data to the render pipeline.
class NS_PVDRENDERER_DLL nsPvdBodyComponent : public nsRenderComponent
{
  NS_DECLARE_COMPONENT_TYPE(nsPvdBodyComponent, nsRenderComponent, nsPvdBodyComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // nsComponent

public:
  virtual void SerializeComponent(nsWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(nsWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // nsRenderComponent

public:
  virtual nsResult GetLocalBounds(nsBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, nsMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // nsPvdBodyComponent

public:
  nsPvdBodyComponent();
  ~nsPvdBodyComponent();

  void SetShape(nsEnum<nsJvdShapeType> shape);            // [ property ]
  nsEnum<nsJvdShapeType> GetShape() const { return m_Shape; }

  void SetDimensions(const nsVec3& vDimensions);          // [ property ]
  const nsVec3& GetDimensions() const { return m_vDimensions; }

  void SetColor(const nsColor& color);                    // [ property ]
  const nsColor& GetColor() const { return m_Color; }

  void SetBodyId(nsUInt64 uiBodyId);                      // [ property ]
  nsUInt64 GetBodyId() const { return m_uiBodyId; }

  void SetMass(float fMass);                              // [ property ]
  float GetMass() const { return m_fMass; }

  void SetSleeping(bool bSleeping);                       // [ property ]
  bool IsSleeping() const { return m_bSleeping; }

  void SetLinearVelocity(const nsVec3& vVelocity);        // [ property ]
  const nsVec3& GetLinearVelocity() const { return m_vLinearVelocity; }

  void SetAngularVelocity(const nsVec3& vVelocity);       // [ property ]
  const nsVec3& GetAngularVelocity() const { return m_vAngularVelocity; }

  /// Temporary helper to push state in bulk once we hook into JVDSDK.
  void SetPvdState(nsUInt64 uiBodyId, nsEnum<nsJvdShapeType> shape, const nsVec3& vDimensions, const nsVec3& vLinearVelocity,
    const nsVec3& vAngularVelocity, float fMass, bool bSleeping, const nsColor& color);

protected:
  void OnMsgExtractRenderData(nsMsgExtractRenderData& msg) const;
  nsBoundingBoxSphere ComputeLocalBounds() const;
  nsVec3 GetHalfExtents() const;
  nsTransform ComputeShapeTransform() const;
  void MarkRenderDataDirty();

  nsEnum<nsJvdShapeType> m_Shape = nsJvdShapeType::Box;
  nsVec3 m_vDimensions = nsVec3(0.5f, 0.5f, 0.5f);
  nsColor m_Color = nsColor::White;

  float m_fMass = 0.0f;
  nsVec3 m_vLinearVelocity = nsVec3::MakeZero();
  nsVec3 m_vAngularVelocity = nsVec3::MakeZero();
  nsUInt64 m_uiBodyId = 0;
  bool m_bSleeping = false;

  mutable nsBoundingBoxSphere m_CachedLocalBounds = nsBoundingBoxSphere::MakeInvalid();
  mutable bool m_bLocalBoundsDirty = true;
};
