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
  Span<aiBone *> bones;
  std::vector<glm::mat4> boneTransforms;
  uint32_t boneTransformsBufferObject;

  Skeleton() = default;
  Skeleton(Span<aiBone *> a_bones, uint32_t a_ssbo)
    : bones(a_bones), boneTransforms(bones.size()), boneTransformsBufferObject(a_ssbo) {}

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
RiggedMeshPtr load_rigged_mesh(const char *path, int idx);
MeshPtr make_mesh_from_data(std::vector<glm::vec4> &vert, std::vector<unsigned int> &ind);
MeshPtr make_plane_mesh();

void render(const Mesh &mesh);
void render_instanced(const Mesh &mesh, uint32_t cnt);