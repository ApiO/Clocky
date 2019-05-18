#include "behaviors.h"
#include <engine/pge.h>

using namespace pge;

namespace
{
  inline f32 ease_in_sine(f32 t, f32 b, f32 c, f32 d) {
    return -c / 2 * (cos(PI*t / d) - 1) + b;
  };
}


namespace game
{
  MoverData::MoverData(f32 fs, f32 fad) :
    fall_speed(fs),
    fall_acc_duration(fad),
    time(0),
    state(MOVER_STATE_FALLING),
    pinned(false) { }

  void handle_ground_mover(u64 world, u64 mover, glm::vec2 &velocity, MoverData &data, const f64 delta_time)
  {
    glm::vec3 p;

    switch (data.state) {
    case MOVER_STATE_FALLING:
      data.time += delta_time;
      velocity.y = data.time < data.fall_acc_duration
        ? ease_in_sine((f32)data.time, 0, data.fall_speed, data.fall_acc_duration)
        : data.fall_speed;
      velocity.y *= -1;
      if (mover::collides_down(world, mover))
        data.state = MOVER_STATE_GROUNDED;
      break;
    case MOVER_STATE_GROUNDED:
      velocity.y = 0;
      mover::get_position(world, mover, p);
      physics::cast_raycast(world, raycast_to_ground, glm::vec3(p.x, p.y + 1, p.z), glm::vec3(p.x, p.y - 64, p.z));
      velocity = ground_dir * velocity.x;
      if (!mover::collides_down(world, mover)) {
        if (_ground_found) {
          velocity.y -= ground_dist / (f32)delta_time;
        } else {
          data.time = 0;
          data.state = MOVER_STATE_FALLING;
        }
      }
      break;
    case MOVER_STATE_MANUAL:
      break;
    }

    mover::move(world, mover, glm::vec3(velocity, 0) * (f32)delta_time);
  }
}