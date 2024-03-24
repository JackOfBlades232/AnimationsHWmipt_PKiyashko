#include "mesh.h"
#include <3dmath.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <log.h>
#include "glad/glad.h"

#include <cstring>

#include <vector>
#include <array>


static glm::mat4 ai_to_glm_mat4(const aiMatrix4x4 &aiMatr)
{
  return glm::mat4(
    aiMatr.a1, aiMatr.b1, aiMatr.c1, aiMatr.d1,
    aiMatr.a2, aiMatr.b2, aiMatr.c2, aiMatr.d2,
    aiMatr.a3, aiMatr.b3, aiMatr.c3, aiMatr.d3,
    aiMatr.a4, aiMatr.b4, aiMatr.c4, aiMatr.d4);
}

// @TODO(PKiyashko): should all this logic be in the constructor? For small meshes, seems fine
Skeleton::Skeleton(Span<aiBone *> a_bones, GLuint a_t_ssbo, GLuint a_tf_ssbo, GLuint a_p_ssbo)
  : bones(a_bones.size()),
  boneMatrices(a_bones.size()), boneMatricesSSBO(a_t_ssbo),
  debugBoneTransforms(a_bones.size()), debugBoneTransformsSSBO(a_tf_ssbo),
  boneTransforms(a_bones.size()), boneTransformsSSBO(a_p_ssbo) 
{
  for (size_t i = 0; i < a_bones.size(); i++)
  {
    const aiBone *srcBone = a_bones[i];
    Bone &dstBone = bones[i];

    dstBone.localTransform = ai_to_glm_mat4(srcBone->mNode->mTransformation);
    dstBone.inverseBindPose = ai_to_glm_mat4(srcBone->mOffsetMatrix);
    dstBone.name = std::string(srcBone->mName.C_Str());
    dstBone.id = i;

    dstBone.parent = nullptr;
    if (srcBone->mNode->mParent)
    {
      for (size_t j = 0; j < a_bones.size(); j++)
      {
        // @NOTE(PKiyashko): mParent seems to point to some other aiNode, which is strange
        //                   so strings will have to do
        if (a_bones[j]->mName == srcBone->mNode->mParent->mName)
        {
          dstBone.parent = &bones[j];
          break;
        }
      }

      // @NOTE(PKiyashko): if a skeleton bone has no in-skeleton parent, we presume it
      //                   to be connected to the root
      if (!dstBone.parent)
      {
        const aiNode *aiRoot = srcBone->mNode->mParent;

        root.name = std::string(aiRoot->mName.C_Str());
        root.parent = nullptr;
        root.id = (size_t)(-1);

        dstBone.parent = &root;
      }
    }
  }
}

void Skeleton::UpdateSkeletalData()
{
  for (size_t i = 0; i < bones.size(); i++)
  {
    const Bone &bone = bones[i];

    // @NOTE(PKiyashko): here it is presumed that the root node has ID transform in mesh space
    glm::mat4 parentTransform = glm::identity<glm::mat4>();
    for (const Bone *bonep = bone.parent; bonep != &root; bonep = bonep->parent)
    {
      assert(bonep);
      parentTransform = bonep->localTransform * parentTransform;
    }

    boneTransforms[i] = parentTransform * bone.localTransform;
    boneMatrices[i] = boneTransforms[i] * bone.inverseBindPose;

    glm::vec3 tipOffset(bone.localTransform[3][0], bone.localTransform[3][1], bone.localTransform[3][2]);
    float scale = glm::length(tipOffset);

    // @NOTE(PKiyashko): the lookAtLH is OpenGL-specific (in fact, all of the code relies on assimp and opengl
    //                   both having rh coord systems). This is due to the fact that camera is considered
    //                   to be looking in the negative Z direction in rh, and our bone has (0, 0, 1) direction
    debugBoneTransforms[i] = parentTransform *
                            glm::inverse(glm::lookAtLH(glm::vec3(0.f), tipOffset, glm::vec3(0.f, 1.f, 0.f))) *
                            glm::scale(glm::vec3(scale));
  }
}

void Skeleton::LoadGPUData() const
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneMatricesSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(boneMatrices[0]) * boneMatrices.size(), 
    boneMatrices.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, debugBoneTransformsSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(debugBoneTransforms[0]) * debugBoneTransforms.size(), 
    debugBoneTransforms.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneTransformsSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(boneTransforms[0]) * boneTransforms.size(), 
    boneTransforms.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Skeleton::UpdateGPUData() const
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneMatricesSSBO);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(boneMatrices[0]) * boneMatrices.size(), boneMatrices.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, debugBoneTransformsSSBO);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(debugBoneTransforms[0]) * debugBoneTransforms.size(), debugBoneTransforms.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneTransformsSSBO);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(boneTransforms[0]) * boneTransforms.size(), boneTransforms.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


static void create_indices(const std::vector<unsigned int> &indices)
{
  GLuint arrayIndexBuffer;
  glGenBuffers(1, &arrayIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrayIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
  glBindVertexArray(0);
}

static void init_channel(int index, size_t data_size, const void *data_ptr, int component_count, bool is_float)
{
  GLuint arrayBuffer;
  glGenBuffers(1, &arrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  glBufferData(GL_ARRAY_BUFFER, data_size, data_ptr, GL_STATIC_DRAW);
  glEnableVertexAttribArray(index);

  if (is_float)
    glVertexAttribPointer(index, component_count, GL_FLOAT, GL_FALSE, 0, 0);
  else
    glVertexAttribIPointer(index, component_count, GL_UNSIGNED_INT, 0, 0);
}

template<int i>
static void init_channel() { }

template<int i, typename T, typename... Channel>
static void init_channel(const std::vector<T> &channel, const Channel&... channels)
{
  if (channel.size() > 0)
  {
    const int size = sizeof(T) / sizeof(channel[0][0]);
    init_channel(i, sizeof(T) * channel.size(), channel.data(), size, !(std::is_same<T, uvec4>::value));
  }
  init_channel<i + 1>(channels...);
}



template<typename... Channel>
static MeshPtr make_mesh(const std::vector<unsigned int> &indices, const Channel&... channels)
{
  GLuint vertexArrayBufferObject;
  glGenVertexArrays(1, &vertexArrayBufferObject);
  glBindVertexArray(vertexArrayBufferObject);
  init_channel<0>(channels...);
  create_indices(indices);

  return std::make_shared<Mesh>(Mesh{vertexArrayBufferObject, (int)indices.size()});
}


static MeshPtr make_mesh(const aiMesh *ai_mesh)
{
  std::vector<uint32_t> indices;
  std::vector<vec3> vertices;
  std::vector<vec3> normals;
  std::vector<vec2> uv;
  std::vector<vec4> weights;
  std::vector<uvec4> weightsIndex;

  int numVert = ai_mesh->mNumVertices;
  int numFaces = ai_mesh->mNumFaces;

  if (ai_mesh->HasFaces())
  {
    indices.resize(numFaces * 3);
    for (int i = 0; i < numFaces; i++)
    {
      assert(ai_mesh->mFaces[i].mNumIndices == 3);
      for (int j = 0; j < 3; j++)
        indices[i * 3 + j] = ai_mesh->mFaces[i].mIndices[j];
    }
  }

  if (ai_mesh->HasPositions())
  {
    vertices.resize(numVert);
    for (int i = 0; i < numVert; i++)
      vertices[i] = to_vec3(ai_mesh->mVertices[i]);
  }

  if (ai_mesh->HasNormals())
  {
    normals.resize(numVert);
    for (int i = 0; i < numVert; i++)
      normals[i] = to_vec3(ai_mesh->mNormals[i]);
  }

  if (ai_mesh->HasTextureCoords(0))
  {
    uv.resize(numVert);
    for (int i = 0; i < numVert; i++)
      uv[i] = to_vec2(ai_mesh->mTextureCoords[0][i]);
  }

  if (ai_mesh->HasBones())
  {
    weights.resize(numVert, vec4(0.f));
    weightsIndex.resize(numVert);
    int numBones = ai_mesh->mNumBones;
    std::vector<int> weightsOffset(numVert, 0);
    for (int i = 0; i < numBones; i++)
    {
      const aiBone *bone = ai_mesh->mBones[i];

      for (unsigned j = 0; j < bone->mNumWeights; j++)
      {
        int vertex = bone->mWeights[j].mVertexId;
        int offset = weightsOffset[vertex]++;
        weights[vertex][offset] = bone->mWeights[j].mWeight;
        weightsIndex[vertex][offset] = i;
      }
    }
    //the sum of weights not 1
    for (int i = 0; i < numVert; i++)
    {
      vec4 w = weights[i];
      float s = w.x + w.y + w.z + w.w;
      weights[i] *= 1.f / s;
    }
  }

  return make_mesh(indices, vertices, normals, uv, weights, weightsIndex);
}

static SkeletonPtr make_skeleton(const aiMesh *ai_mesh)
{
  if (!ai_mesh->HasBones())
  {
    debug_error("trying to load rigged mesh that has no bones");
    return nullptr;
  }
  Span<aiBone *> bonesSpan(ai_mesh->mBones, ai_mesh->mNumBones);
  GLuint localSSBO, offsetsSSBO, bonesSSBO;
  glGenBuffers(1, &localSSBO);
  glGenBuffers(1, &offsetsSSBO);
  glGenBuffers(1, &bonesSSBO);

  SkeletonPtr skeletonHandle = std::make_shared<Skeleton>(bonesSpan, localSSBO, offsetsSSBO, bonesSSBO);
  skeletonHandle->UpdateSkeletalData();
  skeletonHandle->LoadGPUData();
  return skeletonHandle;
}

static const aiScene *read_ai_scene(Assimp::Importer &imp, const char *path)
{
  imp.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  imp.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);
  imp.ReadFile(path, aiProcess_Triangulate | aiProcess_LimitBoneWeights |
    aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder |
    aiProcess_PopulateArmatureData);
  return imp.GetScene();
}

MeshPtr load_mesh(const char *path, int idx)
{
  Assimp::Importer importer;
  const aiScene* scene = read_ai_scene(importer, path);
  if (!scene)
  {
    debug_error("no asset in %s", path);
    return nullptr;
  }

  return make_mesh(scene->mMeshes[idx]);
}

RiggedMeshLoadRes load_rigged_mesh(const char *path, int idx)
{
  Assimp::Importer importer;
  const aiScene* scene = read_ai_scene(importer, path);
  if (!scene)
  {
    debug_error("no asset in %s", path);
    return RiggedMeshLoadRes{nullptr, nullptr};
  }

  MeshPtr meshHandle = make_mesh(scene->mMeshes[idx]);
  SkeletonPtr skeletonHandle = make_skeleton(scene->mMeshes[idx]);
  return RiggedMeshLoadRes{meshHandle, skeletonHandle};
}

void render(const Mesh &mesh, PrimitiveType pType)
{
  glBindVertexArray(mesh.vertexArrayBufferObject);
  glDrawElements(pType, mesh.numIndices, GL_UNSIGNED_INT, 0);
}

void render_instanced(const Mesh &mesh, uint32_t cnt, PrimitiveType pType)
{
  glBindVertexArray(mesh.vertexArrayBufferObject);
  glDrawElementsInstanced(pType, mesh.numIndices, GL_UNSIGNED_INT, 0, cnt);
}

MeshPtr make_mesh_from_data(std::vector<glm::vec4> &vert, std::vector<unsigned int> &ind, std::vector<glm::vec4> *norm)
{
  if (norm)
    return make_mesh(ind, vert, *norm);
  else
    return make_mesh(ind, vert);
}

MeshPtr make_plane_mesh()
{
  std::vector<uint32_t> indices = {0,1,2,0,2,3};
  std::vector<vec3> vertices = {vec3(-1,0,-1), vec3(1,0,-1), vec3(1,0,1), vec3(-1,0,1)};
  std::vector<vec3> normals(4, vec3(0,1,0));
  std::vector<vec2> uv = {vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,1)};
  return make_mesh(indices, vertices, normals, uv);
}
