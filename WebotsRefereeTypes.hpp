#pragma once

/**
 * @file WebotsRefereeTypes.hpp
 * @brief Webots 裁判与发射机构共享的数据结构。
 */

#include <cstdint>

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

/**
 * @brief 与 MCU 裁判摘要包布局一致的机器人状态块。
 */
struct [[gnu::packed]] RobotGameRefereeStatus
{
  uint8_t robot_id;
  uint8_t robot_level;
  uint16_t remain_hp;
  uint16_t max_hp;
  uint16_t shooter_cooling_value;
  uint16_t shooter_heat_limit;
  uint16_t chassis_power_limit;
  uint8_t power_gimbal_output : 1;
  uint8_t power_chassis_output : 1;
  uint8_t power_launcher_output : 1;
};

/**
 * @brief 与 MCU 裁判摘要包布局一致的比赛状态块。
 */
struct [[gnu::packed]] RobotGameRefereeGame
{
  uint8_t game_type : 4;
  uint8_t game_progress : 4;
  uint16_t stage_remain_time;
  uint64_t sync_time_stamp;
};

/**
 * @brief 与 MCU 裁判摘要包布局一致的发射数据块。
 */
struct [[gnu::packed]] RobotGameRefereeLauncher
{
  uint8_t bullet_type;
  uint8_t launcher_id;
  uint8_t bullet_freq;
  float bullet_speed;
};

/**
 * @brief Webots 输出的裁判摘要包。
 */
struct [[gnu::packed]] RobotGameRefereeSummary
{
  RobotGameRefereeStatus robot_status;
  RobotGameRefereeGame game_status;
  RobotGameRefereeLauncher launcher_data;
};

static_assert(sizeof(RobotGameRefereeStatus) == 13);
static_assert(sizeof(RobotGameRefereeGame) == 11);
static_assert(sizeof(RobotGameRefereeLauncher) == 7);
static_assert(sizeof(RobotGameRefereeSummary) == 31);

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
