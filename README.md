# WebotsReferee

`WebotsReferee` 是 Webots 中的裁判摘要模拟模块。它发布 MCU 侧当前使用的
`robot_game_ref` 摘要包，并从 `WebotsFireNotify` 的发射机构状态同步弹速、射频和热量配置。

## 输入输出

输入:

- `webots_launcher/state`: 发射机构周期状态。
- `webots_launcher/shot_event`: 仿真发射机构已接受并完成延迟后的出弹事件。

输出:

- 默认 `host/robot_game_ref`: `RobotGameRefereeSummary`，布局与 MCU `RobotGameRefereePack` 一致。

## 数据内容

- `robot_status` 填充机器人 ID、等级、血量上限、冷却值、热量上限和输出使能。
- 当前摘要采用 `RefereeTypes::RobotGameRefereePack`，不再包含旧版 `launcher_data` 字段。
- 弹速、射频、当前热量仍属于 `webots_launcher/state` 和 `shot_event`；当前 Aimer 使用自身的 `default_bullet_speed`。BSP 应使相关模块的默认弹速配置一致。
- 其余未模拟的当前裁判字段保留零初始化，不把它们当作真实设备测量值。
- `game_status.sync_time_stamp` 使用当前 libxr 时间戳，其余比赛阶段字段暂不模拟。

## 配置

- `bullet_speed`: 内部发射机构状态的默认弹速，单位 m/s；保留构造接口，不新增到当前裁判摘要布局中。
- `shooter_heat_limit`: 默认热量上限。
- `shooter_cooling_value`: 默认每秒冷却值。
- `robot_id`: 本机机器人 ID。
- `robot_level`: 本机等级。
- `max_hp`: 最大血量和当前血量初值。
- `chassis_power_limit`: 底盘功率上限。
- `publish_period_ms`: 裁判摘要发布周期，单位 ms。

## 共享类型

`WebotsRefereeTypes.hpp` 将 `RobotGameRefereeSummary` 直接别名到 `RefereeTypes::RobotGameRefereePack`，
不再复制裁判协议结构体；因此模块依赖 `qdu-future/Referee`。当前验收的摘要为 92 字节。
`WebotsLauncherState` 和 `WebotsLauncherShotEvent` 仍在该头文件定义，供 `WebotsFireNotify` 复用。
