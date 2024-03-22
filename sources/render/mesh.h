#pragma once
#include "glad/glad.h"
#include <utils/span.h>
#include <assimp/scene.h>
#include <glm/matrix.hpp>
#include <memory>
#include <vector>
#include <string>
#include <cassert>

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

// @TODO(PKiyashko): this is a full-blown class now. Consider encapsulating and
//                   making the make_skeleton method a factory method
struct Skeleton
{
  struct Bone
  {
    Bone *parent;
    std::string name;
    size_t id;
  };

  std::vector<Bone> bones;
  Bone root;

  std::vector<glm::mat4> boneLocalTransforms;
  GLuint boneLocalTransformsSSBO{0};
  std::vector<glm::mat4> boneRootTransforms;
  GLuint boneRootTransformsSSBO{0};
  std::vector<glm::mat4> boneOffsets;
  GLuint boneOffsetsSSBO{0};

  Skeleton() = default;
  Skeleton(Span<aiBone *> a_bones, GLuint a_loct_ssbo, GLuint a_tf_ssbo, GLuint a_p_ssbo);

  size_t GetBoneId(const char *bone_name) const
  {
    for (const Bone &bone : bones)
    {
      if (strcmp(bone_name, bone.name.c_str()) == 0)
        return bone.id;
    }
    return (size_t)(-1);
  }

  void SetBoneTransform(size_t bone_id, glm::mat4 transform)
  {
    // @TODO(PKiyashko): more graceful logging on invalid bone_id (or wrong bone name)
    assert(bone_id < boneLocalTransforms.size());
    boneLocalTransforms[bone_id] = transform;
  }
  void SetBoneTransform(const char *bone_name, glm::mat4 transform) 
    { SetBoneTransform(GetBoneId(bone_name), transform); }

  void ApplyTransformToBone(size_t bone_id, glm::mat4 transform) 
  {
    // @TODO(PKiyashko): more graceful logging on invalid bone_id (or wrong bone name)
    assert(bone_id < boneLocalTransforms.size());
    boneLocalTransforms[bone_id] = transform * boneLocalTransforms[bone_id];
  }
  void ApplyTransformToBone(const char *bone_name, glm::mat4 transform) 
    { ApplyTransformToBone(GetBoneId(bone_name), transform); }

  void UpdateSkeletalData();
  void LoadGPUData() const;
  void UpdateGPUData() const;
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