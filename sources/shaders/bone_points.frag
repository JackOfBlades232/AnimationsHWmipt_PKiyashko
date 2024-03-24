#version 450

struct VsOutput
{
  vec3 Color;
};

in VsOutput vsOut;
out vec4 out_color;

void main()
{
  out_color = vec4(vsOut.Color, 1.0);
}
