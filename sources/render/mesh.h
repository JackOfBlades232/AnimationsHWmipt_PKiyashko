#pragma once
#include <utils/span.h>
#include <assimp/scene.h>
#include <glm/matrix.hpp>
#include <memory>
#include <vector>
#include "glad/glad.h"

// This is not a complete list
enum PrimitiveType
{
  TRIANGLES = GL_TRIANGLES,
  LINES = GL_LINES
};

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
  std::vector<glm::mat4> boneOffsets;
  uint32_t boneOffsetsBufferObject;

  Skeleton() = default;
  Skeleton(Span<aiBone *> a_bones, uint32_t a_tf_ssbo, uint32_t a_p_ssbo)
    : bones(a_bones), boneTransforms(bones.size()), boneTransformsBufferObject(a_tf_ssbo),
    boneOffsets(bones.size()), boneOffsetsBufferObject(a_p_ssbo) {}

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
MeshPtr make_plane_mesh();

// @NOTE: this could be variadic, but currently isn't so as to leave the module structure
MeshPtr make_mesh_from_data(std::vector<glm::vec4> &vert, std::vector<unsigned int> &ind, std::vector<glm::vec4> *norm = nullptr);

void render(const Mesh &mesh, PrimitiveType pType = TRIANGLES);
void render_instanced(const Mesh &mesh, uint32_t cnt, PrimitiveType pType = TRIANGLES);