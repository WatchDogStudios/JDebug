#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief Specifies in which mode this camera is configured.
struct NS_CORE_DLL nsCameraMode
{
  using StorageType = nsInt8;

  enum Enum
  {
    None,                 ///< Not initialized
    PerspectiveFixedFovX, ///< Perspective camera, the fov for X is fixed, Y depends on the aspect ratio
    PerspectiveFixedFovY, ///< Perspective camera, the fov for Y is fixed, X depends on the aspect ratio
    OrthoFixedWidth,      ///< Orthographic camera, the width is fixed, the height depends on the aspect ratio
    OrthoFixedHeight,     ///< Orthographic camera, the height is fixed, the width depends on the aspect ratio
    Stereo,               ///< A stereo camera with view/projection matrices provided by an HMD.
    Default = PerspectiveFixedFovY
  };
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsCameraMode);

/// \brief Determines left or right eye of a stereo camera.
///
/// As a general rule, this parameter does not matter for mono-scopic cameras and will always return the same value.
enum class nsCameraEye
{
  Left,
  Right,
  // Two eyes should be enough for everyone.
};

/// \brief A camera class that stores the orientation and camera settings for rendering.
///
/// The camera supports multiple modes including perspective and orthographic projection,
/// as well as stereoscopic rendering. Camera positions and orientations can be set directly
/// via view matrices or manipulated through movement and rotation functions.
///
/// The camera uses a configurable coordinate system for input and output coordinates.
/// By default, forward = +X, right = +Y, up = +Z.
///
/// For stereo cameras, separate view and projection matrices are maintained for each eye.
/// Modification counters track changes to camera settings and orientation for cache invalidation.
class NS_CORE_DLL nsCamera
{
public:
  nsCamera();

  /// \brief Allows to specify a different coordinate system in which the camera input and output coordinates are given.
  ///
  /// The default in z is forward = PositiveX, right = PositiveY, Up = PositiveZ.
  void SetCoordinateSystem(nsBasisAxis::Enum forwardAxis, nsBasisAxis::Enum rightAxis, nsBasisAxis::Enum axis);

  /// \brief Allows to specify a full nsCoordinateSystemProvider to determine forward/right/up vectors for camera movement
  void SetCoordinateSystem(const nsSharedPtr<nsCoordinateSystemProvider>& pProvider);

  /// \brief Returns the position of the camera that should be used for rendering etc.
  nsVec3 GetPosition(nsCameraEye eye = nsCameraEye::Left) const;

  /// \brief Returns the forwards vector that should be used for rendering etc.
  nsVec3 GetDirForwards(nsCameraEye eye = nsCameraEye::Left) const;

  /// \brief Returns the up vector that should be used for rendering etc.
  nsVec3 GetDirUp(nsCameraEye eye = nsCameraEye::Left) const;

  /// \brief Returns the right vector that should be used for rendering etc.
  nsVec3 GetDirRight(nsCameraEye eye = nsCameraEye::Left) const;

  /// \brief Returns the horizontal FOV.
  ///
  /// Works only with nsCameraMode::PerspectiveFixedFovX and nsCameraMode::PerspectiveFixedFovY
  nsAngle GetFovX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical FOV.
  ///
  /// Works only with nsCameraMode::PerspectiveFixedFovX and nsCameraMode::PerspectiveFixedFovY
  nsAngle GetFovY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the horizontal dimension for an orthographic view.
  ///
  /// Works only with nsCameraMode::OrthoFixedWidth and nsCameraMode::OrthoFixedWidth
  float GetDimensionX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical dimension for an orthographic view.
  ///
  /// Works only with nsCameraMode::OrthoFixedWidth and nsCameraMode::OrthoFixedWidth
  float GetDimensionY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the average camera position.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetPosition()
  nsVec3 GetCenterPosition() const;

  /// \brief Returns the average forwards vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirForwards()
  nsVec3 GetCenterDirForwards() const;

  /// \brief Returns the average up vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirUp()
  nsVec3 GetCenterDirUp() const;

  /// \brief Returns the average right vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirRight()
  nsVec3 GetCenterDirRight() const;

  /// \brief Returns the near plane distance that was passed to SetCameraProjectionAndMode().
  float GetNearPlane() const;

  /// \brief Returns the far plane distance that was passed to SetCameraProjectionAndMode().
  float GetFarPlane() const;

  /// \brief Specifies the mode and the projection settings that this camera uses.
  ///
  /// \param fFovOrDim
  ///   Fov X/Y in degree or width/height (depending on Mode).
  void SetCameraMode(nsCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane);

  /// Sets the camera mode to stereo and specifies projection matrices directly.
  ///
  /// \param fAspectRatio
  ///   These stereo projection matrices will only be returned by getProjectionMatrix for the given aspectRatio.
  void SetStereoProjection(const nsMat4& mProjectionLeftEye, const nsMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight);

  /// \brief Returns the fFovOrDim parameter that was passed to SetCameraProjectionAndMode().
  float GetFovOrDim() const;

  /// \brief Returns the current camera mode.
  nsCameraMode::Enum GetCameraMode() const;

  bool IsPerspective() const;

  bool IsOrthographic() const;

  /// \brief Whether this is a stereoscopic camera.
  bool IsStereoscopic() const;

  /// \brief Sets the view matrix directly.
  ///
  /// Works with all camera types. Position- and direction- getter/setter will work as usual.
  void SetViewMatrix(const nsMat4& mLookAtMatrix, nsCameraEye eye = nsCameraEye::Left);

  /// \brief Repositions the camera such that it looks at the given target position.
  ///
  /// Not supported for stereo cameras.
  void LookAt(const nsVec3& vCameraPos, const nsVec3& vTargetPos, const nsVec3& vUp);

  /// \brief Moves the camera in its local space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveLocally(float fForward, float fRight, float fUp);

  /// \brief Moves the camera in global space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveGlobally(float fForward, float fRight, float fUp);

  /// \brief Rotates the camera around the forward, right and up axis in its own local space.
  ///
  /// Rotate around \a rightAxis for looking up/down. \forwardAxis is roll. For turning left/right use RotateGlobally().
  /// Not supported for stereo cameras.
  void RotateLocally(nsAngle forwardAxis, nsAngle rightAxis, nsAngle axis);

  /// \brief Rotates the camera around the forward, right and up axis of the coordinate system in global space.
  ///
  /// Rotate around Z for turning the camera left/right.
  /// Not supported for stereo cameras.
  void RotateGlobally(nsAngle forwardAxis, nsAngle rightAxis, nsAngle axis);

  /// \brief Returns the view matrix for the given eye.
  ///
  /// \note The view matrix is given in OpenGL convention.
  const nsMat4& GetViewMatrix(nsCameraEye eye = nsCameraEye::Left) const;

  /// \brief Calculates the projection matrix from the current camera properties and stores it in out_projectionMatrix.
  ///
  /// If the camera is stereo and the given aspect ratio is close to the aspect ratio passed in SetStereoProjection,
  /// the matrix set in SetStereoProjection will be used.
  void GetProjectionMatrix(float fAspectRatioWidthDivHeight, nsMat4& out_mProjectionMatrix, nsCameraEye eye = nsCameraEye::Left,
    nsClipSpaceDepthRange::Enum depthRange = nsClipSpaceDepthRange::Default) const;

  float GetExposure() const;

  void SetExposure(float fExposure);

  /// \brief Returns a counter that is increased every time the camera settings are modified.
  ///
  /// The camera settings are used to compute the projection matrix. This counter can be used to determine whether the projection matrix
  /// has changed and thus whether cached values need to be updated.
  nsUInt32 GetSettingsModificationCounter() const { return m_uiSettingsModificationCounter; }

  /// \brief Returns a counter that is increased every time the camera orientation is modified.
  ///
  /// The camera orientation is used to compute the view matrix. This counter can be used to determine whether the view matrix
  /// has changed and thus whether cached values need to be updated.
  nsUInt32 GetOrientationModificationCounter() const { return m_uiOrientationModificationCounter; }

private:
  /// \brief This function is called whenever the camera position or rotation changed.
  void CameraOrientationChanged() { ++m_uiOrientationModificationCounter; }

  /// \brief This function is called when the camera mode or projection changes (e.g. SetCameraProjectionAndMode was called).
  void CameraSettingsChanged();

  /// \brief This function is called by RotateLocally() and RotateGlobally() BEFORE the values are applied,
  /// and allows to adjust them (e.g. for limiting how far the camera can rotate).
  void ClampRotationAngles(bool bLocalSpace, nsAngle& forwardAxis, nsAngle& rightAxis, nsAngle& upAxis);

  nsVec3 InternalGetPosition(nsCameraEye eye = nsCameraEye::Left) const;
  nsVec3 InternalGetDirForwards(nsCameraEye eye = nsCameraEye::Left) const;
  nsVec3 InternalGetDirUp(nsCameraEye eye = nsCameraEye::Left) const;
  nsVec3 InternalGetDirRight(nsCameraEye eye = nsCameraEye::Left) const;

  float m_fNearPlane = 0.1f;
  float m_fFarPlane = 1000.0f;

  nsCameraMode::Enum m_Mode = nsCameraMode::None;

  float m_fFovOrDim = 90.0f;

  float m_fExposure = 1.0f;

  nsVec3 m_vCameraPosition[2];
  nsMat4 m_mViewMatrix[2];

  /// If the camera mode is stereo and the aspect ratio given in getProjectio is close to this value, one of the stereo projection matrices
  /// is returned.
  float m_fAspectOfPrecomputedStereoProjection = -1.0;
  nsMat4 m_mStereoProjectionMatrix[2];

  nsUInt32 m_uiSettingsModificationCounter = 0;
  nsUInt32 m_uiOrientationModificationCounter = 0;

  nsSharedPtr<nsCoordinateSystemProvider> m_pCoordinateSystem;

  nsVec3 MapExternalToInternal(const nsVec3& v) const;
  nsVec3 MapInternalToExternal(const nsVec3& v) const;
};


#include <Core/Graphics/Implementation/Camera_inl.h>
