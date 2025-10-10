#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: No description provided
constructor_args:
  - bullet_speed: 30.0
template_args: []
required_hardware: []
depends: []
=== END MANIFEST === */
// clang-format on

#include "app_framework.hpp"
#include "libxr.hpp"
#include "timer.hpp"

class WebotsReferee : public LibXR::Application
{
 public:
  WebotsReferee(LibXR::HardwareContainer &, LibXR::ApplicationManager &app,
                float bullet_speed)
      : bullet_speed_(bullet_speed)
  {
    auto timer_handle = LibXR::Timer::CreateTask<WebotsReferee *>(
        [](WebotsReferee *self)
        { self->bullet_speed_topic_.Publish(self->bullet_speed_); }, this, 100);

    LibXR::Timer::Add(timer_handle);

    LibXR::Timer::Start(timer_handle);

    app.Register(*this);
  }

  void OnMonitor() override {}

 private:
  float bullet_speed_;
  LibXR::Topic bullet_speed_topic_ = LibXR::Topic::FindOrCreate<float>("bullet_speed");
};
