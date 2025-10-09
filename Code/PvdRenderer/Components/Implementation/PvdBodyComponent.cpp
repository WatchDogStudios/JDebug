#include <PvdRenderer/PvdRendererPCH.h>

#include <PvdRenderer/Components/PvdBodyComponent.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Types/Variant.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
NS_BEGIN_COMPONENT_TYPE(nsPvdBodyComponent, 1, nsComponentMode::Static)
{
  NS_BEGIN_PROPERTIES
  {
  NS_ENUM_ACCESSOR_PROPERTY("Shape", nsJvdShapeType, GetShape, SetShape),
    NS_ACCESSOR_PROPERTY("Dimensions", GetDimensions, SetDimensions)->AddAttributes(new nsDefaultValueAttribute(nsVec3(1.0f))),
    NS_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new nsExposeColorAlphaAttribute()),
    NS_ACCESSOR_PROPERTY("BodyId", GetBodyId, SetBodyId),
    NS_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new nsClampValueAttribute(0.0f, nsVariant())),
    NS_ACCESSOR_PROPERTY("Sleeping", IsSleeping, SetSleeping),
    NS_ACCESSOR_PROPERTY("LinearVelocity", GetLinearVelocity, SetLinearVelocity),
    NS_ACCESSOR_PROPERTY("AngularVelocity", GetAngularVelocity, SetAngularVelocity),
  }
  NS_END_PROPERTIES;
  NS_BEGIN_ATTRIBUTES
  {
    new nsCategoryAttribute("PVD"),
  }
  NS_END_ATTRIBUTES;
  NS_BEGIN_MESSAGEHANDLERS
  {
    NS_MESSAGE_HANDLER(nsMsgExtractRenderData, OnMsgExtractRenderData),
  }
  NS_END_MESSAGEHANDLERS;
}
NS_END_COMPONENT_TYPE;
// clang-format on

nsPvdBodyComponent::nsPvdBodyComponent() = default;
nsPvdBodyComponent::~nsPvdBodyComponent() = default;

void nsPvdBodyComponent::SerializeComponent(nsWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& stream = inout_stream.GetStream();
  stream << m_Shape;
  stream << m_vDimensions;
  stream << m_Color;
  stream << m_uiBodyId;
  stream << m_fMass;
  stream << m_bSleeping;
  stream << m_vLinearVelocity;
  stream << m_vAngularVelocity;
}

void nsPvdBodyComponent::DeserializeComponent(nsWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& stream = inout_stream.GetStream();
  stream >> m_Shape;
  stream >> m_vDimensions;
  stream >> m_Color;
  stream >> m_uiBodyId;
  stream >> m_fMass;
  stream >> m_bSleeping;
  stream >> m_vLinearVelocity;
  stream >> m_vAngularVelocity;

  m_bLocalBoundsDirty = true;
  TriggerLocalBoundsUpdate();
  MarkRenderDataDirty();
}

nsResult nsPvdBodyComponent::GetLocalBounds(nsBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, nsMsgUpdateLocalBounds& ref_msg)
{
  m_CachedLocalBounds = ComputeLocalBounds();
  if (!m_CachedLocalBounds.IsValid())
    return NS_FAILURE;

  ref_bounds = m_CachedLocalBounds;
  ref_bAlwaysVisible = false;
  m_bLocalBoundsDirty = false;
  return NS_SUCCESS;
}

void nsPvdBodyComponent::SetShape(nsEnum<nsJvdShapeType> shape)
{
  if (m_Shape == shape)
    return;

  m_Shape = shape;
  m_bLocalBoundsDirty = true;
  MarkRenderDataDirty();
  TriggerLocalBoundsUpdate();
}

void nsPvdBodyComponent::SetDimensions(const nsVec3& vDimensions)
{
  if (m_vDimensions.IsEqual(vDimensions, 0.0001f))
    return;

  m_vDimensions = vDimensions;
  m_bLocalBoundsDirty = true;
  TriggerLocalBoundsUpdate();
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetColor(const nsColor& color)
{
  if (m_Color.IsEqualRGBA(color))
    return;

  m_Color = color;
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetBodyId(nsUInt64 uiBodyId)
{
  if (m_uiBodyId == uiBodyId)
    return;

  m_uiBodyId = uiBodyId;
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetMass(float fMass)
{
  if (nsMath::IsEqual(m_fMass, fMass, 0.0001f))
    return;

  m_fMass = fMass;
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetSleeping(bool bSleeping)
{
  if (m_bSleeping == bSleeping)
    return;

  m_bSleeping = bSleeping;
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetLinearVelocity(const nsVec3& vVelocity)
{
  if (m_vLinearVelocity.IsEqual(vVelocity, 0.0001f))
    return;

  m_vLinearVelocity = vVelocity;
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetAngularVelocity(const nsVec3& vVelocity)
{
  if (m_vAngularVelocity.IsEqual(vVelocity, 0.0001f))
    return;

  m_vAngularVelocity = vVelocity;
  MarkRenderDataDirty();
}

void nsPvdBodyComponent::SetPvdState(nsUInt64 uiBodyId, nsEnum<nsJvdShapeType> shape, const nsVec3& vDimensions, const nsVec3& vLinearVelocity,
  const nsVec3& vAngularVelocity, float fMass, bool bSleeping, const nsColor& color)
{
  SetBodyId(uiBodyId);
  SetShape(shape);
  SetDimensions(vDimensions);
  SetLinearVelocity(vLinearVelocity);
  SetAngularVelocity(vAngularVelocity);
  SetMass(fMass);
  SetSleeping(bSleeping);
  SetColor(color);
}

void nsPvdBodyComponent::OnMsgExtractRenderData(nsMsgExtractRenderData& msg) const
{
  if (m_Shape == nsJvdShapeType::Unknown)
    return;

  if (msg.m_pView && msg.m_pView->GetCameraUsageHint() == nsCameraUsageHint::Shadow)
    return;

  if (m_bLocalBoundsDirty)
  {
    m_CachedLocalBounds = ComputeLocalBounds();
    m_bLocalBoundsDirty = false;
  }

  nsPvdBodyRenderData* pRenderData = nsCreateRenderDataForThisFrame<nsPvdBodyRenderData>(GetOwner());
  {
    nsTransform shapeTransform = ComputeShapeTransform();
    pRenderData->m_GlobalTransform = shapeTransform;

    nsBoundingBoxSphere localBounds = m_CachedLocalBounds;
    if (localBounds.IsValid())
    {
      nsBoundingBoxSphere worldBounds = localBounds;
      nsMat4 worldMatrix = shapeTransform.GetAsMat4();
      worldBounds.Transform(worldMatrix);
      pRenderData->m_GlobalBounds = worldBounds;
    }
    else
    {
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    }

    pRenderData->m_Color = m_Color;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
    pRenderData->m_uiBodyId = m_uiBodyId;
    pRenderData->m_vLinearVelocity = m_vLinearVelocity;
    pRenderData->m_vAngularVelocity = m_vAngularVelocity;
    pRenderData->m_fMass = m_fMass;
    pRenderData->m_bSleeping = m_bSleeping;
    pRenderData->SetShape(m_Shape, m_vDimensions);

    pRenderData->FillBatchIdAndSortingKey();
  }

  msg.AddRenderData(pRenderData, nsPvdRenderDataCategories::Body, nsRenderData::Caching::Never);
}

nsBoundingBoxSphere nsPvdBodyComponent::ComputeLocalBounds() const
{
  nsVec3 vHalfExtents = GetHalfExtents();
  if (vHalfExtents.IsZero())
    return nsBoundingBoxSphere::MakeInvalid();

  nsBoundingBox box = nsBoundingBox::MakeFromCenterAndHalfExtents(nsVec3::MakeZero(), vHalfExtents);
  return nsBoundingBoxSphere::MakeFromBox(box);
}

nsVec3 nsPvdBodyComponent::GetHalfExtents() const
{
  nsVec3 vDims = m_vDimensions;
  vDims.x = nsMath::Max(vDims.x, 0.001f);
  vDims.y = nsMath::Max(vDims.y, 0.001f);
  vDims.z = nsMath::Max(vDims.z, 0.001f);

  switch (m_Shape.GetValue())
  {
  case nsJvdShapeType::Sphere:
    {
      float fRadius = vDims.x * 0.5f;
      return nsVec3(fRadius);
    }
  case nsJvdShapeType::Capsule:
  case nsJvdShapeType::Cylinder:
    {
      float fRadius = vDims.x * 0.5f;
      float fHeight = nsMath::Max(vDims.z * 0.5f, 0.001f);
      return nsVec3(fRadius, fRadius, fHeight);
    }
    default:
      break;
  }

  return vDims * 0.5f;
}

nsTransform nsPvdBodyComponent::ComputeShapeTransform() const
{
  nsTransform t = GetOwner()->GetGlobalTransform();
  nsVec3 vScale = GetHalfExtents() * 2.0f;

  switch (m_Shape.GetValue())
  {
  case nsJvdShapeType::Sphere:
    {
      float fDiameter = nsMath::Max(vScale.x, 0.001f);
      t.m_vScale = t.m_vScale.CompMul(nsVec3(fDiameter));
      break;
    }

  case nsJvdShapeType::Capsule:
  case nsJvdShapeType::Cylinder:
    {
      vScale.x = nsMath::Max(vScale.x, 0.001f);
      vScale.y = nsMath::Max(vScale.y, 0.001f);
      vScale.z = nsMath::Max(vScale.z, 0.001f);
      t.m_vScale = t.m_vScale.CompMul(vScale);
      break;
    }

    default:
      vScale.x = nsMath::Max(vScale.x, 0.001f);
      vScale.y = nsMath::Max(vScale.y, 0.001f);
      vScale.z = nsMath::Max(vScale.z, 0.001f);
      t.m_vScale = t.m_vScale.CompMul(vScale);
      break;
  }

  return t;
}

void nsPvdBodyComponent::MarkRenderDataDirty()
{
  InvalidateCachedRenderData();
}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_Components_Implementation_PvdBodyComponent);
