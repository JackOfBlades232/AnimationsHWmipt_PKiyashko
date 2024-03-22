#pragma once
#include "glad/glad.h"
#include <utils/span.h>
#include <assimp/scene.h>
#include <glm/matrix.hpp>
#include <memory>
#include <vector>
#include <string>

// This is not a complete list
enum PrimitiveType
{
  TRIANGLES = GL_TRIANGLES,
  LINES = GL_LINES
};

struct Mesh
{
  GLuint vertexArrayBufferObject;
  int numIndices;
};

struct Skeleton
{
  struct Bone
  {
    glm::mat4 localTransform;
    Bone *parent;
    std::string name;
  };

  std::vector<Bone> bones;
  Bone root;
  std::vector<glm::mat4> boneRootTransforms;
  GLuint boneRootTransformsBO{0};
  std::vector<glm::mat4> boneOffsets;
  GLuint boneOffsetsBO{0};

  Skeleton() = default;
  Skeleton(Span<aiBone *> a_bones, GLuint a_tf_ssbo, GLuint a_p_ssbo);

  void UpdateTransforms();
  void LoadGPUData();
  void UpdateGPUData();
};

using MeshPtr = std::shared_ptr<Mesh>;
using SkeletonPtr = std::shared_ptr<Skeleton>;

struct RiggedMeshLoadRes
{
  MeshPtr meshHandle;
  SkeletonPtr skeletonHandle;
};

MeshPtr load_mesh(const char *path, int idx);
// @HUH(PKiyashko): is there utility for a load_skeleton function?
RiggedMeshLoadRes load_rigged_mesh(const char *path, int idx);
MeshPtr make_plane_mesh();

// @NOTE(PKiyashko): this could be variadic, but currently isn't so as to leave the module structure
MeshPtr make_mesh_from_data(std::vector<glm::vec4> &vert, std::vector<unsigned int> &ind, std::vector<glm::vec4> *norm = nullptr);

void render(const Mesh &mesh, PrimitiveType pType = TRIANGLES);
void render_instanced(const Mesh &mesh, uint32_t cnt, PrimitiveType pType = TRIANGLES);