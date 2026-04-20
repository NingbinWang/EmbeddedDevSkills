<div align="center">

# 嵌入式Linux的智能AI Skill-EmbeddedDevelopmentSkills

> *「这个Skills主要是收集作者的各种技能，帮助各位做嵌入式Linux的开发者提高自己写代码的能力」*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Claude Code](https://img.shields.io/badge/Claude%20Code-Skill-blueviolet)](https://claude.ai/code)
[![Skills](https://img.shields.io/badge/skills.sh-Compatible-green)](https://skills.sh)

<br>

**目前主要技能包含内核代码的review,代码生成以及相应的代码优化等技能。**

<br>


[![Star History Chart](https://api.star-history.com/svg?repos=NingbinWang/EmbeddedDevSkills&type=Date)](https://star-history.com/#NingbinWang/EmbeddedDevSkills&Date)

</div>

## 技能描述
* kernel-code-gen - 主要是生成相应的内核代码，目前尚未进行相应的修改
* kernel-code-review - 主要是内核代码的review，目前支持内核代码review，代码生成以及相应的代码优化等技能。
* kernel-perf-analysis - 主要做性能分析使用，目前直接从简说linux的自制复制过来，诊断Linux系统在CPU、内存、I/O和网络方面的性能问题与瓶颈。适用于用户要求分析出现以下状况：运行缓慢、过载、卡顿、超时、丢包、内存交换、磁盘阻塞，或显示高负载 / 高延迟。目标是定位瓶颈所在的子系统、判断瓶颈出现在内核态还是应用态，并定位应用层中导致问题的具体进程。后续会逐步完善更改。

---

## 关于更新
* 2026.04.20 更新了kernel-code-review技能，添加了代码review的详细描述。
* 陆续更新中...

---
## 演示
笔者暂时无法展示演示功能，主要是因为作者目前仅买了DeepSeek的API，会遇到
```
API Error: 400 {"error":{"message":"This model's maximum context length is 102400 tokens. However, you requested 128700 tokens (63164 in the messages, 65536 in the completion). Please reduce the length of the messages or                                                       
     completion.","type":"invalid_request_error","param":null,"code":"invalid_request_error"}}
```
后续会使用百练的模型进行测试。



---

## 安装

暂时没有将这些技能放到网络上直接用nxp skills add 添加。目前需要手动来添加。

直接将kerel-code-review直接复制粘贴到 **.claude/skills/**,如果没有skills文件夹，请手动创建一个skills文件夹即可。

---



## 关于参考
本技能主要参考了ascendc的设计思路，对其进行相应的各种修改，用于支持嵌入式Linux的开发。

* 感谢[ascendc](https://gitcode.com/Ascend)提供的参考。
* 感谢[简述Linux](https://github.com/simple-tec/linux-performance-annlysis-skill)提供的参考。


## 关于作者

**AlexKing** — 一个嵌入式开发者老兵，分享自己的开发工作经历。

| 平台 | 链接 |
|------|------|
| 💬 公众号 | 微信搜「图布技术说」或扫码关注 ↓ |

<img src="Images/qrcode.jpg" alt="公众号二维码" width="360">

## 许可证

MIT — 随便用，随便改，随便造。


---

<div align="center">

<br>

MIT License © [AlexKing Tuubu](https://github.com/NingbinWang/)

</div>

---
