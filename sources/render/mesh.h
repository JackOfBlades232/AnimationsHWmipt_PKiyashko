#pragma once
#include <utils/span.h>
#include <assimp/scene.h>
#include <map>
#include <memory>

struct Mesh
{
  const uint32_t vertexArrayBufferObject;
  const int numIndices;

  Mesh(uint32_t vertexArrayBufferObject, int numIndices) :
    vertexArrayBufferObject(vertexArrayBufferObject),
    numIndices(numIndices)
    {}
};

using SkeletonData = Span<aiBone *>;

using MeshPtr = std::shared_ptr<Mesh>;
using SkeletonDataPtr = std::shared_ptr<SkeletonData>;

struct LoadMeshResult
{
  MeshPtr mesh;
  SkeletonDataPtr skeleton;
};

LoadMeshResult load_mesh(const char *path, int idx);
MeshPtr make_plane_mesh();

void render(const MeshPtr &mesh);