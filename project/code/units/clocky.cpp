#include "clocky.h"
#include <runtime/array.h>
#include <runtime/hash.h>
#include <runtime/temp_allocator.h>
#include <glm/gtx/vector_angle.hpp>

#include <enemy_controller.h>

namespace game
{
  namespace clocky
  {
    const f32 STICK_THRESHOLD      = 0.3f;
    const f32 DTAP_THRESHOLD       = 0.6f;
    const f32 SPEED                = 512.0f;    // pixels per second
    const f32 SLIDING_SPEED        = 512.0f;
    const f32 DASH_SPEED           = 2048.0f;
    const f32 DASH_RANGE           = 256.0f;
    const f32 DASH_COOLDOWN        = 0.2f;
    const f32 DASH_TAP_COOLDOWN    = 0.25f;
    const f32 ATP_CONE             = PI * 0.8f; // angle in radians
    const f32 ATP_SPEED            = 2048.0f;   // pixels per second
    const f32 ATP_RANGE            = 300.0f;    // be careful to sync it with the dash_sphere.pgshape
    const f32 ATP_GHOST_DURATION   = 0.5f;
    const f64 ATTACK_COOLDOWN      = 0.2;
    const f32 ATTACK_IMPULSE       = 5.0f;
    const f32 ATTACK_CONE          = PI / 2.0f;
    const f64 FLY_DURATION         = 0.4f; // duration of fly after an attack
    const f32 FLYING_SPEED         = 128.0f;
    const f32 AERIAL_ACCELERATION_TIME = 0.4f;
    const f32 GROUND_ACCELERATION_TIME = 0.2f;
    const f32 STUN_DURATION        = 0.1f;
    const f32 STUN_RECOIL          = 40.0f;
    const f32 COMBO_MAX_COUNTDOWN  = 3.0f;  // combo countdown at the first hit
    const f32 COMBO_MIN_COUNTDOWN  = 1.0f;  // combo countdown at the COMBO_CAP number of hits 
    const i32 COMBO_MAX_MULTIPLIER = 3;     // attack & lifegain multiplier at the COMBO_CAP number of hits 
    const i32 COMBO_CAP            = 30;

    // jump vars
    const f32 ASCENT_HEIGHT   = 256.0f;
    const f32 ASCENT_DURATION = 0.3f;
    const f32 FALL_SPEED      = 2048.0f;
    const f32 FALL_ACCELERATION_DURATION = 1.0f;
  }

  inline void update_orientation(Clocky &c)
  {
    if (c.orientation == c.aiming_dir.x > 0) return;

    c.orientation = c.aiming_dir.x > 0;

    glm::quat q;
    pge::unit::get_local_rotation(c._world, c._unit, 0, q);

    glm::vec3 a(glm::degrees(glm::eulerAngles(q)));
    a.y += (c.orientation ? -1 : 1) * 180.f;

    pge::unit::set_local_rotation(c._world, c._unit, 0, glm::quat(glm::radians(a)));
  }
}

namespace
{
  using namespace pge;
  using namespace game;
  using namespace clocky;

  Clocky *_instance;

  void update_direction_and_commands(Clocky &c, f64 dt)
  {
    DTapOrientation last_dtap_orientation = c.dtap_orientation;
    c.dtap_cooldown -= dt;
    c.should_move = false;
    c.dtap_orientation = DTAP_NONE;

    c.aiming_dir.x = 0;
    c.aiming_dir.y = 0;

    for (u32 i = 0; i < NUM_COMMANDS; i++)
      c.cmd[i] = false;

    if (pad::active(0)) {
      // update direction from gamepad
      const f32 ax = abs(pad::axes(_instance->pad_index, 0));
      const f32 ay = abs(pad::axes(_instance->pad_index, 1));

      if (ax > STICK_THRESHOLD)
        c.aiming_dir.x = pad::axes(_instance->pad_index, 0);

      if (ay > STICK_THRESHOLD)
        c.aiming_dir.y = pad::axes(_instance->pad_index, 1) * -1;

      if (ax > DTAP_THRESHOLD || ay > DTAP_THRESHOLD) {
        if (ax > ay) {
          c.dtap_orientation = pad::axes(_instance->pad_index, 0) > 0 ?
          DTAP_RIGHT : DTAP_LEFT;
        } else {
          c.dtap_orientation = pad::axes(_instance->pad_index, 1) > 0 ?
          DTAP_DOWN : DTAP_NONE; //DTAP_UP;
        }
      }

      // update commands from gamepad
      if (pad::pressed(_instance->pad_index, PAD_KEY_2))
        c.cmd[CMD_JUMP] = true;

      if (pad::pressed(_instance->pad_index, PAD_KEY_1))
        c.cmd[CMD_ATTACK] = true;
    } else {
      // update direction from keyboard
      if (keyboard::button(KEYBOARD_KEY_RIGHT) && !keyboard::button(KEYBOARD_KEY_LEFT)) {
        c.aiming_dir.x = 1;
        c.dtap_orientation = DTAP_RIGHT;
      }

      if (keyboard::button(KEYBOARD_KEY_LEFT) && !keyboard::button(KEYBOARD_KEY_RIGHT)) {
        c.aiming_dir.x = -1;
        c.dtap_orientation = DTAP_LEFT;
      }

      if (keyboard::button(KEYBOARD_KEY_UP) && !keyboard::button(KEYBOARD_KEY_DOWN)) {
        c.aiming_dir.y = 1;
        c.dtap_orientation = DTAP_NONE; //DTAP_UP;
      }

      if (keyboard::button(KEYBOARD_KEY_DOWN) && !keyboard::button(KEYBOARD_KEY_UP)) {
        c.aiming_dir.y = -1;
        c.dtap_orientation = DTAP_DOWN;
      }

      // update commands from keyboard
      if (keyboard::pressed(KEYBOARD_KEY_SPACE))
        c.cmd[CMD_JUMP] = true;

      if (keyboard::pressed(KEYBOARD_KEY_LEFT_CONTROL))
        c.cmd[CMD_ATTACK] = true;
    }

    if (c.aiming_dir == glm::vec2(0, 0)) {
      c.aiming_dir.x = c.orientation ? 1.0f : -1.0f;
    } else {
      c.aiming_dir = glm::normalize(c.aiming_dir);
      if (c.aiming_dir.x != 0) {
        update_orientation(c);
        c.should_move = true;
      }
    }

    if (c.dtap_orientation != DTAP_NONE) {
      if (last_dtap_orientation == DTAP_NONE) {
        if (c.last_oriented_dtap == c.dtap_orientation && c.dtap_cooldown > 0)
          c.cmd[CMD_DASH] = true;
        c.dtap_cooldown = DASH_TAP_COOLDOWN;
      }
      c.last_oriented_dtap = c.dtap_orientation;
    }
  }

  void update_close_targets(const Clocky &c)
  {
    glm::vec3 pp, ep; // player & enemy position
    glm::vec2 ed;     // enemy direction
    Target    t;

    unit::get_world_position(c._world, c._unit, 0, pp);

    const Hash<Target>::Entry *e, *end = hash::end(*c.close_targets);
    for (e = hash::begin(*c.close_targets); e < end; e++) {
      unit::get_world_position(c._world, actor::unit(c._world, e->key), 0, ep);
      t.position  = glm::vec2(ep);
      t.direction = glm::normalize(glm::vec2(ep - pp));
      t.distance  = glm::distance(pp, ep) - 40;
      hash::set(*c.close_targets, e->key, t);
    }
  }

  u64 find_best_target(const Clocky &c) {
    f32 best_score = -1.0f;
    f32 tmp_score;
    u64 best_target = 0;

    const Hash<Target>::Entry *e, *end = hash::end(*c.close_targets);
    for (e = hash::begin(*c.close_targets); e < end; e++) {
      tmp_score = glm::angle(c.aiming_dir, e->value.direction);
      if (tmp_score > ATP_CONE / 2.0) // discard the target if it's not in the dashing cone
        continue;

      tmp_score /= ATP_CONE / 2.0f; // ponderate the angle score
      tmp_score += e->value.distance / ATP_RANGE; // add the ponderated distance to the score

      if (tmp_score < best_score || !best_target) {
        best_score  = tmp_score;
        best_target = e->key;
      }
    }
    return best_target;
  }

  inline f32 ease_out_sine(f32 t, f32 b, f32 c, f32 d) {
    return c * sin(t / d * (PI / 2)) + b;
  };

  inline f32 ease_in_sine(f32 t, f32 b, f32 c, f32 d) {
    return -c / 2 * (cos(PI*t / d) - 1) + b;
  };

  void register_close_target(const Array<ContactPoint> &contacts, const void * user_data)
  {
    Target t;
    for (u32 i = 0; i < array::size(contacts); i++) {
      t.distance  = contacts[i].distance;
      t.position  = glm::vec2(contacts[i].position);
      t.direction = glm::vec2(); // will be calculated when needed
      hash::set(*_instance->close_targets, contacts[i].actor, t);
    }
  }

  void unregister_close_Enemy(const Array<ContactPoint> &contacts, const void * user_data)
  {
    for (u32 i = 0; i < array::size(contacts); i++)
      hash::remove(*_instance->close_targets, contacts[i].actor);
  }

  void show_impact_text(u64 world, u64 text, const glm::vec2 &position)
  {
    const char *texts[3] ={ "ZBRAAA!", "TAVU!", "BOOM!" };
    const i32 i = rand() % 3;
    glm::vec2 v;
    text::set_string(world, text, texts[i]);
    text::get_size(world, text, v);

    v.x = position.x - v.x / 2;
    v.y = position.y;

    text::set_local_position(world, text, glm::vec3(v, 256));
  }

  void hide_impact_text(u64 world, u64 text)
  {
    text::set_local_position(world, text, glm::vec3(0, 0, 9999));
  }

  void delete_atp_ghosts(Clocky &c, i32 from)
  {
    for (u32 i = from; i < NUM_GHOSTS; i++) {
      if (c._atp_ghosts[i]) {
        world::despawn_geometry(c._world, c._atp_ghosts[i]);
        c._atp_ghosts[i] = 0;
      }
    }
  }

  void show_atp_ghosts(Clocky &c, const glm::vec3 &from, const glm::vec3 &to)
  {
    const i32 num_ghosts = (i32)(NUM_GHOSTS * glm::distance(to, from) / ATP_RANGE);
    glm::vec3 verts[4] ={ { 0, 80, 0 }, { 0, 0, 0 }, { 40, 0, 0 }, { 40, 80, 0 } };
    const glm::vec3 dp = (to - from) / (f32)num_ghosts;
    glm::vec3 p = from;

    delete_atp_ghosts(c, 0);

    Color color;
    for (i32 i = 0; i < num_ghosts; i++) {
      color = Color(255, 255, 255, 255 - 200 / (i + 1));
      c._atp_ghosts[i] = world::spawn_polygon(c._world, verts, 4, color, p, glm::quat(1, 0, 0, 0));
      p += dp;
    }
    c._atp_ghost_cooldown = ATP_GHOST_DURATION / NUM_GHOSTS * num_ghosts;
  }

  void update_tp_ghosts(Clocky &c, f64 dt)
  {
    const f32 lt = ATP_GHOST_DURATION / NUM_GHOSTS;
    f32 tmp;
    i32 to_delete;
    glm::vec3 dp;
    glm::vec3 p;
    c._atp_ghost_cooldown -= (f32)dt;
    modf((c._atp_ghost_cooldown + lt) / lt, &tmp);
    to_delete = (i32)tmp + 1;

    if (to_delete >= NUM_GHOSTS || !c._atp_ghosts[to_delete])
      return;

    if (to_delete <= 0) {
      delete_atp_ghosts(c, 0);
      return;
    }

    geometry::get_local_position(c._world, c._atp_ghosts[to_delete], p);
    geometry::get_local_position(c._world, c._atp_ghosts[to_delete - 1], dp);
    dp = p - dp;

    for (i32 i = 0; i < to_delete; i++) {
      geometry::get_local_position(c._world, c._atp_ghosts[i], p);
      geometry::set_local_position(c._world, c._atp_ghosts[i], p + dp);
    }
    delete_atp_ghosts(c, to_delete);
  }

  bool deal_damages(Clocky &c)
  {
    i32 hits = 0;

    glm::vec3 p;
    unit::get_local_position(_instance->_world, _instance->_unit, 0, p);

    const Hash<Target>::Entry *e, *end = hash::end(*c.close_targets);
    for (e = hash::begin(*c.close_targets); e < end; e++) {
      if (e->value.distance > ATTACK_RANGE || glm::angle(c.attack_dir, e->value.direction) > ATTACK_CONE / 2)
        continue;
      audio::trigger_sound(_instance->_world, "sfx/punch");
      show_impact_text(_instance->_world, _instance->_impact_text, e->value.position);
      enemy_controller::recieve_damage(*c.enemies, actor::unit(_instance->_world, e->key), _instance->attack_dir, c.combo_multiplier);
      {
        u64 impact = world::spawn_unit(_instance->_world, "fx/fxImpact00/fxImpact00", 
                                       glm::vec3(e->value.position.x, e->value.position.y + 40, p.z), IDENTITY_ROTATION, glm::vec3(3, 3, 1));
        unit::play_animation(_instance->_world, impact, "impact", 0, 0, false, 1.f);
        array::push_back(*_instance->fx, impact);
      }
      hits += 1;
    }

    if (hits == 0)
      return false;

    c.combo_countdown = c.combo_hits > COMBO_CAP
      ? COMBO_MIN_COUNTDOWN
      : COMBO_MAX_COUNTDOWN - ((COMBO_MAX_COUNTDOWN - COMBO_MIN_COUNTDOWN) * (f32)c.combo_hits / (f32)COMBO_CAP);
    c.combo_hits += hits;

    printf("combo countdown: %f\n", c.combo_countdown);

    return true;
  }

  void to_ascending_state(Clocky &c) {
    glm::vec3 p;
    mover::get_position(c._world, c._mover, p);
    c.fly_from   = p.y;
    c.state_time = 0;
    c.pinned = false;
    if (mover::collides_sides(c._world, c._mover)) {
      c.velocity.x = (mover::collides_right(c._world, c._mover) ? -1 : 1) * SPEED;
      update_orientation(c);
    }
    c.state = STATE_ASCENDING;
    mover::set_collision_filter(c._world, c._mover, "mover_ascending");
  }

  void to_falling_or_grounded_state(Clocky &c) {
    c.state_time = 0;
    if (mover::collides_down(c._world, c._mover) || c.pinned) {
      c.state = STATE_GROUNDED;
    } else {
      if (c.state == STATE_SLIDING)
        c.state_time = ease_out_sine(abs(c.velocity.y), 0, FALL_ACCELERATION_DURATION, FALL_SPEED);
    }
    c.state = STATE_FALLING;
  }

  void to_flying_or_grounded_state(Clocky &c) {
    if (mover::collides_down(c._world, c._mover) || c.pinned) {
      c.state = STATE_GROUNDED;
      c.state_time = 0;
    } else {
      c.state = STATE_FLYING;
      c.state_time = FLY_DURATION;
    }
  }

  void to_dash_state(Clocky &c) {
    switch (c.dtap_orientation) {
    case DTAP_RIGHT:
      if (c.attack_dir.x > 0 && c._dash_cooldown > 0)
        return;
      c.attack_dir = glm::vec2(1, 0);
      break;
    case DTAP_LEFT:
      if (c.attack_dir.x < 0 && c._dash_cooldown > 0)
        return;
      c.attack_dir = glm::vec2(-1, 0);
      break;
    case DTAP_DOWN:
      if (c.attack_dir.y < 0 && c._dash_cooldown > 0)
        return;
      c.attack_dir = glm::vec2(0, -1);
      break;
    }

    if (c.state == STATE_GROUNDED) {
      if (c.dtap_orientation == DTAP_DOWN)
        mover::set_collision_filter(c._world, c._mover, "mover_ascending"); // fall down from platform
      else
        c.attack_dir = c.ground_dir * c.attack_dir.x;
    }

    else
      c.can_air_dash = false;

    c.state_time = DASH_RANGE / DASH_SPEED;
    c.state = STATE_DASHING;
    c._dash_cooldown = (f32)DASH_COOLDOWN;
  }

  void to_attack_state(Clocky &c)
  {
    c.state_time = 0;
    c.state = STATE_ATTACK;
    c.about_to_attack = true;
  }

  void to_atp_or_attack_state(Clocky &c)
  {
    // if min_distance is positive, tp to the target
    if (!c.best_target)
      return;

    const Target &t = *hash::get(*c.close_targets, c.best_target);
    c.attack_dir = t.direction;
    if (abs(t.distance) < ATTACK_RANGE) {
      // if the target is in the attack range, directly attack
      to_attack_state(c);
    } else {
      // if the target is not in the attack range, tp and then attack
      c.pinned = false;
      audio::trigger_sound(c._world, "sfx/dash");
      c.state_time = t.distance / ATP_SPEED;
      c.attack_cooldown = -1.0f;
      c.state = STATE_ATP;
    }
  }

  void to_stun_state(Clocky &c)
  {
    c.state_time = 0;
    c.state = STATE_STUNNED;
  }

  void handle_ground_raycast(const Array<ContactPoint> &hits)
  {
    if (!array::size(hits)) {
      _instance->pinned = false;
      return;
    }

    f32 tmp;
    _instance->ground_dir = glm::normalize(glm::vec2(hits[0].normal));
    tmp = _instance->ground_dir.x;
    _instance->ground_dir.x = _instance->ground_dir.y;
    _instance->ground_dir.y = tmp * -1;
    _instance->ground_dist  = hits[0].distance;
  }

  void recieve_bullet_damages(const Array<ContactPoint> &contacts, const void * user_data)
  {
    glm::vec3 p;
    actor::get_world_position(_instance->_world, _instance->_hitbox, p);

    for (u32 i = 0; i < array::size(contacts); i++) {
      recieve_damage(glm::normalize(glm::vec2(contacts[i].position - p)), 1);
      world::despawn_unit(_instance->_world, actor::unit(_instance->_world, contacts[i].actor));
    }
  }

  void on_platform_touched(const Array<ContactPoint> &contacts, const void * user_data)
  {
    _instance->num_overlapping_platforms += array::size(contacts);
  }

  void on_platform_untouched(const Array<ContactPoint> &contacts, const void * user_data)
  {
    _instance->num_overlapping_platforms -= array::size(contacts);
  }
}

namespace game
{
  namespace clocky
  {
    void spawn(Clocky &c, u64 world, const glm::vec2 &position, f32 lifetime, EnemyController &enemies, Array<u64> &fx)
    {
      Allocator &a = memory_globals::default_allocator();

      c.state            = STATE_FALLING;
      c.ascending        = false;
      c.about_to_attack  = false;
      c.can_air_dash     = true;
      c.orientation      = true;
      c.should_move      = false;
      c.aiming_dir       = glm::vec2(1, 0);
      c._world           = world;
      c._unit            = world::spawn_unit(world, "units/clocky/clocky", glm::vec3(position, 0), glm::quat(1.f, 0.f, 0.f, 0.f));
      c._mover           = unit::mover(world, c._unit, 0);
      c._hitbox          = unit::actor(world, c._unit, "hitbox");
      c._box             = unit::actor(world, c._unit, "box");
      c._atp_sphere      = unit::actor(world, c._unit, "dash_sphere");
      c._impact_text     = world::spawn_text(world, "fonts/consolas.24/consolas.24", "", TEXT_ALIGN_LEFT, glm::vec3(0, 0, -9999), glm::quat(1, 0, 0, 0));
      c.close_targets    = MAKE_NEW(a, Hash<Target>, a);
      c.velocity         = glm::vec2(0, 0);
      c.dtap_cooldown    = 0;
      c.attack_cooldown  = 0;
      c.pad_index        = 0;
      c.pinned           = false;
      c.ground_dist      = 0;
      c._atp_ghost_cooldown = 0;
      c.dtap_orientation = DTAP_NONE;
      c.enemies          = &enemies;
      c.lifetime         = lifetime;
      c.combo_hits       = 0;
      c.num_overlapping_platforms = 0;
      c.fx = &fx;

      for (u32 i = 0; i < NUM_GHOSTS; i++)
        c._atp_ghosts[i] = 0;

      // create the ground raycast
      c._ground_rcast = physics::create_raycast(world, handle_ground_raycast, "ground_cast", true);

      // set the callback for the dash_sphere actor
      actor::set_touched_callback(world, c._atp_sphere, register_close_target, NULL);
      actor::set_untouched_callback(world, c._atp_sphere, unregister_close_Enemy, NULL);

      // set the callback for the hitbox to recieve bullet damages
      actor::set_touched_callback(world, c._hitbox, recieve_bullet_damages, NULL);

      // set the callbacks for the box to know when clocky is overlapping a platform
      actor::set_touched_callback(world, c._box, on_platform_touched, NULL);
      actor::set_untouched_callback(world, c._box, on_platform_untouched, NULL);

      // play the idle animation
      unit::play_animation(world, c._unit, "idle", 0, 0, true);

      // finds first first pad slot
      for (u32 i = 0; i < pge::MAX_NUM_PADS; i++)
        if (pad::active(i) && pad::num_buttons(i) < 15) {
          c.pad_index = i;
          break;
        }

      _instance = &c;
    }

    void update(Clocky &c, f64 delta_time)
    {
      glm::vec3 p(0, 0, 0); // player position
      glm::vec3 tmp;

      _instance->hitted = false;
      c.lifetime -= (f32)delta_time;
      c.combo_countdown -= delta_time;
      c._dash_cooldown  -= (f32)delta_time;

      if (c.combo_countdown <= 0)
        c.combo_hits = 0;

      c.combo_multiplier = c.combo_hits >= COMBO_CAP
        ? (COMBO_MAX_MULTIPLIER)
        : i32(1 + floor((COMBO_MAX_MULTIPLIER - 1) * (f32)c.combo_hits / (f32)COMBO_CAP));

      update_direction_and_commands(c, delta_time);
      mover::get_position(c._world, c._mover, p);
      update_close_targets(c);
      update_tp_ghosts(c, delta_time);
      c.best_target = find_best_target(c);

      if (c.state != STATE_ASCENDING && c.num_overlapping_platforms <= 0) {
        c.num_overlapping_platforms = 0;
        mover::set_collision_filter(c._world, c._mover, "mover");
      }

      if (c.state == STATE_GROUNDED || c.state == STATE_DASHING)
        physics::cast_raycast(c._world, c._ground_rcast, glm::vec3(p.x, p.y + 1, p.z), glm::vec3(p.x, p.y - 64, p.z));

      if (mover::collides_sides(c._world, c._mover))
        c.can_air_dash = true;

      c.attack_cooldown  -= delta_time;
      if (c.attack_cooldown < 0)
        hide_impact_text(c._world, c._impact_text);

      if (c.state != STATE_ATP && c.state != STATE_DASHING && c.state != STATE_ATTACK && c.state != STATE_STUNNED) {
        if (c.state == STATE_GROUNDED && !c.should_move) {
          c.velocity.x = 0; // directly stop the player if he's on the ground
        } else {
          f32 target_speed = (c.state == STATE_FLYING ? FLYING_SPEED : SPEED);
          f32 target_vel = !c.should_move ? 0 : target_speed * (c.orientation ? 1 : -1);
          f32 direction  = target_vel - c.velocity.x > 0 ? 1.0f : -1.0f;

          if (c.velocity.x != target_vel) {
            f32 a = (c.state == STATE_GROUNDED || c.state == STATE_SLIDING
                     ? target_speed / GROUND_ACCELERATION_TIME
                     : target_speed / AERIAL_ACCELERATION_TIME) * direction;

            c.velocity.x += a * (f32)delta_time;
            if (((target_vel - c.velocity.x) > 0 ? 1 : -1) != direction)
              c.velocity.x = target_vel;
          }
        }
        // trigger jump?
        if (c.cmd[CMD_JUMP] && (c.state == STATE_FLYING || mover::collides_down(c._world, c._mover) || mover::collides_sides(c._world, c._mover)))
          to_ascending_state(c);
        // trigger dash?
        if (c.cmd[CMD_DASH] && (c.state == STATE_GROUNDED || (c.state != STATE_GROUNDED && c.can_air_dash)))
          to_dash_state(c);
      }

      if (c.cmd[CMD_ATTACK] && hash::size(*c.close_targets) > 0 && c.attack_cooldown < 0)
        to_atp_or_attack_state(c);

      switch (c.state) {
      case STATE_ATTACK:
        c.state_time += delta_time;

        if (c.about_to_attack) {
          c.about_to_attack = false;
          c.attack_cooldown = ATTACK_COOLDOWN;
          if (deal_damages(c))
            c.can_air_dash = true;
        }

        c.velocity = c.attack_dir * ATTACK_ADVANCE / (f32)ATTACK_ADVANCE_DURATION;
        if (c.state_time > ATTACK_ADVANCE_DURATION) {
          c.velocity -= (f32)(c.state_time - ATTACK_ADVANCE_DURATION) * c.attack_dir * ATTACK_ADVANCE / (f32)ATTACK_ADVANCE_DURATION;
          to_flying_or_grounded_state(c);
        }
        break;
      case STATE_FLYING:
        c.state_time -= delta_time;
        c.velocity.y  = 0;
        if (c.state_time <= 0)
          to_falling_or_grounded_state(c);
        break;
      case STATE_ATP:
        tmp = p + glm::vec3(c.attack_dir * (f32)c.state_time * ATP_SPEED, 0);
        mover::set_position(c._world, c._mover, tmp);
        show_atp_ghosts(c, p, tmp);
        to_attack_state(c);
        return;
      case STATE_DASHING:
        c.state_time -= delta_time;
        c.velocity = c.attack_dir * DASH_SPEED;
        if (c.state_time <= 0 || mover::collides_sides(c._world, c._mover)) {
          to_falling_or_grounded_state(c);
          c.velocity.x = SPEED * c.orientation ? 1.0f : -1.0f;
        }
        break;
      case STATE_ASCENDING:
        c.state_time += delta_time;
        if (c.state_time >= ASCENT_DURATION)
          to_falling_or_grounded_state(c);
        else
          c.velocity.y = (ease_out_sine((f32)c.state_time, c.fly_from, ASCENT_HEIGHT, ASCENT_DURATION) - p.y) / (f32)delta_time;
        break;
      case STATE_FALLING:
        if (mover::collides_sides(c._world, c._mover))
          c.state = STATE_SLIDING;
        c.state_time += delta_time;
        c.velocity.y = c.state_time < FALL_ACCELERATION_DURATION
          ? ease_in_sine((f32)c.state_time, 0, FALL_SPEED, FALL_ACCELERATION_DURATION)
          : FALL_SPEED;
        c.velocity.y *=-1;
        if (mover::collides_down(c._world, c._mover)) {
          _instance->pinned = true;
          c.state = STATE_GROUNDED;
          c.state_time = 0;
        }
        break;
      case STATE_SLIDING:
        if (!mover::collides_sides(c._world, c._mover))
          to_falling_or_grounded_state(c);

        c.state_time += delta_time;
        c.velocity.y = c.state_time < FALL_ACCELERATION_DURATION
          ? ease_in_sine((f32)c.state_time, 0, SLIDING_SPEED, FALL_ACCELERATION_DURATION)
          : SLIDING_SPEED;
        c.velocity.y *= -1;
        break;
      case STATE_STUNNED:
        c.state_time += delta_time;
        c.velocity = c.attack_dir * STUN_RECOIL / STUN_DURATION;
        if (c.state_time > STUN_DURATION)
          to_flying_or_grounded_state(c);
        break;
      case STATE_GROUNDED:
        c.can_air_dash = true;
        c.velocity.y = 0;
        if (!c.pinned)
          to_falling_or_grounded_state(c);
        break;
      default:
        break;
      }

      glm::vec2 v = c.velocity;
      // apply slope if needed
      if (c.pinned && (c.state == STATE_GROUNDED || c.state == STATE_DASHING)) {
        v = c.ground_dir * c.velocity.x;
        if (!mover::collides_down(c._world, c._mover))
          v.y -= c.ground_dist / (f32)delta_time;
      }
      mover::move(c._world, c._mover, glm::vec3(v, 0) * (f32)delta_time);

      // update animation state
      if (c.state == STATE_GROUNDED) {
        if (c.should_move)
          unit::play_animation(c._world, c._unit, "run", 0, 0, true);
        else
          unit::play_animation(c._world, c._unit, "idle", 0, 0, true);
      } else if (!mover::collides_down(c._world, c._mover) && ! c.pinned) {
        unit::play_animation(c._world, c._unit, "jump", 0, 0, true);
      }
    }

    void add_lifetime(Clocky &c, i32 time)
    {
      c.lifetime += (f32)time * (f32)c.combo_multiplier;
      if (time > 0)
        printf("lifetime +%d (X%d)\n", time, c.combo_multiplier);
    }

    void recieve_damage(const glm::vec2 &direction, i32 damage)
    {
      _instance->lifetime  -= damage;
      printf("lifetime -%d\n", damage);
      _instance->attack_dir = direction;
      to_stun_state(*_instance);
      _instance->combo_hits = 0;
      _instance->hitted = true;
      _instance->last_hit_dir = direction;
    }

    void destroy(Clocky &c)
    {
      Allocator &a = memory_globals::default_allocator();
      MAKE_DELETE(a, Hash<Target>, c.close_targets);
      world::despawn_text(c._world, c._impact_text);
      world::despawn_unit(c._world, c._unit);
      physics::destroy_raycast(c._world, c._ground_rcast);
    }
  }
}