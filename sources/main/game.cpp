
#include <render/direction_light.h>
#include <render/material.h>
#include <render/mesh.h>
#include "camera.h"
#include <application.h>

enum RenderMode
{
  RMODE_REGULAR,
  RMODE_BONES
};

struct UserCamera
{
  glm::mat4 transform;
  mat4x4 projection;
  ArcballCamera arcballCamera;
};

struct Character
{
  glm::mat4 transform;
  MeshPtr mesh;
  SkeletonPtr skeleton;
  MaterialPtr material;
};

struct Scene
{
  DirectionLight light;

  UserCamera userCamera;

  RenderMode rmode;

  std::vector<Character> characters;

};

static std::unique_ptr<Scene> scene;

// @DEBUG: for all bones
// @BAD: this has no business being vectors, just to easily plug into mesh api
static std::vector<glm::vec4> bone_mesh_vert =
{
  {0.f,   0.f,    0.f,    1.f},
  {0.07f, 0.f,    0.07f,  1.f},
  {0.07f, 0.07f,  0.f,    1.f},
  {0.07f, 0.f,    -0.07f, 1.f},
  {0.07f, -0.07f, 0.f,    1.f},
  {1.f,   0.f,    0.f,    1.f},
};
static std::vector<glm::vec4> bone_mesh_norm =
{
  {-1.f, 0.f,  0.f,  1.f},
  {0.f,  0.f,  1.f,  1.f},
  {0.f,  1.f,  0.f,  1.f},
  {0.f,  0.f,  -1.f, 1.f},
  {0.f,  -1.f, 0.f,  1.f},
  {1.f,  0.f,  0.f,  1.f},
};
static std::vector<unsigned int> bone_mesh_ind =
{
  1, 2, 0,
  2, 3, 0,
  3, 4, 0,
  4, 1, 0,
  1, 5, 2,
  2, 5, 3,
  3, 5, 4,
  4, 5, 1
};

static std::vector<glm::vec4> axes_mesh_vert =
{
  {0.f, 0.f, 0.f, 1.f},
  {.1f, 0.f, 0.f, 1.f},
  {0.f, 0.f, 0.f, 1.f},
  {0.f, .1f, 0.f, 1.f},
  {0.f, 0.f, 0.f, 1.f},
  {0.f, 0.f, .1f, 1.f},
};
// @NOTE(PKiyashko) For ease of compatibility with mesh functions
static std::vector<unsigned int> axes_mesh_ind =
{
  0, 1,
  2, 3,
  4, 5
};

static MeshPtr bone_mesh;
static MaterialPtr bones_material;
static MeshPtr axes_mesh;
static MaterialPtr axes_material;


void game_init()
{
  scene = std::make_unique<Scene>();
  scene->light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene->light.lightColor = glm::vec3(1.f);
  scene->light.ambient = glm::vec3(0.2f);

  scene->userCamera.projection = glm::perspective(90.f * DegToRad, get_aspect_ratio(), 0.01f, 500.f);

  scene->rmode = RMODE_REGULAR;

  ArcballCamera &cam = scene->userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene->userCamera.transform = calculate_transform(scene->userCamera.arcballCamera);

  input.onMouseButtonEvent += [](const SDL_MouseButtonEvent &e) { arccam_mouse_click_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseMotionEvent += [](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseWheelEvent += [](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene->userCamera.arcballCamera); };
  input.onKeyboardEvent += [](const SDL_KeyboardEvent &e) {
    if (e.keysym.sym == SDLK_q && e.state == SDL_RELEASED && e.repeat == 0)
      scene->rmode = scene->rmode == RMODE_REGULAR ? RMODE_BONES : RMODE_REGULAR;
    };

  bone_mesh = make_mesh_from_data(bone_mesh_vert, bone_mesh_ind, &bone_mesh_norm);
  bones_material = make_material("skeleton", ROOT_PATH"sources/shaders/bones.vert", ROOT_PATH"sources/shaders/bones.frag");
  axes_mesh = make_mesh_from_data(axes_mesh_vert, axes_mesh_ind);
  axes_material = make_material("axes", ROOT_PATH"sources/shaders/bone_points.vert", ROOT_PATH"sources/shaders/bone_points.frag");

  auto material = make_material("character", ROOT_PATH"sources/shaders/character_vs.glsl", ROOT_PATH"sources/shaders/character_ps.glsl");
  std::fflush(stdout);
  material->set_property("mainTex", create_texture2d(ROOT_PATH"resources/MotusMan_v55/MCG_diff.jpg"));

  auto mesh = load_mesh(ROOT_PATH"resources/MotusMan_v55/MotusMan_v55.fbx", 0);

  RiggedMeshLoadRes meshAndSkeleton = load_rigged_mesh(ROOT_PATH"resources/MotusMan_v55/MotusMan_v55.fbx", 0);
  scene->characters.emplace_back(Character{
    glm::identity<glm::mat4>(),
    meshAndSkeleton.meshHandle,
    meshAndSkeleton.skeletonHandle,
    std::move(material)
  });
  std::fflush(stdout);
}


void game_update()
{
  arcball_camera_update(
    scene->userCamera.arcballCamera,
    scene->userCamera.transform,
    get_delta_time());
}

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  switch (scene->rmode) {
  case RMODE_REGULAR:
  {
    const Material &material = *character.material;
    const Shader &shader = material.get_shader();

    shader.use();
    material.bind_uniforms_to_shader();
    shader.set_mat4x4("Transform", character.transform);
    shader.set_mat4x4("ViewProjection", cameraProjView);
    shader.set_vec3("CameraPosition", cameraPosition);
    shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
    shader.set_vec3("AmbientLight", light.ambient);
    shader.set_vec3("SunLight", light.lightColor);

    render(*character.mesh);
  } break;

  case RMODE_BONES:
  {
    const Shader &bonesShader = bones_material->get_shader();
    const Shader &axesShader = axes_material->get_shader();

    //bonesShader.use();
    //bones_material->bind_uniforms_to_shader();
    //bonesShader.bind_ssbo(character.skeleton->boneRootTransformsBO, 0);
    //bonesShader.set_mat4x4("RootTransform", character.transform);
    //bonesShader.set_mat4x4("ViewProjection", cameraProjView);
    //bonesShader.set_vec3("CameraPosition", cameraPosition);
    //bonesShader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
    //bonesShader.set_vec3("AmbientLight", light.ambient);
    //bonesShader.set_vec3("SunLight", light.lightColor);

    //render_instanced(*bone_mesh, character.skeleton->bones.size());

    axesShader.use();
    axes_material->bind_uniforms_to_shader();
    axesShader.bind_ssbo(character.skeleton->boneOffsetsBO, 0);
    axesShader.set_mat4x4("RootTransform", character.transform);
    axesShader.set_mat4x4("ViewProjection", cameraProjView);

    render_instanced(*axes_mesh, character.skeleton->bones.size(), LINES);
  } break;

  default:
    break;
  }
}

void game_render()
{
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  const float grayColor = 0.3f;
  glClearColor(grayColor, grayColor, grayColor, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  const mat4 &projection = scene->userCamera.projection;
  const glm::mat4 &transform = scene->userCamera.transform;
  mat4 projView = projection * inverse(transform);

  for (const Character &character : scene->characters)
    render_character(character, projView, glm::vec3(transform[3]), scene->light);
}