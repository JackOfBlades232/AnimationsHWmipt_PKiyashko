#version 450

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
};

uniform mat4 Transform;
uniform mat4 ViewProjection;

layout(binding = 0) readonly buffer BoneOffsets
{
  mat4 boneOffsets[];
};


layout(location = 0)  in vec3 BonespacePositions0;
layout(location = 1)  in vec3 BonespacePositions1;
layout(location = 2)  in vec3 BonespacePositions2;
layout(location = 3)  in vec3 BonespacePositions3;
layout(location = 4)  in vec3 BonespaceNormals0;
layout(location = 5)  in vec3 BonespaceNormals1;
layout(location = 6)  in vec3 BonespaceNormals2;
layout(location = 7)  in vec3 BonespaceNormals3;
layout(location = 8)  in vec2 UV;
layout(location = 9)  in vec4 BoneWeights;
layout(location = 10) in uvec4 BoneIndex;

out VsOutput vsOutput;

void main()
{
  vec4 meshspacePos  = BoneWeights.x * boneOffsets[BoneIndex.x] * vec4(BonespacePositions0, 1.0) +
                       BoneWeights.y * boneOffsets[BoneIndex.y] * vec4(BonespacePositions1, 1.0) +
                       BoneWeights.z * boneOffsets[BoneIndex.z] * vec4(BonespacePositions2, 1.0) +
                       BoneWeights.w * boneOffsets[BoneIndex.w] * vec4(BonespacePositions3, 1.0);

  vec3 meshspaceNorm = BoneWeights.x * normalize(transpose(inverse(mat3(boneOffsets[BoneIndex.x]))) * BonespaceNormals0.xyz) +
                       BoneWeights.y * normalize(transpose(inverse(mat3(boneOffsets[BoneIndex.y]))) * BonespaceNormals1.xyz) +
                       BoneWeights.z * normalize(transpose(inverse(mat3(boneOffsets[BoneIndex.z]))) * BonespaceNormals2.xyz) +
                       BoneWeights.w * normalize(transpose(inverse(mat3(boneOffsets[BoneIndex.w]))) * BonespaceNormals3.xyz);

  vec3 VertexPosition = (Transform * meshspacePos).xyz;
  vsOutput.EyespaceNormal = normalize(transpose(inverse(mat3(Transform))) * meshspaceNorm);

  gl_Position = ViewProjection * vec4(VertexPosition, 1);
  vsOutput.WorldPosition = VertexPosition;

  vsOutput.UV = UV;
}
