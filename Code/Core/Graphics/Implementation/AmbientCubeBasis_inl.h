#pragma once

template <typename T>
NS_ALWAYS_INLINE nsAmbientCube<T>::nsAmbientCube()
{
  nsMemoryUtils::ZeroFillArray(m_Values);
}

template <typename T>
template <typename U>
NS_ALWAYS_INLINE nsAmbientCube<T>::nsAmbientCube(const nsAmbientCube<U>& other)
{
  *this = other;
}

template <typename T>
template <typename U>
NS_FORCE_INLINE void nsAmbientCube<T>::operator=(const nsAmbientCube<U>& other)
{
  for (nsUInt32 i = 0; i < nsAmbientCubeBasis::NumDirs; ++i)
  {
    m_Values[i] = other.m_Values[i];
  }
}

template <typename T>
NS_FORCE_INLINE bool nsAmbientCube<T>::operator==(const nsAmbientCube& other) const
{
  return nsMemoryUtils::IsEqual(m_Values, other.m_Values);
}

template <typename T>
NS_ALWAYS_INLINE bool nsAmbientCube<T>::operator!=(const nsAmbientCube& other) const
{
  return !(*this == other);
}

template <typename T>
void nsAmbientCube<T>::AddSample(const nsVec3& vDir, const T& value)
{
  m_Values[vDir.x > 0.0f ? 0 : 1] += nsMath::Abs(vDir.x) * value;
  m_Values[vDir.y > 0.0f ? 2 : 3] += nsMath::Abs(vDir.y) * value;
  m_Values[vDir.z > 0.0f ? 4 : 5] += nsMath::Abs(vDir.z) * value;
}

template <typename T>
T nsAmbientCube<T>::Evaluate(const nsVec3& vNormal) const
{
  nsVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] + vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
         vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}

template <typename T>
nsResult nsAmbientCube<T>::Serialize(nsStreamWriter& inout_stream) const
{
  return inout_stream.WriteArray(m_Values);
}

template <typename T>
nsResult nsAmbientCube<T>::Deserialize(nsStreamReader& inout_stream)
{
  return inout_stream.ReadArray(m_Values);
}
