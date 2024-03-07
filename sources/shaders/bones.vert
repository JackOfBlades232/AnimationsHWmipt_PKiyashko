#version 450

layout(location = 0) in vec4 vPos;

uniform mat4 RootTransform;
uniform mat4 ViewProjection;

layout(binding = 0) readonly buffer BoneTransforms
{
  mat4 bones[];
};

void main() 
{
  gl_Position = ViewProjection * vec4((RootTransform * bones[gl_InstanceID] * vPos).xyz, 1.0);
}
