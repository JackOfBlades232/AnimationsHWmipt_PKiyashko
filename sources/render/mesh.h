#pragma once
#include <utils/span.h>
#include <assimp/scene.h>
#include <glm/matrix.hpp>
#include <memory>
#include <vector>

struct Mesh
{
  uint32_t vertexArrayBufferObject;
  int numIndices;
};

struct Skeleton
{
  Span<aiBone *> skeleton;
  std::vector<glm::mat4> boneTransforms;
  uint32_t boneTransformsBufferObject;

  Skeleton() = default;
  Skeleton(Span<aiBone *> a_bones, uint32_t a_ssbo)
    : skeleton(a_bones), boneTransforms(skeleton.size()), boneTransformsBufferObject(a_ssbo) {}

  void UpdateTransforms();
  void UpdateGpuData();
};

struct RiggedMesh
{
  Mesh mesh;
  Skeleton skeleton;
};

using MeshPtr = std::shared_ptr<Mesh>;
using RiggedMeshPtr = std::shared_ptr<RiggedMesh>;

MeshPtr load_mesh(const char *path, int idx);
MeshPtr make_plane_mesh();
RiggedMeshPtr load_rigged_mesh(const char *path, int idx);

void render(const Mesh &mesh);