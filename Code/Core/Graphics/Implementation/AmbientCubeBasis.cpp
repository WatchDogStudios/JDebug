#include <Core/CorePCH.h>

#include <Core/Graphics/AmbientCubeBasis.h>

nsVec3 nsAmbientCubeBasis::s_Dirs[NumDirs] = {nsVec3(1.0f, 0.0f, 0.0f), nsVec3(-1.0f, 0.0f, 0.0f), nsVec3(0.0f, 1.0f, 0.0f),
  nsVec3(0.0f, -1.0f, 0.0f), nsVec3(0.0f, 0.0f, 1.0f), nsVec3(0.0f, 0.0f, -1.0f)};
