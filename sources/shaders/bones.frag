#version 450

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

vec3 LightedColor(
  vec3 color,
  float shininess,
  float metallness,
  vec3 world_position,
  vec3 world_normal,
  vec3 light_dir,
  vec3 camera_pos)
{
  vec3 W = normalize(camera_pos - world_position);
  vec3 E = reflect(light_dir, world_normal);
  float df = max(0.0, dot(world_normal, -light_dir));
  float sf = max(0.0, dot(E, W));
  sf = pow(sf, shininess);
  return color * (AmbientLight + df * SunLight) + vec3(1,1,1) * sf * metallness;
}
void main()
{
  float shininess = 40;
  float metallness = 0;
  vec3 BaseColor = vec3(1.0, 0.72, 0.2);
  vec3 color = LightedColor(BaseColor, shininess, metallness,
    vsOut.WorldPosition, vsOut.EyespaceNormal, LightDirection, CameraPosition);
  out_color = vec4(color, 1.0);
}
