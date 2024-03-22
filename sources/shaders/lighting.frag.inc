vec3 LightedColor(
  vec3 color,
  float shininess,
  float metallness,
  vec3 world_position,
  vec3 world_normal,
  vec3 light_dir,
  vec3 camera_pos,
  vec3 ambient_light,
  vec3 sun_light)
{
  vec3 W = normalize(camera_pos - world_position);
  vec3 E = reflect(light_dir, world_normal);
  float df = max(0.0, dot(world_normal, -light_dir));
  float sf = max(0.0, dot(E, W));
  sf = pow(sf, shininess);
  return color * (ambient_light + df * sun_light) + vec3(1,1,1) * sf * metallness;
}
