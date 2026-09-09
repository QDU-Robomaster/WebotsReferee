#pragma once

/**
 * @file WebotsRefereeTypes.hpp
 * @brief Webots 裁判与发射机构共享的数据结构。
 */

#include <cstdint>
#include "RefereeTypes.hpp"

namespace WebotsRefereeTypes
{
/**
 * @brief Webots 发射请求的最近一次拒绝原因。
 */
enum class WebotsLauncherRejectReason : uint8_t
{
  NONE = 0,        ///< 最近一次请求未被拒绝。
  DISABLED = 1,    ///< 发射机构未上电或被禁用。
  PENDING = 2,     ///< 上一发仍在延迟队列中。
  RATE_LIMIT = 3,  ///< 请求触发射频限制。
  HEAT_LIMIT = 4,  ///< 请求会使热量达到或超过上限。
};

/** @brief 与 Aimer/实机 Referee 使用完全相同的类型，不在仿真端复制协议布局。 */
using RobotGameRefereeStatus = RefereeTypes::RobotStatus;
using RobotGameRefereeGame = RefereeTypes::GameStatus;
using RobotGameRefereeSummary = RefereeTypes::RobotGameRefereePack;

/**
 * @brief Webots 发射机构当前状态快照。
 */
struct WebotsLauncherState
{
  uint64_t update_time_us{0};
  uint64_t last_request_time_us{0};
  uint64_t last_fire_time_us{0};
  uint64_t next_fire_request_us{0};
  uint64_t pending_fire_time_us{0};
  uint64_t shot_count{0};
  float current_heat{0.0f};
  float heat_limit{0.0f};
  float cooling_rate{0.0f};
  float single_shot_heat{0.0f};
  float bullet_speed{0.0f};
  float max_fire_frequency_hz{0.0f};
  float fire_delay_s{0.0f};
  float min_fire_interval_s{0.0f};
  float current_fire_frequency_hz{0.0f};
  uint8_t launcher_enabled{1};
  uint8_t can_fire{0};
  uint8_t pending_fire{0};
  uint8_t last_reject_reason{0};
};

/**
 * @brief Webots 发射机构真实发弹事件。
 */
struct WebotsLauncherShotEvent
{
  uint64_t shot_id{0};
  uint64_t request_time_us{0};
  uint64_t fire_time_us{0};
  uint64_t shot_interval_us{0};
  float bullet_speed{0.0f};
  float heat_before{0.0f};
  float heat_after{0.0f};
  float heat_limit{0.0f};
  float cooling_rate{0.0f};
  float single_shot_heat{0.0f};
  float fire_delay_s{0.0f};
  float min_fire_interval_s{0.0f};
};
}  // namespace WebotsRefereeTypes
