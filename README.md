# EyeTracker

## 项目内容

### 功能描述

由安卓移动设备作为传感器，通过目视控制，使用者可以使用目视跟踪技术控制鼠标、使用屏幕键盘进行键入。

### 必要性

- 提供一种全新计算机交互方式，用眼球完整的进行系统层面交互。
- 为身体行动不便的群体提供帮助，为全世界许多ASL患者带来了在电脑上操作的可能。
- 采用手机作为传感器，方便快捷，减少设备购置成本。

### 特殊性

- [三星EyeCan](http://www.eyecanproject.org/p/english.html)：眼球鼠标，是一个安装在显示器下方的装置，可以让用户仅用眼球转动就可操控电脑。与之相比，这个项目采用手机作为外设，方便快捷，减少设备购置成本。
- [Windows 10 目视控制入门](https://support.microsoft.com/zh-cn/help/4043921/windows-10-get-started-eye-control)：采用Tobii外设，由屏幕上的启动板选择模式，控制执行相应鼠标操作或打字。与之相比，这个项目的目视检测模块运行在手机中，减少计算机的资源开销。

### 关键技术

- 实现安卓客户端，使用[caffe-android-lib](https://github.com/sh1r0/caffe-android-lib)将现有的眼动识别[caffe模型](https://github.com/CSAILVision/GazeCapture)移植到安卓客户端，实现校准、控制鼠标移动和点击、控制滚动的基础功能。
- 实现服务端，利用socket实现windows系统pc与android设备连接通信，涉及网络编程。
- 根据模型识别眼动精度调整具体交互方式。

### 参考文献

- [《Eye Tracking for Everyone》](https://people.csail.mit.edu/khosla/papers/cvpr2016_Khosla.pdf)Krafka K, Khosla A, Kellnhofer P, et al. Eye tracking for everyone[C]//Proceedings of the IEEE conference on computer vision and pattern recognition. 2016: 2176-2184.
- [andorid手机远程控制电脑](https://gitee.com/lujianing/android-remote-control-computer)

### 完成计划

- 阅读文件 11/2-11/15
- 编码 客户端：11/16-12/15 服务端：12/15-12/30
- 实验 1.1/1.16

## 第一次会议内容
*2018-10-26*

### 文献阅读

1. 《Battery-Free Eye Tracker on Glasses》

      *Li T, Zhou X. Battery-Free Eye Tracker on Glasses[C]//ACM Conference on Mobile Computing and Networking (MobiCom). 2018.*

      - 描述了一种基于瞳孔吸光特性的无电池可穿戴式眼动仪，可以同时跟踪瞳孔的二维位置和直径。
      - 通过在眼睛周围设置近红外(NIR)光和光电二极管，发射近红外光从各个方向依次照亮眼睛，由光电二极管感知反射光的空间模式，通过轻量级推理算法来推断瞳孔的位置和直径。
      - 该系统利用不同眼动阶段的特点，相应地调整其传感和计算，以进一步节省能源。
      - 原型机由现成的硬件组件组成，并集成到普通眼镜中。

2. 《Eye Tracking for Everyone》

      *Krafka K, Khosla A, Kellnhofer P, et al. Eye tracking for everyone[C]//Proceedings of the IEEE conference on computer vision and pattern recognition. 2016: 2176-2184.*
      - 利用手机或平板电脑摄像头直接实现人眼追踪。
      - 包含一个卷积神经网络iTracker，采用来自1450人的近250万帧图像进行训练，在ios系统的现代移动设备上实时运行(10-15fps)。
      - 与已有的方法相比，它的误差大大降低。

### 毕业项目构思

- 用眼球移动控制计算机的app；
- 利用上面提到第二篇文章中基于caffe实现并完成训练的模型，实现利用移动设备摄像头跟踪人眼运动.

### 导师反馈
同意构思，要求设计人眼运动与计算机的交互。

## 第二次会议内容
*2018-10-30*

### 毕业项目构思
模仿[Windows 10 目视控制](https://support.microsoft.com/zh-cn/help/4043921/windows-10-get-started-eye-control)的交互方式，实现人眼注视的校准、控制鼠标移动和点击、控制滚动的基础功能。

### 导师反馈
同意构思
