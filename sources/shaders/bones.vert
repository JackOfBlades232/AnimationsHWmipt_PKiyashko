#version 450

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
};

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;

out VsOutput vsOut;

uniform mat4 RootTransform;
uniform mat4 ViewProjection;

layout(binding = 0) readonly buffer BoneTransforms
{
  mat4 bones[];
};

void main() 
{
  mat4 Transform = RootTransform * bones[gl_InstanceID];
  vsOut.WorldPosition = (Transform * vec4(Position, 1.0)).xyz;
  vsOut.EyespaceNormal = normalize((Transform * vec4(Normal, 0)).xyz);
  gl_Position = ViewProjection * vec4(vsOut.WorldPosition, 1.0);
}
