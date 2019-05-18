#pragma once

#include <runtime/types.h>

namespace game
{
  class FpsWidget
  {
  public:
    void init(pge::u64 world, const char *font, pge::i32 screen_with, pge::i32 screen_height);
    void update(pge::f64 delta_time);
    void shutdown(void);
  private:
#ifndef NDEBUG
    bool initialized;
#endif
    pge::u64 world;
    pge::u64 fps_text;
    pge::f64 elapsed_time;
    pge::u32 frame_count;
    pge::u32 last_frame_count;
    pge::u32 text_spacing;
    pge::i32 screen_with;
    pge::i32 screen_height;
  };
}