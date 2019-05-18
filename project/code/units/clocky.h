#include <engine/pge.h>
#include "clocky_types.h"

namespace game
{
  using namespace pge;

  namespace clocky
  {
    const f32 ATTACK_ADVANCE = 20.0f;
    const f64 ATTACK_ADVANCE_DURATION = 0.1f;
    const f32 UNIT_WIDTH     = 40.0f;
    const f32 ATTACK_RANGE   = 64.0f;
  }

  namespace clocky
  {
    void spawn(Clocky &c, u64 world, const glm::vec2 &position, f32 lifetime, EnemyController &enemies, Array<u64> &fx);
    void add_lifetime(Clocky &c, i32 lifetime);
    void recieve_damage(const glm::vec2 &direction, i32 damage);
    void update(Clocky &c, f64 delta_time);
    void destroy(Clocky &c);
  }
}