# WebotsReferee

`WebotsReferee` 是 Webots 中的裁判摘要模拟模块。它发布 MCU 侧当前使用的
`robot_game_ref` 摘要包，并从 `WebotsFireNotify` 的发射机构状态同步弹速、射频和热量配置。

## 输入输出

输入:

- `webots_launcher/state`: 发射机构周期状态。
- `webots_launcher/shot_event`: 真实接受并完成延迟后的出弹事件。

输出:

- 默认 `host/robot_game_ref`: `RobotGameRefereeSummary`，布局与 MCU `RobotGameRefereePack` 一致。

## 数据内容

- `robot_status` 填充机器人 ID、等级、血量上限、冷却值、热量上限和输出使能。
- `launcher_data.bullet_speed` 来自发射机构状态或构造参数默认值。
- `launcher_data.bullet_freq` 来自真实出弹事件的最近射频；长时间无发弹时由发射机构状态恢复为 0。
- `game_status.sync_time_stamp` 使用当前 libxr 时间戳，其余比赛阶段字段暂不模拟。

## 配置

- `bullet_speed`: 默认弹速，单位 m/s。
- `shooter_heat_limit`: 默认热量上限。
- `shooter_cooling_value`: 默认每秒冷却值。
- `robot_id`: 本机机器人 ID。
- `robot_level`: 本机等级。
- `max_hp`: 最大血量和当前血量初值。
- `chassis_power_limit`: 底盘功率上限。
- `publish_period_ms`: 裁判摘要发布周期，单位 ms。

## 共享类型

`WebotsRefereeTypes.hpp` 定义 `RobotGameRefereeSummary`、`WebotsLauncherState` 和
`WebotsLauncherShotEvent`。`WebotsFireNotify` 复用同一份类型，避免裁判和发射机构之间重复维护结构体布局。
