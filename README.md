# WebotsReferee

Webots 裁判摘要模拟模块：周期发布 robot_game_ref 摘要并同步发射机构状态 / Webots referee summary simulation Module that publishes the robot_game_ref summary periodically and mirrors the launcher state

## 1. 模块作用 / Purpose

WebotsReferee 是 Webots 中的裁判摘要模拟模块。它周期发布与 MCU 侧布局相同的 `robot_game_ref` 摘要包，并从 `WebotsFireNotify` 的发射机构状态同步冷却值、热量上限和发射机构使能。摘要由 LibXR 定时器任务每 `publish_period_ms` 发布一次。

摘要内容：

- `robot_status` 填充机器人 ID、等级、血量上限和当前血量（等于 `max_hp`）、冷却值、热量上限、底盘功率上限，以及云台、底盘、发射机构输出使能（均为 1）。
- 收到过发射机构状态后，每次发布前用最新状态覆盖 `shooter_cooling_value`、`shooter_heat_limit` 和 `power_launcher_output`。
- 弹速、射频与当前热量保存在模块内部的发射机构状态中。
- `game_status.sync_time_stamp` 使用当前 LibXR 时间戳（µs）。
- 其余裁判字段保持零初始化。

WebotsReferee is the referee summary simulation Module in Webots. It periodically publishes the `robot_game_ref` summary packet with the same layout as on the MCU side, and mirrors the cooling value, the heat limit and the launcher enable from the launcher state of `WebotsFireNotify`. The summary is published by a LibXR timer task every `publish_period_ms`.

Summary content:

- `robot_status` is filled with the robot ID, the level, the maximum HP and the current HP (equal to `max_hp`), the cooling value, the heat limit, the chassis power limit and the gimbal, chassis and launcher output enables (all 1).
- After a launcher state has been received, `shooter_cooling_value`, `shooter_heat_limit` and `power_launcher_output` are overwritten with the latest state before every publish.
- Bullet speed, fire rate and current heat are kept in the internal launcher state of the Module.
- `game_status.sync_time_stamp` uses the current LibXR timestamp (µs).
- The remaining referee fields stay zero-initialized.

## 2. 共享类型 / Shared Types

`WebotsRefereeTypes.hpp` 将 `RobotGameRefereeSummary` 别名到 `Referee::RobotGameRefereePack`（`RobotGameRefereeStatus`、`RobotGameRefereeGame` 同理），复用裁判协议结构体，因此模块依赖 `QDU-Robomaster/Referee`，摘要为 92 字节。`WebotsLauncherRejectReason`、`WebotsLauncherState` 和 `WebotsLauncherShotEvent` 定义在该头文件中，供 `WebotsFireNotify` 复用。

`WebotsRefereeTypes.hpp` aliases `RobotGameRefereeSummary` to `Referee::RobotGameRefereePack` (likewise `RobotGameRefereeStatus` and `RobotGameRefereeGame`), reusing the referee protocol structures, so the Module depends on `QDU-Robomaster/Referee`; the summary is 92 bytes. `WebotsLauncherRejectReason`, `WebotsLauncherState` and `WebotsLauncherShotEvent` are defined in that header and reused by `WebotsFireNotify`.

## 3. 构造接口 / Constructor

```cpp
WebotsReferee(const Param& param = {.bullet_speed = 30.0f,
                                    .shooter_heat_limit = 240.0f,
                                    .shooter_cooling_value = 40.0f,
                                    .robot_id = 7,
                                    .robot_level = 1,
                                    .max_hp = 200,
                                    .chassis_power_limit = 45,
                                    .publish_period_ms = 100,
                                    .referee_robot_game_tp_name = "robot_game_ref"});
```

依赖：无。

配置参数（`Param`）：

- `bullet_speed`：内部发射机构状态的初始弹速，单位 m/s，默认 `30.0`；摘要中没有该字段。
- `shooter_heat_limit`：初始热量上限，默认 `240.0`。
- `shooter_cooling_value`：初始每秒冷却值，默认 `40.0`。
- `robot_id`：本机机器人 ID，默认 `7`（红方哨兵）。
- `robot_level`：本机等级，默认 `1`。
- `max_hp`：最大血量和当前血量初值，默认 `200`。
- `chassis_power_limit`：底盘功率上限，单位 W，默认 `45`。
- `publish_period_ms`：裁判摘要发布周期，单位 ms，默认 `100`，最小按 1 执行。
- `referee_robot_game_tp_name`：`host` 域内裁判摘要 Topic 名，默认 `"robot_game_ref"`（与硬件 Referee 的同名参数一致），与订阅方（ArmorDetector、Aimer 的 `referee_topic`）相同。

Dependencies: none.

Configuration parameters (`Param`):

- `bullet_speed`: initial bullet speed of the internal launcher state in m/s, default `30.0`; the summary has no such field.
- `shooter_heat_limit`: initial heat limit, default `240.0`.
- `shooter_cooling_value`: initial cooling value per second, default `40.0`.
- `robot_id`: ID of this robot, default `7` (red sentry).
- `robot_level`: level of this robot, default `1`.
- `max_hp`: initial maximum HP and current HP, default `200`.
- `chassis_power_limit`: chassis power limit in W, default `45`.
- `publish_period_ms`: publish period of the referee summary in ms, default `100`, at least 1 is used.
- `referee_robot_game_tp_name`: name of the referee summary Topic in the `host` domain, default `"robot_game_ref"` (equal to the parameter of the same name of the hardware Referee), equal to the subscribers' name (`referee_topic` of ArmorDetector and Aimer).

## 4. Topic

| Topic | 方向 | 类型 | 说明 |
| --- | --- | --- | --- |
| `host/<referee_robot_game_tp_name>`（默认 `host/robot_game_ref`） | 发布 | `RobotGameRefereeSummary`（`Referee::RobotGameRefereePack`） | 裁判摘要，每 `publish_period_ms` 发布一次 |
| `webots_launcher/state` | 订阅 | `WebotsLauncherState` | 发射机构周期状态；本模块创建该 Topic |
| `webots_launcher/shot_event` | 订阅 | `WebotsLauncherShotEvent` | 仿真发射机构已接受并完成延迟后的出弹事件；本模块创建该 Topic |

| Topic | Direction | Type | Meaning |
| --- | --- | --- | --- |
| `host/<referee_robot_game_tp_name>` (default `host/robot_game_ref`) | Publish | `RobotGameRefereeSummary` (`Referee::RobotGameRefereePack`) | Referee summary, published every `publish_period_ms` |
| `webots_launcher/state` | Subscribe | `WebotsLauncherState` | Periodic launcher state; this Module creates the Topic |
| `webots_launcher/shot_event` | Subscribe | `WebotsLauncherShotEvent` | Shot event of the simulated launcher after an accepted request has completed its delay; this Module creates the Topic |

## 5. 配置示例 / Configuration Example

`xrobot instance add QDU-Robomaster/WebotsReferee` 写入的实例，无依赖项，`param` 按需修改。以下取自 `bsp-webots-autoaim` 的配置：

An instance written by `xrobot instance add QDU-Robomaster/WebotsReferee`, which has no dependencies; `param` is adjusted as needed. The following is taken from the `bsp-webots-autoaim` configuration:

```yaml
modules:
  - module: QDU-Robomaster/WebotsReferee
    id: WebotsReferee_0
    args:
      - param:
          bullet_speed: 23.0
          shooter_heat_limit: 240.0f
          shooter_cooling_value: 40.0f
          robot_id: 7
          robot_level: 1
          max_hp: 200
          chassis_power_limit: 45
          publish_period_ms: 100
          referee_robot_game_tp_name: "robot_game_ref"
```

订阅该裁判摘要的模块列在本实例之后。

Modules that subscribe to this referee summary are listed after this instance.

## 6. 依赖与硬件 / Dependencies and Hardware

依赖：

- `QDU-Robomaster/Referee`：`RobotGameRefereePack` 等裁判数据类型。
- Webots：模块面向 Webots 仿真，在 LibXR Webots 后端下构建和验证（CI 使用 `-DLIBXR_SYSTEM=webots -DLIBXR_DRIVER=webots -DWEBOTS_HOME=/usr/local/webots`）。
- LibXR。

硬件：Webots 仿真环境。

Dependencies:

- `QDU-Robomaster/Referee`: the referee data types such as `RobotGameRefereePack`.
- Webots: the Module targets the Webots simulation and is built and verified with the LibXR Webots backend (CI uses `-DLIBXR_SYSTEM=webots -DLIBXR_DRIVER=webots -DWEBOTS_HOME=/usr/local/webots`).
- LibXR.

Hardware: the Webots simulation environment.
