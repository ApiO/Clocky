#pragma once

#include <glm/glm.hpp>
#include <runtime/types.h>

namespace game
{
  const pge::f32  DEFAULT_ROTATION_SPEED = glm::radians(3.f * 60);
  const pge::f32  DEFAULT_MOVE_SPEED = 30.f * 60;
  const pge::f32  DEFULAT_FOV = 45.f;
  const pge::f32  DEFULAT_NEAR_RANGE = -1.f;
  const pge::f32  DEFULAT_FAR_RANGE = 1.f;
  const glm::vec3 DEFAULT_UP(0.f, 1.f, 0.f);
  const glm::vec3 DEFAULT_FORWARD(0.f, 0.f, 1.f);
  const glm::vec3 DEFAULT_EULER_ANGLES(0.f);
  const glm::vec3 DEFAULT_TRANSLATION(0.f, 0.f, 0.f);
  const glm::quat DEFAULT_ROTATION(1.f, 0.f, 0.f, 0.f);
  const glm::mat4 IDENTITY_MATRIX(1.f);


  enum CameraDirection {
    CAMERA_MOVE_UP,
    CAMERA_MOVE_DOWN,
    CAMERA_MOVE_LEFT,
    CAMERA_MOVE_RIGHT,
    CAMERA_MOVE_FORWARD,
    CAMERA_MOVE_BACKWARD
  };
  enum CameraMode
  {
    CAMERA_MODE_FREE,
    CAMERA_MODE_OTRHO,
    CAMERA_MODE_LOOK_AT
  };

  class Camera
  {
  public:
    Camera();

    void init(pge::u64 world, CameraMode mode, pge::f32 width, pge::f32 height);

    void shutdown();

    void reset();

    // Sets the camera's mode
    void set_mode(CameraMode mode, pge::f32 near, pge::f32 far);

    // Sets the camera's orientation. Changes ignored if CAMERA_MODE_LOOK_AT is defined
    void set_euler_angles(const glm::vec3 &r);

    // Sets the camera's up vector
    void set_up(const glm::vec3 &up);

    // Sets the camera's forward vector
    void set_forward(const glm::vec3 &forward);

    // Sets the camera's move speed
    void set_move_speed(pge::f32 v);

    // Sets the orthographic projection matrix. Use if CAMERA_MODE_ORTHO
    void set_orthographic_projection(pge::f32 left, pge::f32 right, pge::f32 bottom, pge::f32 top);

    // Sets the vertical angle of view of the camera. Changes ignored if CAMERA_MODE_ORTHO is defined
    void set_vertical_fov(pge::f32 degree);

    // Sets the camera position. Position value is changed by function "move" too.
    void set_position(const glm::vec3 &position);

    // Sets the camera's rotation speed
    void set_rotation_speed(pge::f32 v);

    // Defines the point's coordinates that the camera is looking at. Used if CAMERA_MODE_LOOK_AT is defined
    void set_look_at(const glm::vec3 &look_at);

    // Sets the near range value according to the forward vector
    void set_near_range(pge::f32 value);

    // Sets the far range value according to the forward vector
    void set_far_range(pge::f32 value);

    // Updates the camera's position depending on its mode
    void move(CameraDirection direction, pge::f64 delta_time);

    // Applies all changes on camera, depending on its mode
    void update();

    pge::u64    get_id();
    CameraMode  get_mode();
    glm::vec3   get_euler_angles();
    glm::vec3   get_position();
    glm::vec3   get_up();
    glm::vec3   get_forward();
    pge::f32    get_move_speed();
    pge::f32    get_rotation_speed();
    glm::vec3   get_look_at();
    pge::f32    get_near_range();
    pge::f32    get_far_range();
    pge::f32    get_vertical_fov();

  private:
    pge::u64    id;
    pge::u64    world;
    CameraMode  mode;
    bool        dirty;
    glm::vec3   up;
    glm::vec3   forward;
    glm::vec3   euler_angles;
    glm::vec3   position;
    glm::vec3   look_at;
    pge::f32    move_speed;
    pge::f32    rotation_speed;
    pge::f32    z_near;
    pge::f32    z_far;
    pge::f32    vertical_fov;
  };
}