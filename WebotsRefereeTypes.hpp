#pragma once

#include <cstdint>
#include "Referee.hpp"

namespace WebotsRefereeTypes
{
/**
 * @brief Webots 发射请求的最近一次拒绝原因。
 *        Latest reject reason of a Webots fire request.
 */
enum class WebotsLauncherRejectReason : uint8_t
{
  NONE = 0,        ///< 最近一次请求未被拒绝 The latest request was not rejected
  DISABLED = 1,    ///< 发射机构未上电或被禁用 The launcher is unpowered or disabled
  PENDING = 2,     ///< 上一发仍在延迟队列中 The previous shot is still in the delay queue
  RATE_LIMIT = 3,  ///< 请求触发射频限制 The request hit the fire-rate limit
  HEAT_LIMIT = 4,  ///< 请求使热量达到或超过上限 Request reaches or exceeds the heat limit
};

/**
 * @brief 机器人状态，与 Referee 相同的类型。
 *        Robot status, the same type as in Referee.
 */
using RobotGameRefereeStatus = Referee::RobotStatus;
/**
 * @brief 比赛状态，与 Referee 相同的类型。
 *        Game status, the same type as in Referee.
 */
using RobotGameRefereeGame = Referee::GameStatus;
/**
 * @brief 裁判摘要，与 Referee 相同的类型。
 *        Referee summary, the same type as in Referee.
 */
using RobotGameRefereeSummary = Referee::RobotGameRefereePack;

/**
 * @brief Webots 发射机构当前状态快照。
 *        Snapshot of the current Webots launcher state.
 */
struct WebotsLauncherState
{
  uint64_t update_time_us{0};  ///< 状态更新时间，单位 us
  ///< State update time in us
  uint64_t last_request_time_us{0};  ///< 最近一次请求的时间，单位 us
  ///< Time of the latest request in us
  uint64_t last_fire_time_us{0};  ///< 最近一次出弹的时间，单位 us
  ///< Time of the latest shot in us
  uint64_t next_fire_request_us{0};  ///< 下一次请求可被接受的最早时间，单位 us
  ///< Earliest time at which the next request can be accepted, in us
  uint64_t pending_fire_time_us{0};  ///< 待发弹的出弹时间，单位 us，无待发时为 0
  ///< Fire time of the pending shot in us, 0 when none is pending
  uint64_t shot_count{0};  ///< 出弹计数
  ///< Shot count
  float current_heat{0.0f};  ///< 当前热量
  ///< Current heat
  float heat_limit{0.0f};  ///< 热量上限
  ///< Heat limit
  float cooling_rate{0.0f};  ///< 每秒恢复的热量
  ///< Heat recovered per second
  float single_shot_heat{0.0f};  ///< 单发增加的热量
  ///< Heat added by one shot
  float bullet_speed{0.0f};  ///< 弹丸初速度，单位 m/s
  ///< Projectile muzzle speed in m/s
  float max_fire_frequency_hz{0.0f};  ///< 最大射频，单位 Hz
  ///< Maximum fire rate in Hz
  float fire_delay_s{0.0f};  ///< 请求到出弹的延迟，单位 s
  ///< Delay from request to shot in s
  float min_fire_interval_s{0.0f};  ///< 最小发射间隔，单位 s
  ///< Minimum fire interval in s
  float current_fire_frequency_hz{0.0f};  ///< 当前射频，单位 Hz
  ///< Current fire rate in Hz
  uint8_t launcher_enabled{1};  ///< 发射机构是否使能
  ///< Whether the launcher is enabled
  uint8_t can_fire{0};  ///< 当前请求是否会被接受
  ///< Whether a request would be accepted now
  uint8_t pending_fire{0};  ///< 是否有待发弹
  ///< Whether a shot is pending
  uint8_t last_reject_reason{0};  ///< 最近一次拒绝原因，`WebotsLauncherRejectReason` 的值
  ///< Latest reject reason, a `WebotsLauncherRejectReason` value
};

/**
 * @brief Webots 发射机构的真实发弹事件。
 *        Actual shot event of the Webots launcher.
 */
struct WebotsLauncherShotEvent
{
  uint64_t shot_id{0};  ///< 出弹序号
  ///< Shot sequence number
  uint64_t request_time_us{0};  ///< 请求时间，单位 us
  ///< Request time in us
  uint64_t fire_time_us{0};  ///< 出弹时间，单位 us
  ///< Fire time in us
  uint64_t shot_interval_us{0};  ///< 与上一发的间隔，单位 us，首发为 0
  ///< Interval to the previous shot in us, 0 for the first shot
  float bullet_speed{0.0f};  ///< 弹丸初速度，单位 m/s
  ///< Projectile muzzle speed in m/s
  float heat_before{0.0f};  ///< 出弹前热量
  ///< Heat before the shot
  float heat_after{0.0f};  ///< 出弹后热量
  ///< Heat after the shot
  float heat_limit{0.0f};  ///< 热量上限
  ///< Heat limit
  float cooling_rate{0.0f};  ///< 每秒恢复的热量
  ///< Heat recovered per second
  float single_shot_heat{0.0f};  ///< 单发增加的热量
  ///< Heat added by one shot
  float fire_delay_s{0.0f};  ///< 请求到出弹的延迟，单位 s
  ///< Delay from request to shot in s
  float min_fire_interval_s{0.0f};  ///< 最小发射间隔，单位 s
  ///< Minimum fire interval in s
};
}  // namespace WebotsRefereeTypes
