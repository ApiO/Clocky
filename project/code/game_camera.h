#include <engine/pge_types.h>
#include <glm/glm.hpp>

namespace game
{
  using namespace pge;

  struct GameCamera {
    enum Mode {
      CLAMPED_TO_BOX,
      CLAMPED_TO_PATH,
      FOLLOW_PATH_CONST,
      FOLLOW_PATH_CURVE,
      SHAKE,
      FREE
    };

    u64        id;
    u64        world;
    f32        rfov;
    f32        ratio;
    glm::vec2  target;

    Mode        mode;
    glm::vec2   clamp_min;
    glm::vec2   clamp_max;

    const glm::vec3 *path;
    u32              path_size;
    Array<f32>      *path_data;
    u32              path_head;

    const f32       *key_time;
    const f32       *key_value;
    u32              key_head;
    u32              num_keys;

    f64              effect_time;
    f64              effect_duration;

    f32              shake_freq;
    f32              shake_mag;
    f32              shake_rad_freq;
    f32              shake_rad_mag;
    glm::vec2        shake_dir;
  };

  namespace game_camera
  {
    const f32 DEFAULT_DAMPING = 4.0f;
    const f32 DEFAULT_FOV     = 45.0f;
    const f32 DEFAULT_NEAR    = 0.2f;
    const f32 DEFAULT_FAR     = 100000.0f;

    void init          (GameCamera &c, u64 world, const glm::vec2 &resolution, Allocator &a);
    void clamp_to_box  (GameCamera &c, const glm::vec3 &min, const glm::vec2 &max);
    void clamp_to_path (GameCamera &c, const glm::vec3 *path, u32 path_size);
    void follow_path   (GameCamera &c, const glm::vec3 *path, u32 path_size, f32 speed);
    void follow_path   (GameCamera &c, const glm::vec3 *path, u32 path_size, const f32 *key_time, const f32 *key_value, u32 num_keys);
    void shake         (GameCamera &c, const glm::vec2 &dir, f32 frequency, f32 magnitude, f32 time, 
                        f32 radial_frequency = 0, f32 radial_magnitude = 0);
    void free          (GameCamera &c);
    void set_target    (GameCamera &c, const glm::vec2 &t);
    void update        (GameCamera &c, f64 delta_time);
    void destroy       (GameCamera &c);
  }
}