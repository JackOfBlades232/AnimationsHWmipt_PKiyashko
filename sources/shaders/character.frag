#version 450
#extension GL_ARB_shading_language_include : require

#include </lighting.frag.inc>

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
};

uniform vec3 CameraPosition;
uniform vec3 LightDirection;
uniform vec3 AmbientLight;
uniform vec3 SunLight;

in VsOutput vsOutput;
out vec4 FragColor;

uniform sampler2D mainTex;

void main()
{
  float shininess = 1.3;
  float metallness = 0.4;
  vec3 color = texture(mainTex, vsOutput.UV).rgb ;
  color = LightedColor(color, shininess, metallness, vsOutput.WorldPosition, 
    vsOutput.EyespaceNormal, LightDirection, CameraPosition, AmbientLight, SunLight);
  FragColor = vec4(color, 1.0);
}
