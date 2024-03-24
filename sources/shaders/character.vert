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
  mat4 boneMatrices[];
};


layout(location = 0) in vec3  Position;
layout(location = 1) in vec3  Normal;
layout(location = 2) in vec2  UV;
layout(location = 3) in vec4  BoneWeights;
layout(location = 4) in uvec4 BoneIndex;

out VsOutput vsOutput;

void main()
{
  mat4 skinningMatrix = BoneWeights.x * boneMatrices[BoneIndex.x] +
                        BoneWeights.y * boneMatrices[BoneIndex.y] +
                        BoneWeights.z * boneMatrices[BoneIndex.z] +
                        BoneWeights.w * boneMatrices[BoneIndex.w];

  mat4 toWorldMatrix = Transform * skinningMatrix;

  vec3 VertexPosition = (toWorldMatrix * vec4(Position, 1.0)).xyz;
  vsOutput.EyespaceNormal = normalize((transpose(inverse(toWorldMatrix)) * vec4(Normal, 0.0)).xyz);

  gl_Position = ViewProjection * vec4(VertexPosition, 1);
  vsOutput.WorldPosition = VertexPosition;

  vsOutput.UV = UV;
}
