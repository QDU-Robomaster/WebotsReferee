# WebotsReferee

`WebotsReferee` 是 Webots 中的裁判摘要模拟模块。它周期发布与 MCU 侧相同布局的 `robot_game_ref`
摘要包，并从 `WebotsFireNotify` 的发射机构状态同步冷却值、热量上限和发射机构使能。

## 输入输出

输入（`webots_launcher` 域，本模块也会创建这两个 topic）：

- `webots_launcher/state`：发射机构周期状态（`WebotsLauncherState`）。
- `webots_launcher/shot_event`：仿真发射机构已接受并完成延迟后的出弹事件（`WebotsLauncherShotEvent`）。

输出：

- `host/<referee_robot_game_tp_name>`（默认 `host/robot_game_ref`）：`RobotGameRefereeSummary`，
  即 `Referee::RobotGameRefereePack`，由 LibXR 定时器任务每 `publish_period_ms` 发布一次。

## 数据内容

- `robot_status` 填充机器人 ID、等级、血量上限和当前血量（等于 `max_hp`）、冷却值、热量上限、
  底盘功率上限，以及云台/底盘/发射机构输出使能（均为 1）。
- 收到过发射机构状态后，每次发布前用最新状态覆盖 `shooter_cooling_value`、`shooter_heat_limit`
  和 `power_launcher_output`。
- 弹速、射频、当前热量只保存在模块内部的发射机构状态中，不写入裁判摘要（当前摘要布局没有这些
  字段）。
- `game_status.sync_time_stamp` 使用当前 LibXR 时间戳（µs），其余比赛阶段字段不模拟。
- 其余未模拟的裁判字段保持零初始化，不把它们当作真实设备测量值。

## 共享类型

`WebotsRefereeTypes.hpp` 将 `RobotGameRefereeSummary` 直接别名到 `Referee::RobotGameRefereePack`
（`RobotGameRefereeStatus`、`RobotGameRefereeGame` 同理），不复制裁判协议结构体；因此模块依赖
`QDU-Robomaster/Referee`。摘要为 92 字节。`WebotsLauncherRejectReason`、`WebotsLauncherState` 和
`WebotsLauncherShotEvent` 在该头文件中定义，供 `WebotsFireNotify` 复用。

## 依赖

- `QDU-Robomaster/Referee`：提供 `RobotGameRefereePack` 等裁判数据类型。

外部依赖：Webots。本模块面向 Webots 仿真，只在 LibXR Webots 后端下构建和验证（CI 使用
`-DLIBXR_SYSTEM=webots -DLIBXR_DRIVER=webots -DWEBOTS_HOME=/usr/local/webots`）。

## 构造接口

```cpp
WebotsReferee(const Param& param = {.bullet_speed = 30.0f, .shooter_heat_limit = 240.0f,
                                    .shooter_cooling_value = 40.0f, .robot_id = 7,
                                    .robot_level = 1, .max_hp = 200,
                                    .chassis_power_limit = 45,
                                    .publish_period_ms = 100,
                                    .referee_robot_game_tp_name = "robot_game_ref"});
```

无依赖项。

配置（`Param`）：

- `bullet_speed`：内部发射机构状态的初始弹速，单位 m/s，默认 `30.0`；不进入裁判摘要。
- `shooter_heat_limit`：初始热量上限，默认 `240.0`。
- `shooter_cooling_value`：初始每秒冷却值，默认 `40.0`。
- `robot_id`：本机机器人 ID，默认 `7`（红方哨兵）。
- `robot_level`：本机等级，默认 `1`。
- `max_hp`：最大血量和当前血量初值，默认 `200`。
- `chassis_power_limit`：底盘功率上限，单位 W，默认 `45`。
- `publish_period_ms`：裁判摘要发布周期，单位 ms，默认 `100`，最小按 1 执行。
- `referee_robot_game_tp_name`：`host` 域内裁判摘要 Topic 名，默认 `"robot_game_ref"`（与硬件 Referee
  的同名参数一致），须与订阅方（ArmorDetector、Aimer 的 `referee_topic`）一致。

## 使用

```sh
xrobot module add QDU-Robomaster/WebotsReferee
xrobot setup
xrobot instance add QDU-Robomaster/WebotsReferee
```

`xrobot instance add` 在 `User/xrobot.yaml` 中写入一个实例，依赖项留空，默认值按源码写出。
本模块没有依赖项，按需修改 `param`：

```yaml
modules:
  - module: QDU-Robomaster/WebotsReferee
    id: webotsreferee_0
    args:
      - param:
          bullet_speed: 30.0f
          shooter_heat_limit: 240.0f
          shooter_cooling_value: 40.0f
          robot_id: '7'
          robot_level: '1'
          max_hp: '200'
          chassis_power_limit: '45'
          publish_period_ms: '100'
          referee_robot_game_tp_name: '"robot_game_ref"'
```

本模块不使用 BSP 对象，不需要 `XR_REGISTER`。订阅该裁判摘要的模块应排在本实例之后。

填好后再次运行 `xrobot setup`，生成 `User/xrobot_main.hpp`。

`xrobot module show .`（在本仓库中）或 `xrobot module show Modules/QDU-Robomaster/WebotsReferee`
（在 BSP 中）打印当前的构造函数。
