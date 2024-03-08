#version 450

struct VsOutput
{
  vec3 Color;
};

layout(location = 0) in vec3 Position;

out VsOutput vsOut;

uniform mat4 RootTransform;
uniform mat4 ViewProjection;

layout(binding = 0) readonly buffer BoneOffsets
{
  mat4 boneOffsets[];
};

void main() 
{
  if (gl_VertexID < 2)
    vsOut.Color = vec3(1.0, 0.0, 0.0);
  else if (gl_VertexID < 4)
    vsOut.Color = vec3(0.0, 1.0, 0.0);
  else
    vsOut.Color = vec3(0.0, 0.0, 1.0);

  mat4 Transform = RootTransform * boneOffsets[gl_InstanceID];
  gl_Position = ViewProjection * vec4((Transform * vec4(Position, 1.0)).xyz, 1.0);
}
