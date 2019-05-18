#pragma once

#include "enemies/enemies.h"

namespace game
{
  struct EnemyController
  {
    pge::u64 world;
    pge::u64 player;
    
    pge::Hash<Walker>    *walkers;
    pge::Hash<Flybot>    *flybots;
    pge::Hash<Spikejonz> *spikejonz;

    pge::i32 _lifetime_gain;
  };


  namespace enemy_controller
  {
    void init     (EnemyController &controller, pge::u64 world, pge::u64 player, pge::Allocator &a);
    void shutdown (EnemyController &controller);
    void update   (EnemyController &controller, pge::f64 delta_time);

    void register_unit   (EnemyController &controller, pge::u64 unit);
    void unregister_unit (EnemyController &controller, pge::u64 unit);

    void recieve_damage (EnemyController &controller, pge::u64 unit, const glm::vec2 &direction, pge::i32 damage);
    pge::i32 lifetime_gain  (EnemyController &controller);
  }

  namespace enemy_controller
  {
    inline pge::i32 lifetime_gain(EnemyController &controller) { return controller._lifetime_gain; }
  }
}