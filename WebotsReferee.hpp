#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: Webots 裁判摘要模拟模块：周期发布 robot_game_ref 摘要并同步发射机构状态 / Webots referee summary simulation Module that publishes the robot_game_ref summary periodically and mirrors the launcher state
depends:
- id: QDU-Robomaster/Referee
  ref: same-or-dev
=== END MANIFEST === */
// clang-format on

#include <algorithm>
#include <cmath>

#include "WebotsRefereeTypes.hpp"
#include "libxr.hpp"
#include "timebase.hpp"
#include "timer.hpp"

/**
 * @brief Webots 裁判摘要模拟器。
 *        Webots referee summary simulator.
 *
 * @details 发布与 MCU `robot_game_ref` 摘要包布局一致的数据。发射机构状态由
 *          `WebotsFireNotify` 通过 `webots_launcher/state` 和
 *          `webots_launcher/shot_event` 同步。
 *          Publishes data with the same layout as the MCU `robot_game_ref` summary
 *          packet. The launcher state is synchronized from `WebotsFireNotify` through
 *          `webots_launcher/state` and `webots_launcher/shot_event`.
 */
class WebotsReferee
{
 public:
  /**
   * @brief 构造参数。
   *        Construction parameters.
   */
  struct Param
  {
    float bullet_speed;  ///< 初始弹速，单位 m/s
    ///< Initial bullet speed in m/s
    float shooter_heat_limit;  ///< 初始热量上限
    ///< Initial heat limit
    float shooter_cooling_value;  ///< 初始每秒冷却值
    ///< Initial cooling value per second
    uint8_t robot_id;  ///< 机器人 ID
    ///< Robot ID
    uint8_t robot_level;  ///< 机器人等级
    ///< Robot level
    uint16_t max_hp;  ///< 最大血量和当前血量初值
    ///< Maximum HP and initial current HP
    uint16_t chassis_power_limit;  ///< 底盘功率上限，单位 W
    ///< Chassis power limit in W
    int publish_period_ms;  ///< 发布周期，单位 ms，最小按 1 执行
    ///< Publish period in ms, at least 1 is used
    const char* referee_robot_game_tp_name;  ///< host 域内裁判摘要 Topic 名称
    ///< Name of the referee summary Topic in the host domain
  };

  /**
   * @brief 构造 Webots 裁判摘要模拟器：创建摘要与发射机构 Topic，
   *        注册回调并启动周期发布任务。
   *        Construct the Webots referee summary simulator: create the summary and
   *        launcher Topics, register the callbacks and start the periodic publishing
   *        task.
   *
   * @param param 构造参数。
   *              Construction parameters.
   */
  WebotsReferee(const Param& param = {.bullet_speed = 30.0f,
                                      .shooter_heat_limit = 240.0f,
                                      .shooter_cooling_value = 40.0f,
                                      .robot_id = 7,
                                      .robot_level = 1,
                                      .max_hp = 200,
                                      .chassis_power_limit = 45,
                                      .publish_period_ms = 100,
                                      .referee_robot_game_tp_name = "robot_game_ref"})
      : referee_domain_("host"),
        robot_game_referee_topic_(
            LibXR::Topic::CreateTopic<WebotsRefereeTypes::RobotGameRefereeSummary>(
                param.referee_robot_game_tp_name, &referee_domain_, true)),
        launcher_domain_("webots_launcher"),
        launcher_state_topic_(
            LibXR::Topic::CreateTopic<WebotsRefereeTypes::WebotsLauncherState>(
                "state", &launcher_domain_, true)),
        launcher_shot_event_topic_(
            LibXR::Topic::CreateTopic<WebotsRefereeTypes::WebotsLauncherShotEvent>(
                "shot_event", &launcher_domain_, true))
  {
    state_.bullet_speed = param.bullet_speed;
    state_.heat_limit = param.shooter_heat_limit;
    state_.cooling_rate = param.shooter_cooling_value;
    state_.single_shot_heat = 10.0f;
    state_.max_fire_frequency_hz = 20.0f;
    state_.fire_delay_s = 0.03f;
    state_.min_fire_interval_s = 0.05f;
    state_.launcher_enabled = 1;

    summary_ = {};
    summary_.robot_status.robot_id = param.robot_id;
    summary_.robot_status.robot_level = param.robot_level;
    summary_.robot_status.remain_hp = param.max_hp;
    summary_.robot_status.max_hp = param.max_hp;
    summary_.robot_status.shooter_cooling_value = ClampToUint16(param.shooter_cooling_value);
    summary_.robot_status.shooter_heat_limit = ClampToUint16(param.shooter_heat_limit);
    summary_.robot_status.chassis_power_limit = param.chassis_power_limit;
    summary_.robot_status.power_gimbal_output = 1;
    summary_.robot_status.power_chassis_output = 1;
    summary_.robot_status.power_launcher_output = 1;

    auto launcher_state_cb = LibXR::Topic::Callback::Create(
        [](bool, WebotsReferee* self, const LibXR::ConstRawData& data)
        {
          auto* state = reinterpret_cast<const WebotsRefereeTypes::WebotsLauncherState*>(
              data.addr_);
          if (state != nullptr &&
              data.size_ == sizeof(WebotsRefereeTypes::WebotsLauncherState))
          {
            LibXR::Mutex::LockGuard lock(self->state_mutex_);
            self->state_ = *state;
            self->have_launcher_state_ = true;
            self->UpdateHeatLocked(state->current_heat, state->update_time_us);
          }
        },
        this);
    launcher_state_topic_.RegisterCallback(launcher_state_cb);

    auto launcher_shot_event_cb = LibXR::Topic::Callback::Create(
        [](bool, WebotsReferee* self, const LibXR::ConstRawData& data)
        {
          auto* event =
              reinterpret_cast<const WebotsRefereeTypes::WebotsLauncherShotEvent*>(
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
            self->UpdateHeatLocked(event->heat_after, event->fire_time_us);

            auto& launcher = self->summary_.launcher_data;
            launcher.bullet_type = BULLET_TYPE_17MM;
            launcher.launcherer_id = LAUNCHER_ID_17MM;
            launcher.bullet_freq = event->shot_interval_us > 0
                                       ? ClampToUint8(1000000.0f / static_cast<float>(
                                                                       event->shot_interval_us))
                                       : 0;
            launcher.bullet_speed = event->bullet_speed;
            ++self->summary_.shot_seq;
          }
        },
        this);
    launcher_shot_event_topic_.RegisterCallback(launcher_shot_event_cb);

    auto timer_handle = LibXR::Timer::CreateTask<WebotsReferee*>(
        [](WebotsReferee* self) { self->PublishSummary(); }, this,
        static_cast<uint32_t>(std::max(1, param.publish_period_ms)));

    LibXR::Timer::Add(timer_handle);

    LibXR::Timer::Start(timer_handle);
  }

 private:
  /**
   * @brief 周期发布裁判摘要。
   *        Publish the referee summary periodically.
   */
  void PublishSummary()
  {
    WebotsRefereeTypes::RobotGameRefereeSummary summary{};

    {
      LibXR::Mutex::LockGuard lock(state_mutex_);
      const auto now_us = static_cast<uint64_t>(LibXR::Timebase::GetMicroseconds());
      summary_.game_status.sync_time_stamp = now_us;
      if (have_launcher_state_)
      {
        summary_.robot_status.shooter_cooling_value = ClampToUint16(state_.cooling_rate);
        summary_.robot_status.shooter_heat_limit = ClampToUint16(state_.heat_limit);
        summary_.robot_status.power_launcher_output = state_.launcher_enabled ? 1 : 0;
      }
      if (have_heat_)
      {
        const float elapsed_s =
            now_us > heat_time_us_ ? static_cast<float>(now_us - heat_time_us_) * 1e-6f
                                   : 0.0f;
        const float heat = std::max(0.0f, heat_ - std::max(0.0f, state_.cooling_rate) *
                                                      elapsed_s);
        summary_.launcher_17_heat = ClampToUint16(std::round(heat));
      }

      summary = summary_;
    }

    robot_game_referee_topic_.Publish(summary);
  }

  /**
   * @brief 把浮点值钳位到裁判包使用的 `uint16_t`。
   *        Clamp a floating-point value to the `uint16_t` used by the referee packet.
   *
   * @param value 输入值，非有限值和非正值按 0 处理。
   *              Input value; non-finite and non-positive values are treated as 0.
   * @return 钳位后的值。
   *         The clamped value.
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
   * @brief 把浮点值四舍五入并钳位到 `uint8_t`。
   *        Round a floating-point value and clamp it to `uint8_t`.
   *
   * @param value 输入值，非有限值和非正值按 0 处理。
   *              Input value; non-finite and non-positive values are treated as 0.
   * @return 钳位后的值。
   *         The clamped value.
   */
  static uint8_t ClampToUint8(float value)
  {
    if (!std::isfinite(value) || value <= 0.0f)
    {
      return 0;
    }

    return static_cast<uint8_t>(std::min(std::round(value), 255.0f));
  }

  /**
   * @brief 记录一个热量样本，只保留时间最新的样本；调用方持有 `state_mutex_`。
   *        Record a heat sample and keep only the newest one; the caller holds
   *        `state_mutex_`.
   *
   * @param heat 热量。
   *             Heat.
   * @param time_us 样本时间，单位 us。
   *                Sample time in us.
   */
  void UpdateHeatLocked(float heat, uint64_t time_us)
  {
    if (!std::isfinite(heat) || (have_heat_ && time_us < heat_time_us_))
    {
      return;
    }
    heat_ = std::max(0.0f, heat);
    heat_time_us_ = time_us;
    have_heat_ = true;
  }

  /** @brief 0x0207 中 17 mm 弹丸的弹丸类型值。 */
  static constexpr uint8_t BULLET_TYPE_17MM = 1;

  /** @brief 0x0207 中 17 mm 发射机构的发射机构 ID。 */
  static constexpr uint8_t LAUNCHER_ID_17MM = 1;

  /** @brief 最近一次热量样本。 */
  float heat_{0.0f};

  /** @brief 最近一次热量样本的时间，单位 us。 */
  uint64_t heat_time_us_{0};

  /** @brief 是否已有热量样本。 */
  bool have_heat_{false};

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
