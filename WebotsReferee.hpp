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

#include <cstdint>

#include "app_framework.hpp"
#include "libxr.hpp"
#include "timer.hpp"

struct [[gnu::packed]] WebotsRefereeRobotStatus
{
  uint8_t robot_id{};
  uint8_t robot_level{};
  uint16_t remain_hp{};
  uint16_t max_hp{};
  uint16_t shooter_cooling_value{};
  uint16_t shooter_heat_limit{};
  uint16_t chassis_power_limit{};
  uint8_t power_gimbal_output : 1 {};
  uint8_t power_chassis_output : 1 {};
  uint8_t power_launcher_output : 1 {};
};

struct [[gnu::packed]] WebotsRefereeGameStatus
{
  uint8_t game_type : 4 {};
  uint8_t game_progress : 4 {};
  uint16_t stage_remain_time{};
  uint64_t sync_time_stamp{};
};

struct [[gnu::packed]] WebotsRefereeLauncherData
{
  uint8_t bullet_type{};
  uint8_t launcher_id{};
  uint8_t bullet_freq{};
  float bullet_speed{};
};

struct [[gnu::packed]] WebotsRefereeSummary
{
  WebotsRefereeRobotStatus robot_status{};
  WebotsRefereeGameStatus game_status{};
  WebotsRefereeLauncherData launcher_data{};
};

static_assert(sizeof(WebotsRefereeRobotStatus) == 13);
static_assert(sizeof(WebotsRefereeGameStatus) == 11);
static_assert(sizeof(WebotsRefereeLauncherData) == 7);
static_assert(sizeof(WebotsRefereeSummary) == 31);

class WebotsReferee : public LibXR::Application
{
 public:
  WebotsReferee(LibXR::HardwareContainer &, LibXR::ApplicationManager &app,
                float bullet_speed)
  {
    referee_msg_.launcher_data.bullet_speed = bullet_speed;
    auto timer_handle = LibXR::Timer::CreateTask<WebotsReferee *>(
        [](WebotsReferee *self)
        { self->robot_game_referee_topic_.Publish(self->referee_msg_); }, this, 100);

    LibXR::Timer::Add(timer_handle);

    LibXR::Timer::Start(timer_handle);

    app.Register(*this);
  }

  void OnMonitor() override {}

 private:
  LibXR::Topic::Domain host_domain_ = LibXR::Topic::Domain("host");
  WebotsRefereeSummary referee_msg_{};
  LibXR::Topic robot_game_referee_topic_ =
      LibXR::Topic::FindOrCreate<WebotsRefereeSummary>("robot_game_ref",
                                                       &host_domain_);
};
