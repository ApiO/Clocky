#include <stdio.h>
#include <engine/pge.h>
#include <runtime/assert.h>
#include "fps_widget.h"

using namespace pge;

namespace game
{
  void FpsWidget::init(pge::u64 w, const char *font, pge::i32 width, pge::i32 h)
  {
#ifndef NDEBUG
    initialized = true;
#endif

    world = w;
    fps_text = 0;
    text_spacing = 2u;
    screen_with = width;
    screen_height = h;

    fps_text = world::spawn_text(world, font, NULL, TEXT_ALIGN_LEFT, glm::vec3(0.f), glm::quat(1, 0, 0, 0));
    frame_count = 59;
    elapsed_time = 1.f;
    last_frame_count = 0u;
    update(0.0166666f);
  }


  void FpsWidget::update(pge::f64 delta_time)
  {
    ASSERT(initialized);

    frame_count++;
    elapsed_time += delta_time;

    if (elapsed_time < 1.f) return;

    elapsed_time = 0.f;

    if (last_frame_count == frame_count) {
      frame_count = 0u;
      return;
    }

    char value[256];
    sprintf(value, "%d fps", frame_count);
    text::set_string(world, fps_text, value);

    f32 width;
    text::get_width(world, fps_text, width);

    f32 x = screen_with*.5f - width - text_spacing;
    f32 y = screen_height*.5f - text_spacing;

    text::set_local_position(world, fps_text, glm::vec3(x, y, 0.f));

    last_frame_count = frame_count;
    frame_count = 0u;
  }

  void FpsWidget::shutdown(void)
  {
    world::despawn_text(world, fps_text);
#ifndef NDEBUG
    initialized = false;
#endif
  }
}