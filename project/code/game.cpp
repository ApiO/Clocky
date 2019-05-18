#include <engine/pge.h>
#include <engine/pge_types.h>

#include <stdio.h>
#include <runtime/memory.h>
#include <runtime/hash.h>
#include <runtime/temp_allocator.h>
#include <game_camera.h>
#include <time.h>

//#include "aristobot_controller.h"
#include "enemy_controller.h"

#include "units/clocky.h"
#include "camera.h"
#include "game_types.h"
#include "fps_widget.h"
#include "noise.h"

namespace
{
  using namespace pge;
  using namespace game;

  u64    _game_world;
  GameCamera _game_camera;
  u64    _hud_world;
  u64    _back_world;
  u64    _back_sprite;
  Camera _hud_camera;
  Camera _back_camera;
  u64    _viewport;
  u64    _level;
  u64    _halo;
  Noise  _noise_fx;

  u64    _lifetime_text;
  i32    _last_lifetime;
  u64    _combo_text;
  i32    _last_combo;
  char   _str_buf[256];

  i32    screen_width;
  i32    screen_height;
  u64    game_pack;
  FpsWidget fps_widget;
  EnemyController enemies;
  Array<u64>      *fx;
  const char *FONT_NAME = "fonts/consolas.24/consolas.24";

  Clocky player;

  const u32 PAD_INDEX = 0;
  const f32 CAMERA_DAMPING = 4.0f;
  const glm::vec2 virtual_resolution(1280, 720);

  const glm::vec3 test_path2 [2] = { {0, 0, 0}, {100,600,0} };
  const glm::vec3 test_path [8] = { {0,0,0}, {100,50,0}, {0,100,0}, {100,150,0}, {0,200,0}, {100,250,0}, {0,300,0}, {100,600,0}};
  const f32       curve_time[5] = { 0, 3, 4, 5, 10 };
  const f32       curve_val [5] = { 0, 0.5f, 0, 1, 0};

  bool show_debug = true;

  /*
  // DEBUG monster
  template<typename T>
  inline void debug_draw_monsters_direction(Hash<T> &enemies)
  {
    Hash<T>::Entry *entry, *end = hash::end(enemies);
    glm::vec3 a, b;
    for (entry = hash::begin(enemies); entry < end; entry++) {
      unit::get_world_position(_game_world, entry->key, 0, a);
      a.y += 40;
      b = a;
      b.x += (entry->value.look_right ? 1 : -1) * 50;
      application::render_line(a, b, Color(0, 255, 255, 255),
                               _game_world, _game_camera.get_id(), _viewport);
    }
  }
  */
}

namespace game
{
  using namespace pge;

  void init()
  {
    Allocator &a = memory_globals::default_allocator();
    window::get_resolution(screen_width, screen_height);

    _game_world = application::create_world();
    _hud_world = application::create_world();
    _back_world = application::create_world();
    fx = MAKE_NEW(a, Array<u64>, a);

    {
      f32 target_aspect = virtual_resolution.x / virtual_resolution.y;
      i32 width = screen_width ;
      i32 height = (i32)(width / target_aspect + 0.5f);

      if (height > screen_height) {
         //It doesn't fit our height, we must switch to pillarbox then
          height = screen_height;
          width = (int)(height * target_aspect + 0.5f);
      }
      _viewport = application::create_viewport((screen_width/2) - (width/2), (screen_height/2) - (height/2), width, height);
    }
    
    game_pack = application::resource_package("packages/default");
    resource_package::load(game_pack);
    resource_package::flush(game_pack);

    game_camera::init(_game_camera, _game_world, virtual_resolution, a);
    //game_camera::follow_path(_game_camera, test_path, 8, 5);
    //game_camera::clamp_to_box(_game_camera, glm::vec2(-screen_width, -screen_height), glm::vec2(screen_width, screen_height));
    //game_camera::follow_path(_game_camera, test_path, 8, curve_time, curve_val, 5);
    //game_camera::clamp_to_path(_game_camera, test_path, 8);

    _hud_camera.init(_hud_world, CAMERA_MODE_OTRHO, (f32)virtual_resolution.x, (f32)virtual_resolution.y);
    _hud_camera.set_orthographic_projection(f32(-virtual_resolution.x*.5f), f32(virtual_resolution.x*.5f), f32(-virtual_resolution.y*.5f), f32(virtual_resolution.y*.5f));
    _hud_camera.set_near_range(-1.0f);
    _hud_camera.set_far_range(1.0f);

    _back_camera.init(_back_world, CAMERA_MODE_OTRHO, (f32)virtual_resolution.x, (f32)virtual_resolution.y);
    _back_camera.set_orthographic_projection(f32(-virtual_resolution.x*.5f), f32(virtual_resolution.x*.5f), f32(-virtual_resolution.y*.5f), f32(virtual_resolution.y*.5f));
    _back_camera.set_near_range(-1.0f);
    _back_camera.set_far_range(1.0f);

    _level = world::load_level(_game_world, "levels/maquette", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
    //_level = world::load_level(_game_world, "levels/default/default", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
    //aristobot_controller::init(_game_world, a);

    clocky::spawn(player, _game_world, glm::vec2(215, 100), 30, enemies, *fx);

    { // background initialization
      world::spawn_box(_back_world, screen_width, screen_height, Color(32, 72, 44, 255), Color(65, 146, 90, 255), glm::vec3(0, 0, 0), IDENTITY_ROTATION, true);
      glm::vec2 size;
      f32 scale;
      _back_sprite = world::spawn_sprite(_back_world, "backgrounds/normal_bg", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      sprite::get_size(_back_world, _back_sprite, size);
      scale = ((screen_width - size.x) < (size.y - screen_height)) ? (f32)screen_width / size.x : (f32)screen_height / size.y;
      sprite::set_local_scale(_back_world, _back_sprite, glm::vec3(scale, scale, 1));
    }
    
    noise::init(_noise_fx, _hud_world, glm::vec2(screen_width, screen_height), 1.0/30, a);

    { // front halo initialization
      glm::vec2 size;
      _halo = world::spawn_sprite(_hud_world, "fx/front_decal", glm::vec3(0, 0, -1), IDENTITY_ROTATION, glm::vec3(4, 4, 1));
      sprite::get_size(_hud_world, _halo, size);
      size.x = (f32)(screen_width  + 4) / size.x;
      size.y = (f32)(screen_height + 4) / size.y;
      sprite::set_local_scale(_hud_world, _halo, glm::vec3(size.x, size.y, 1));
    } 

    _lifetime_text = world::spawn_text(_hud_world, FONT_NAME, NULL, TEXT_ALIGN_LEFT, glm::vec3(-virtual_resolution.x / 2, virtual_resolution.y / 2, 0), glm::quat(1, 0, 0, 0));
    _combo_text = world::spawn_text(_hud_world, FONT_NAME, NULL, TEXT_ALIGN_LEFT, glm::vec3(-virtual_resolution.x / 2, virtual_resolution.y / 2 - 24, 0), glm::quat(1, 0, 0, 0));


    _last_lifetime = -1;
    _last_combo = -1;

    enemy_controller::init(enemies, _game_world, player._unit, a);

    physics::show_debug(show_debug);
    srand((u32)time(NULL));

    fps_widget.init(_hud_world, FONT_NAME, virtual_resolution.x, virtual_resolution.y);
  }

  void update(f64 delta_time)
  {
    fps_widget.update(delta_time);
    noise::update(_noise_fx, delta_time);

    if (keyboard::any_pressed()) {
      if (keyboard::pressed(KEYBOARD_KEY_F5)) {
        Allocator &a = memory_globals::default_allocator();
        enemy_controller::shutdown(enemies);
        clocky::destroy(player);
        world::unload_level(_game_world, _level);
        //_level = world::load_level(_game_world, "levels/default/default", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
        _level = world::load_level(_game_world, "levels/maquette", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
        clocky::spawn(player, _game_world, glm::vec2(215, 100), 30, enemies, *fx);
        enemy_controller::init(enemies, _game_world, player._unit, a);
      }

      if (keyboard::pressed(KEYBOARD_KEY_P)) {
        show_debug = !show_debug;
        physics::show_debug(show_debug);
      }
    }

    if (player.hitted)
      game_camera::shake(_game_camera, player.last_hit_dir, 6, 16, 0.33, 12, 8);

    clocky::update(player, delta_time);
    clocky::add_lifetime(player, enemy_controller::lifetime_gain(enemies));
    enemy_controller::update(enemies, delta_time);
    //aristobot_controller::update(delta_time, player);

    world::update(_game_world, delta_time);

    // sync camera
    glm::vec3 p;
    unit::get_world_position(_game_world, player._unit, 0, p);
    p.y += 40.0f;

    game_camera::set_target(_game_camera, (glm::vec2)p);
    game_camera::update(_game_camera, delta_time);

    _hud_camera.update();
    _back_camera.update();

    if (_last_lifetime != (i32)player.lifetime) {
      _last_lifetime = (i32)player.lifetime;
      sprintf(_str_buf, "HP : %i", _last_lifetime);
      text::set_string(_hud_world, _lifetime_text, _str_buf);
    }

    if (_last_combo != player.combo_hits) {
      glm::vec2 ts;
      _last_combo = player.combo_hits;
      sprintf(_str_buf, "Combo Hits: %d\nCombo Buff: X%d", _last_combo, player.combo_multiplier);
      text::set_string(_hud_world, _combo_text, _str_buf);
    }

    for (u32 i = 0; i < array::size(*fx); i++) {
      while (i < array::size(*fx) && !unit::is_playing_animation(_game_world, (*fx)[i])) {
        world::despawn_unit(_game_world, (*fx)[i]);
        (*fx)[i] = array::pop_back(*fx);
      }
        
    }
    
    world::update(_game_world, 0.0f); // sync the camera :(
    world::update(_hud_world, delta_time);
    world::update(_back_world, delta_time);

    if (keyboard::pressed(KEYBOARD_KEY_ESCAPE))
      application::quit();
  }

  void render()
  {
    application::render_world(_back_world, _back_camera.get_id(), _viewport);
    application::render_world(_game_world, _game_camera.id, _viewport);

    glm::vec3 p;
    // draw the player aiming dir
    /*
    glm::vec3 p;
    unit::get_world_position(_game_world, player._unit, 0, p);
    p.y += 40;
    application::render_line(p, p + glm::vec3(player.aiming_dir, 0) * (f32)clocky::ATTACK_RANGE, Color(255, 255, 255, 255), //ATTACK_RANGE à virer des const public quand plus utilisé pour debug
                             _game_world, _game_camera.get_id(), _viewport);
    */

    // draws enemyorientation
    /*
    debug_draw_monsters_direction(*enemies.walkers);
    debug_draw_monsters_direction(*enemies.flybots);
    debug_draw_monsters_direction(*enemies.spikejonz);
    //*/

    // draw a marker on the target of the dash
    if (player.best_target) {
      const Target &t = *hash::get(*player.close_targets, player.best_target);
      p = glm::vec3(t.position, 0);
      p.y += 40;

      application::render_circle(p, 40, Color(255, 0, 0, 255),
        _game_world, _game_camera.id, _viewport, false);

      f32 width = 50;

      application::render_line(glm::vec3(p.x - width, p.y, 0), glm::vec3(p.x + width, p.y, 0), Color(255, 0, 0, 255),
                                 _game_world, _game_camera.id, _viewport);

      application::render_line(glm::vec3(p.x, p.y - width, 0), glm::vec3(p.x, p.y + width, 0), Color(255, 0, 0, 255),
        _game_world, _game_camera.id, _viewport);

    }

    application::render_world(_hud_world, _hud_camera.get_id(), _viewport);
  }

  void shutdown()
  {
    for (u32 i = 0; i < array::size(*fx); i++)
      world::despawn_unit(_game_world, (*fx)[i]);

    MAKE_DELETE(memory_globals::default_allocator(), Array<u64>, fx);

    fps_widget.shutdown();
    noise::free(_noise_fx);
    world::despawn_sprite(_hud_world, _halo);
    world::despawn_text(_hud_world, _lifetime_text);
    world::despawn_text(_hud_world, _combo_text);

    enemy_controller::shutdown(enemies);
    //aristobot_controller::shutdown();

    clocky::destroy(player);
    world::unload_level(_game_world, _level);
    
    application::destroy_viewport(_viewport);
    game_camera::destroy(_game_camera);
    application::destroy_world(_back_world);
    application::destroy_world(_game_world);
    application::destroy_world(_hud_world);

    resource_package::unload(game_pack);
    application::release_resource_package(game_pack);
  }
}
