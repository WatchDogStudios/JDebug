#include <CoreTest/CoreTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

NS_CREATE_SIMPLE_TEST(World, Camera)
{
  NS_TEST_BLOCK(nsTestBlock::Enabled, "LookAt")
  {
    nsCamera camera;

    camera.LookAt(nsVec3(0, 0, 0), nsVec3(1, 0, 0), nsVec3(0, 0, 1));
    NS_TEST_VEC3(camera.GetPosition(), nsVec3(0, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(1, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(0, 1, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(0, 0, 1), nsMath::DefaultEpsilon<float>());

    camera.LookAt(nsVec3(0, 0, 0), nsVec3(-1, 0, 0), nsVec3(0, 0, 1));
    NS_TEST_VEC3(camera.GetPosition(), nsVec3(0, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(-1, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(0, -1, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(0, 0, 1), nsMath::DefaultEpsilon<float>());

    camera.LookAt(nsVec3(0, 0, 0), nsVec3(0, 0, 1), nsVec3(0, 1, 0));
    NS_TEST_VEC3(camera.GetPosition(), nsVec3(0, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(0, 0, 1), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(1, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(0, 1, 0), nsMath::DefaultEpsilon<float>());

    camera.LookAt(nsVec3(0, 0, 0), nsVec3(0, 0, -1), nsVec3(0, 1, 0));
    NS_TEST_VEC3(camera.GetPosition(), nsVec3(0, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(0, 0, -1), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(-1, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(0, 1, 0), nsMath::DefaultEpsilon<float>());

    const nsMat4 mLookAt = nsGraphicsUtils::CreateLookAtViewMatrix(nsVec3(2, 3, 4), nsVec3(3, 3, 4), nsVec3(0, 0, 1), nsHandedness::LeftHanded);
    camera.SetViewMatrix(mLookAt);

    NS_TEST_VEC3(camera.GetPosition(), nsVec3(2, 3, 4), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(1, 0, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(0, 1, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(0, 0, 1), nsMath::DefaultEpsilon<float>());

    // look at with dir == up vector
    camera.LookAt(nsVec3(2, 3, 4), nsVec3(2, 3, 5), nsVec3(0, 0, 1));
    NS_TEST_VEC3(camera.GetPosition(), nsVec3(2, 3, 4), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(0, 0, 1), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(0, 1, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(-1, 0, 0), nsMath::DefaultEpsilon<float>());

    camera.LookAt(nsVec3(2, 3, 4), nsVec3(2, 3, 3), nsVec3(0, 0, 1));
    NS_TEST_VEC3(camera.GetPosition(), nsVec3(2, 3, 4), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirForwards(), nsVec3(0, 0, -1), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirRight(), nsVec3(0, 1, 0), nsMath::DefaultEpsilon<float>());
    NS_TEST_VEC3(camera.GetDirUp(), nsVec3(1, 0, 0), nsMath::DefaultEpsilon<float>());
  }
}
