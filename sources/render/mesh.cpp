#include "mesh.h"
#include <vector>
#include <3dmath.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <log.h>
#include "glad/glad.h"

#include <cstring>


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
static void InitChannel() { }

template<int i, typename T, typename... Channel>
static void InitChannel(const std::vector<T> &channel, const Channel&... channels)
{
  if (channel.size() > 0)
  {
    const int size = sizeof(T) / sizeof(channel[0][0]);
    init_channel(i, sizeof(T) * channel.size(), channel.data(), size, !(std::is_same<T, uvec4>::value));
  }
  InitChannel<i + 1>(channels...);
}


void Skeleton::UpdateTransforms()
{
  for (size_t i = 0; i < boneTransforms.size(); i++)
  {
    const aiBone *bone = bones[i];

    // @TODO: use mOffsetMatrix of the bone?
    // This does rotation & pos
    aiMatrix4x4 transform = aiMatrix4x4(); // identity
    for (const aiNode *node = bone->mNode; node; node = node->mParent)
      transform = node->mTransformation * transform;

    if (strstr(bone->mName.C_Str(), "UpLeg"))
      transform *= aiMatrix4x4();

    float scale = 0.1f;
    if (bone->mNode->mNumChildren > 0) {
      aiVector3D childrenCenter = aiVector3D();
      for (size_t j = 0; j < bone->mNode->mNumChildren; j++) {
        aiMatrix4x4 &childTransform = bone->mNode->mChildren[j]->mTransformation;
        childrenCenter += aiVector3D(childTransform.a4, childTransform.b4, childTransform.c4);
      }
      childrenCenter /= bone->mNode->mNumChildren;
      scale = childrenCenter.Length();
    }

    aiMatrix4x4 scaleMat;
    transform *= aiMatrix4x4::Scaling(aiVector3D(scale), scaleMat);

    boneTransforms[i] = glm::mat4(
      transform.a1, transform.b1, transform.c1, transform.d1,
      transform.a2, transform.b2, transform.c2, transform.d2,
      transform.a3, transform.b3, transform.c3, transform.d3,
      transform.a4, transform.b4, transform.c4, transform.d4);
    /*
    boneTransforms[i] = glm::mat4(
      transform.a1, transform.a2, transform.a3, transform.a4,
      transform.b1, transform.b2, transform.b3, transform.b4,
      transform.c1, transform.c2, transform.c3, transform.c4,
      transform.d1, transform.d2, transform.d3, transform.d4);
      */
  }
}

void Skeleton::UpdateGpuData()
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneTransformsBufferObject);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(boneTransforms[0]) * boneTransforms.size(), boneTransforms.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


template<typename... Channel>
static void init_mesh(Mesh *out_mesh, const std::vector<unsigned int> &indices, const Channel&... channels)
{
  uint32_t vertexArrayBufferObject;
  glGenVertexArrays(1, &vertexArrayBufferObject);
  glBindVertexArray(vertexArrayBufferObject);
  InitChannel<0>(channels...);
  create_indices(indices);

  // @TODO: wtf why is this not working
  *out_mesh = Mesh{vertexArrayBufferObject, (int)indices.size()};
}


static void init_mesh(Mesh *out_mesh, const aiMesh *ai_mesh, bool use_bones = false)
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

  if (use_bones && ai_mesh->HasBones())
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

  init_mesh(out_mesh, indices, vertices, normals, uv, weights, weightsIndex);
}

static void init_skeleton(Skeleton *out_skeleton, const aiMesh *ai_mesh)
{
  assert(ai_mesh->HasBones()); // @TODO: more graceful?
  Span<aiBone *> bonesSpan(ai_mesh->mBones, ai_mesh->mNumBones);
  uint32_t bonesSsbo;

  glGenBuffers(1, &bonesSsbo);
  *out_skeleton = Skeleton(bonesSpan, bonesSsbo);
  out_skeleton->UpdateTransforms();

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesSsbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(out_skeleton->boneTransforms[0]) * out_skeleton->boneTransforms.size(), 
    out_skeleton->boneTransforms.data(), GL_DYNAMIC_DRAW /*@HUH? this is not thought out*/);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

#define AI_READ_SCENE(_importer, _path)                                              \
  _importer ## .SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);        \
  _importer ## .SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);            \
  _importer ## .ReadFile(_path, aiProcess_Triangulate | aiProcess_LimitBoneWeights | \
    aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder |      \
    aiProcess_CalcTangentSpace | aiProcess_PopulateArmatureData);

MeshPtr load_mesh(const char *path, int idx)
{
  Assimp::Importer importer;
  AI_READ_SCENE(importer, path);
  const aiScene* scene = importer.GetScene();
  if (!scene)
  {
    debug_error("no asset in %s", path);
    return nullptr;
  }

  MeshPtr meshHandle = std::make_shared<Mesh>();
  init_mesh(meshHandle.get(), scene->mMeshes[idx]);
  return meshHandle;
}

RiggedMeshPtr load_rigged_mesh(const char *path, int idx)
{
  // @TODO: this could be pulled out
  Assimp::Importer importer;
  AI_READ_SCENE(importer, path);
  const aiScene* scene = importer.GetScene();
  if (!scene)
  {
    debug_error("no asset in %s", path);
    return nullptr;
  }

  RiggedMeshPtr rmeshHandle = std::make_shared<RiggedMesh>();
  init_mesh(&rmeshHandle->mesh, scene->mMeshes[idx]);
  init_skeleton(&rmeshHandle->skeleton, scene->mMeshes[idx]);
  return rmeshHandle;
}

void render(const Mesh &mesh)
{
  glBindVertexArray(mesh.vertexArrayBufferObject);
  glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
}

void render_instanced(const Mesh &mesh, uint32_t cnt)
{
  glBindVertexArray(mesh.vertexArrayBufferObject);
  glDrawElementsInstanced(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0, cnt);
}

MeshPtr make_mesh_from_data(std::vector<glm::vec4> &vert, std::vector<unsigned int> &ind)
{
  MeshPtr outMesh = std::make_shared<Mesh>();
  init_mesh(outMesh.get(), ind, vert);
  return outMesh;
}

MeshPtr make_plane_mesh()
{
  std::vector<uint32_t> indices = {0,1,2,0,2,3};
  std::vector<vec3> vertices = {vec3(-1,0,-1), vec3(1,0,-1), vec3(1,0,1), vec3(-1,0,1)};
  std::vector<vec3> normals(4, vec3(0,1,0));
  std::vector<vec2> uv = {vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,1)};
  MeshPtr meshHandle = std::make_shared<Mesh>();
  init_mesh(meshHandle.get(), indices, vertices, normals, uv);
  return meshHandle;
}
