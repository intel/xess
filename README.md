# XeSS 2 SDK

The XeSS 2 SDK is a set of real-time AI-based technologies that drastically boost your frame rate at the highest visual quality while keeping your game responsive. This repository contains the SDK for integrating XeSS 2 into your game or application.

Visit [XeSS 2 Developer Page](https://www.intel.com/content/www/us/en/developer/topic-technology/gamedev/xess2.html) for an overview of the technology.

## What's Inside?

- **XeSS Super Resolution (XeSS-SR)** - boosts frame rates on **all GPUs** supporting SM6.4 (DP4A).
- **XeSS Frame Generation (XeSS-FG)** - achieves fluid motion and higher frame rates, available on Intel® Arc™ GPUs with Intel® Xe Matrix eXtensions (XMX).
- **Xe Low Latency (XeLL)** - minimizes input lag for a more responsive gaming experience, available on discrete and integrated Intel® Arc GPUs.

The SDK provides everything you need to integrate XeSS 2 into your application. While XeSS-FG requires XeLL integration, both XeSS-SR and XeLL can be enabled independently, giving you flexibility.

Additionally, we provide plugins for Unreal Engine and Unity:

- Intel® XeSS Plugin for Unreal Engine: [Github](https://github.com/GameTechDev/XeSSUnrealPlugin)
- Intel® XeSS Plugin for Unity Engine: [Github](https://github.com/GameTechDev/XeSSUnityPlugin), [Unity Asset Store](https://assetstore.unity.com/packages/tools/utilities/intel-xess-plugin-for-unity-engine-311773)

## Getting Started

We recommend starting with XeSS-SR integration to get a grasp of the APIs, then continue with XeLL followed by XeSS-FG.

Explore our [samples](./samples) to get started quickly and check out the developer guides for additional insights and integration tips:

- [XeSS-SR Developer Guide](./doc/XeSS-SR%20Developer%20Guide%202.0%20English.pdf)
- [XeSS-FG Developer Guide](./doc/XeSS-FG%20Developer%20Guide%201.1%20English.pdf)
- [XeLL Developer Guide](./doc/XeLL%20Developer%20Guide%201.1%20English.pdf)

Leverage [XeSS Inspector tool](https://github.com/GameTechDev/XeSSInspector) designed for verifying, debugging, and tuning XeSS 2 SDK integrations. XeSS Inspector allows you to inspect and visualize data coming from the game, markers/events, API calls, and create frame dumps.
