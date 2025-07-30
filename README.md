# XeSS 2 SDK

The XeSS 2 SDK is a set of real-time AI-based technologies that drastically boost your frame rate at the highest visual quality while keeping your game responsive. This repository contains the SDK for integrating XeSS 2 into your game or application.

Visit [XeSS 2 Developer Page](https://www.intel.com/content/www/us/en/developer/topic-technology/gamedev/xess2.html) for an overview of the technology.

## What's Inside?

- **XeSS Super Resolution (XeSS-SR)** - boosts frame rates on **all GPUs** with SM 6.4 (DP4a) support.
- **XeSS Frame Generation (XeSS-FG)** - achieves fluid motion and higher displayed frame rates, available on **all GPUs** with SM 6.4[^1] support.
- **Xe Low Latency (XeLL)** - minimizes input lag for a more responsive gaming experience, available on discrete and integrated Intel® Arc™ GPUs and all GPUs when combined with XeSS-FG.

The SDK provides everything you need to integrate XeSS 2 into your application. While XeSS-FG requires XeLL integration, both XeSS-SR and XeLL can be enabled independently, giving you flexibility.

Additionally, we provide plugins for Unreal Engine and Unity:

- Intel® XeSS Plugin for Unreal Engine: [GitHub](https://github.com/GameTechDev/XeSSUnrealPlugin)
- Intel® XeSS Plugin for Unity Engine: [GitHub](https://github.com/GameTechDev/XeSSUnityPlugin), [Unity Asset Store](https://assetstore.unity.com/packages/tools/utilities/intel-xess-plugin-for-unity-engine-311773)

## Getting Started

We recommend starting with XeSS-SR integration to get a grasp of the APIs, then continue with XeLL followed by XeSS-FG.

Explore our [samples](./samples) to get started quickly and check out the developer guides for additional insights and integration tips:

- [XeSS-SR Developer Guide](./doc/xess_sr_developer_guide_english.md)
- [XeSS-FG Developer Guide](./doc/xess_fg_developer_guide_english.md)
- [XeLL Developer Guide](./doc/xell_developer_guide_english.md)

Leverage [XeSS Inspector tool](https://github.com/GameTechDev/XeSSInspector) designed for verifying, debugging, and tuning XeSS 2 SDK integrations. XeSS Inspector allows you to inspect and visualize data coming from the game, markers/events, API calls, and create frame dumps.

[^1]: On Intel® Arc™ GPUs XeSS-FG requires Intel® Xe Matrix Extensions (XMX) support.
