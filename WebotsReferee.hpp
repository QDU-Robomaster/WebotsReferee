#pragma once

/**
 * @file WebotsReferee.hpp
 * @brief Webots 裁判摘要模拟模块。
 */

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: Webots referee simulator that mirrors launcher state into robot_game_ref
constructor_args:
  - bullet_speed: 30.0
  - shooter_heat_limit: 240.0
  - shooter_cooling_value: 40.0
  - robot_id: 7
  - robot_level: 1
  - max_hp: 200
  - chassis_power_limit: 45
  - publish_period_ms: 100
template_args: []
required_hardware: []
depends: []
=== END MANIFEST === */
// clang-format on

#include <algorithm>
#include <cmath>

#include "WebotsRefereeTypes.hpp"
#include "app_framework.hpp"
#include "libxr.hpp"
#include "timebase.hpp"
#include "timer.hpp"

/**
 * @brief Webots 裁判摘要模拟器。
 *
 * 模块发布与 MCU `robot_game_ref` 摘要包布局一致的数据。发射机构状态由
 * `WebotsFireNotify` 通过 `webots_launcher/state` 和
 * `webots_launcher/shot_event` 同步。
 */
class WebotsReferee : public LibXR::Application
{
 public:
  /**
   * @brief 构造 Webots 裁判摘要模拟器。
   *
   * @param bullet_speed 默认弹速，单位 m/s。
   * @param shooter_heat_limit 热量上限。
   * @param shooter_cooling_value 每秒冷却值。
   * @param robot_id 机器人 ID。
   * @param robot_level 机器人等级。
   * @param max_hp 最大血量和初始当前血量。
   * @param chassis_power_limit 底盘功率上限。
   * @param publish_period_ms 发布周期，单位 ms。
   */
  WebotsReferee(LibXR::HardwareContainer &, LibXR::ApplicationManager &app,
                float bullet_speed, float shooter_heat_limit = 240.0f,
                float shooter_cooling_value = 40.0f, uint8_t robot_id = 7,
                uint8_t robot_level = 1, uint16_t max_hp = 200,
                uint16_t chassis_power_limit = 45,
                int publish_period_ms = 100)
      : referee_domain_("host"),
        robot_game_referee_topic_(
            LibXR::Topic::CreateTopic<WebotsRefereeTypes::RobotGameRefereeSummary>(
                "robot_game_ref", &referee_domain_, true)),
        launcher_domain_("webots_launcher"),
        launcher_state_topic_(
            LibXR::Topic::CreateTopic<WebotsRefereeTypes::WebotsLauncherState>(
                "state", &launcher_domain_, true)),
        launcher_shot_event_topic_(
            LibXR::Topic::CreateTopic<WebotsRefereeTypes::WebotsLauncherShotEvent>(
                "shot_event", &launcher_domain_, true))
  {
    state_.bullet_speed = bullet_speed;
    state_.heat_limit = shooter_heat_limit;
    state_.cooling_rate = shooter_cooling_value;
    state_.single_shot_heat = 10.0f;
    state_.max_fire_frequency_hz = 20.0f;
    state_.fire_delay_s = 0.03f;
    state_.min_fire_interval_s = 0.05f;
    state_.launcher_enabled = 1;

    summary_ = {};
    summary_.robot_status.robot_id = robot_id;
    summary_.robot_status.robot_level = robot_level;
    summary_.robot_status.remain_hp = max_hp;
    summary_.robot_status.max_hp = max_hp;
    summary_.robot_status.shooter_cooling_value =
        ClampToUint16(shooter_cooling_value);
    summary_.robot_status.shooter_heat_limit =
        ClampToUint16(shooter_heat_limit);
    summary_.robot_status.chassis_power_limit = chassis_power_limit;
    summary_.robot_status.power_gimbal_output = 1;
    summary_.robot_status.power_chassis_output = 1;
    summary_.robot_status.power_launcher_output = 1;
    summary_.launcher_data.bullet_type = 1;
    summary_.launcher_data.launcher_id = 1;
    summary_.launcher_data.bullet_speed = bullet_speed;

    auto launcher_state_cb = LibXR::Topic::Callback::Create(
        [](bool, WebotsReferee *self, LibXR::RawData &data)
        {
          auto *state = reinterpret_cast<WebotsRefereeTypes::WebotsLauncherState *>(
              data.addr_);
          if (state != nullptr &&
              data.size_ == sizeof(WebotsRefereeTypes::WebotsLauncherState))
          {
            LibXR::Mutex::LockGuard lock(self->state_mutex_);
            self->state_ = *state;
            self->have_launcher_state_ = true;
          }
        },
        this);
    launcher_state_topic_.RegisterCallback(launcher_state_cb);

    auto launcher_shot_event_cb = LibXR::Topic::Callback::Create(
        [](bool, WebotsReferee *self, LibXR::RawData &data)
        {
          auto *event =
              reinterpret_cast<WebotsRefereeTypes::WebotsLauncherShotEvent *>(
                  data.addr_);
          if (event != nullptr &&
              data.size_ == sizeof(WebotsRefereeTypes::WebotsLauncherShotEvent))
          {
            LibXR::Mutex::LockGuard lock(self->state_mutex_);
            self->state_.last_fire_time_us = event->fire_time_us;
            self->state_.shot_count = event->shot_id;
            self->state_.current_heat = event->heat_after;
            self->state_.heat_limit = event->heat_limit;
            self->state_.cooling_rate = event->cooling_rate;
            self->state_.single_shot_heat = event->single_shot_heat;
            self->state_.bullet_speed = event->bullet_speed;
            self->state_.fire_delay_s = event->fire_delay_s;
            self->state_.min_fire_interval_s = event->min_fire_interval_s;
            self->state_.pending_fire = 0;
            self->state_.pending_fire_time_us = 0;
            if (event->shot_interval_us > 0)
            {
              self->state_.current_fire_frequency_hz =
                  1000000.0f / static_cast<float>(event->shot_interval_us);
            }
            self->have_launcher_state_ = true;
          }
        },
        this);
    launcher_shot_event_topic_.RegisterCallback(launcher_shot_event_cb);

    auto timer_handle = LibXR::Timer::CreateTask<WebotsReferee *>(
        [](WebotsReferee *self)
        { self->PublishSummary(); }, this,
        static_cast<uint32_t>(std::max(1, publish_period_ms)));

    LibXR::Timer::Add(timer_handle);

    LibXR::Timer::Start(timer_handle);

    app.Register(*this);
  }

  void OnMonitor() override {}

 private:
  /**
   * @brief 周期发布裁判摘要。
   */
  void PublishSummary()
  {
    WebotsRefereeTypes::RobotGameRefereeSummary summary{};

    {
      LibXR::Mutex::LockGuard lock(state_mutex_);
      summary_.game_status.sync_time_stamp =
          static_cast<uint64_t>(LibXR::Timebase::GetMicroseconds());
      if (have_launcher_state_)
      {
        summary_.robot_status.shooter_cooling_value =
            ClampToUint16(state_.cooling_rate);
        summary_.robot_status.shooter_heat_limit =
            ClampToUint16(state_.heat_limit);
        summary_.robot_status.power_launcher_output = state_.launcher_enabled ? 1 : 0;
        summary_.launcher_data.bullet_speed = state_.bullet_speed;
        summary_.launcher_data.bullet_freq =
            ClampToUint8Rounded(state_.current_fire_frequency_hz);
      }

      summary = summary_;
    }

    robot_game_referee_topic_.Publish(summary);
  }

  /**
   * @brief 将浮点配置钳位到裁判包使用的 uint16_t。
   */
  static uint16_t ClampToUint16(float value)
  {
    if (!std::isfinite(value) || value <= 0.0f)
    {
      return 0;
    }

    return static_cast<uint16_t>(std::min(value, 65535.0f));
  }

  /**
   * @brief 将浮点射频四舍五入后钳位到 uint8_t。
   */
  static uint8_t ClampToUint8Rounded(float value)
  {
    if (!std::isfinite(value) || value <= 0.0f)
    {
      return 0;
    }

    return static_cast<uint8_t>(std::clamp(std::lround(value), 0L, 255L));
  }

  /** @brief 最近一次发布的裁判摘要。 */
  WebotsRefereeTypes::RobotGameRefereeSummary summary_{};

  /** @brief 最近一次收到的发射机构状态。 */
  WebotsRefereeTypes::WebotsLauncherState state_{};

  /** @brief 是否已经收到过发射机构状态。 */
  bool have_launcher_state_{false};

  /** @brief 保护发射机构状态和裁判摘要缓存。 */
  LibXR::Mutex state_mutex_;

  /** @brief 裁判摘要输出域。 */
  LibXR::Topic::Domain referee_domain_;

  /** @brief MCU 裁判摘要 topic。 */
  LibXR::Topic robot_game_referee_topic_;

  /** @brief Webots 发射机构内部状态域。 */
  LibXR::Topic::Domain launcher_domain_;

  /** @brief 发射机构状态 topic。 */
  LibXR::Topic launcher_state_topic_;

  /** @brief 真实出弹事件 topic。 */
  LibXR::Topic launcher_shot_event_topic_;
};
