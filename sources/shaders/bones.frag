#version 450
#extension GL_ARB_shading_language_include : require

#include </lighting.frag.inc>

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
};

uniform vec3 CameraPosition;
uniform vec3 LightDirection;
uniform vec3 AmbientLight;
uniform vec3 SunLight;

in VsOutput vsOut;
out vec4 out_color;

void main()
{
  float shininess = 40;
  float metallness = 0;
  vec3 BaseColor = vec3(1.0, 0.72, 0.2);
  vec3 color = LightedColor(BaseColor, shininess, metallness, vsOut.WorldPosition, 
    vsOut.EyespaceNormal, LightDirection, CameraPosition, AmbientLight, SunLight);
  out_color = vec4(color, 1.0);
}
