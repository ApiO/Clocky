#include "enemy_controller.h"

#include <engine/pge.h>
#include <runtime/hash.h>
#include <runtime/assert.h>
#include <runtime/temp_allocator.h>
#include <runtime/murmur_hash.h>

using namespace pge;

namespace
{
  using namespace game;

  const u32 WALKER_RES_ID = 2925124307u;
  const u32 FLYBOT_RES_ID = 2150305649u;
  const u32 SPIKEJONZ_RES_ID = 664006284u;


  template<typename T>
  inline void update_enemies(u64 world, Hash<T> &enemies, f64 delta_time, glm::vec3 player_position)
  {
    u32 i = 0;
    Hash<T>::Entry *begin = hash::begin(enemies);

    while (i < hash::size(enemies)) {
      Hash<T>::Entry &entry = begin[i];

      entry.value.update(delta_time, player_position);

      if (entry.value.despawn_time <= 0.f) {
        world::despawn_unit(world, entry.key);
        hash::remove(enemies, entry.key);
        continue;
      }
      i++;
    }
  }

  inline void _register_unit(EnemyController &controller, u64 unit)
  {
    switch (unit::name_hash(controller.world, unit)) {
    case WALKER_RES_ID:
    {
      Walker w;
      w.mover = pge::unit::mover(controller.world, unit, 0);
      w.world = controller.world;
      w.unit = unit;
      w.hitbox = unit::actor(w.world, unit, "hitbox");
      hash::set(*controller.walkers, unit, w);
    }
      break;
    case FLYBOT_RES_ID:
    {
      Flybot f;
      f.world = controller.world;
      f.unit = unit;
      f.hitbox = unit::actor(f.world, unit, "hitbox");
      hash::set(*controller.flybots, unit, f);
    }
      break;
    case SPIKEJONZ_RES_ID:
    {
      Spikejonz s;
      s.world = controller.world;
      s.unit = unit;
      hash::set(*controller.spikejonz, unit, s);
    }
      break;
    }
  }

  void raycast_to_player_cb(const Array<ContactPoint> &hits) {
    _player_found = array::size(hits) > 0u && array::begin(hits)->actor == player_hitbox;
  }
  void raycast_to_ground_cb(const Array<ContactPoint> &hits) {
    _ground_found = array::size(hits) > 0u;

    if (!_ground_found) 
      return;

    f32 tmp;
    ground_dir = glm::normalize(glm::vec2(hits[0].normal));
    tmp = ground_dir.x;
    ground_dir.x = ground_dir.y;
    ground_dir.y = tmp * -1;
    ground_dist  = hits[0].distance;
  }
}

namespace game
{
  bool _player_found;
  u64  player_unit;
  u64  player_hitbox;
  u64  raycast_to_player;

  bool _ground_found;
  glm::vec2 ground_dir;
  f32       ground_dist;
  u64  raycast_to_ground;

  namespace enemy_controller
  {
    void init(EnemyController &controller, u64 world, u64 player, Allocator &a)
    {
      TempAllocator4096 ta(a);
      Array<u64> units(ta);

      controller.world = world;
      controller.player = player;
      controller._lifetime_gain = 0;

      controller.walkers = MAKE_NEW(a, Hash<Walker>, a);
      controller.flybots = MAKE_NEW(a, Hash<Flybot>, a);
      controller.spikejonz = MAKE_NEW(a, Hash<Spikejonz>, a);

      world::get_units(world, units);

      for (u32 i = 0; i < array::size(units); i++)
        _register_unit(controller, units[i]);

      player_unit   = player;
      player_hitbox = unit::actor(world, player, "hitbox");

      raycast_to_player = physics::create_raycast(world, raycast_to_player_cb, "enemy_cast", true, false);
      raycast_to_ground = physics::create_raycast(world, raycast_to_ground_cb, "ground_cast", true, false);
    }

    void update(EnemyController &controller, f64 delta_time)
    {
      glm::vec3 pp;
      unit::get_local_position(controller.world, controller.player, 0, pp);

      update_enemies(controller.world, *controller.walkers, delta_time, pp);
      update_enemies(controller.world, *controller.flybots, delta_time, pp);
      update_enemies(controller.world, *controller.spikejonz, delta_time, pp);

      controller._lifetime_gain = 0;
    }

    void shutdown(EnemyController &controller)
    {
      physics::destroy_raycast(controller.world, raycast_to_player);

      Allocator &a = *controller.walkers->_data._allocator;

      MAKE_DELETE(a, Hash<Walker>, controller.walkers);
      MAKE_DELETE(a, Hash<Flybot>, controller.flybots);
      MAKE_DELETE(a, Hash<Spikejonz>, controller.spikejonz);

      controller.player = controller.world = u64(-1);
    }


    void register_unit(EnemyController &controller, u64 unit)
    {
      _register_unit(controller, unit);
    }

    void unregister_unit(EnemyController &controller, u64 unit)
    {
      switch (unit::name_hash(controller.world, unit)) {
      case WALKER_RES_ID:
        hash::remove(*controller.walkers, unit);
        return;
      case SPIKEJONZ_RES_ID:
        hash::remove(*controller.flybots, unit);
        return;
      case FLYBOT_RES_ID:
        hash::remove(*controller.spikejonz, unit);
        return;
      }
      XERROR("Unhandled enemy type: %u", unit::name_hash(controller.world, unit));
    }


    void recieve_damage(EnemyController &controller, u64 unit, const glm::vec2 &direction, i32 damage)
    {
      switch (unit::name_hash(controller.world, unit)) {
      case WALKER_RES_ID:
      {
        Walker &walker = *((Walker*)hash::get(*controller.walkers, unit));
        walker.recieve_damage(direction, damage);
        if (walker.hp < 1)
          controller._lifetime_gain += WALKER_LIFETIME_GAIN;
      }
        return;
      case FLYBOT_RES_ID:
      {
        Flybot &flybot = *((Flybot*)hash::get(*controller.flybots, unit));
        flybot.recieve_damage(direction, damage);
        if (flybot.hp < 1)
          controller._lifetime_gain += FLYBOT_LIFETIME_GAIN;
      }
        return;
      case SPIKEJONZ_RES_ID:
        ((Spikejonz*)hash::get(*controller.spikejonz, unit))->recieve_damage(direction, damage);
        return;
      }
      XERROR("Unhandled enemy type: %u", unit::name_hash(controller.world, unit));
    }

  }
}