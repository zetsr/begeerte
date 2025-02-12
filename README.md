# begeerte

支持~~多款~~游戏，拥有~~现代化~~ **loader** 的简易内存与钩子~~黑客~~（~~或许还是GPT代码与公开存储库的粘贴应用~~）

没有太多的维护价值，它被创建的最初目的只是用于逆向练习。

以及，**纪念我在DoD花费的一万小时**。

![begeerte for day of dragons](https://github.com/user-attachments/assets/afeab634-b28e-46f8-abc9-f73f9de43002)

## 使用方法（自行编译）

1. 前往 **[begeerte](https://github.com/zetsr/begeerte/archive/refs/heads/main.zip)** 或 **[Releases](https://github.com/zetsr/begeerte/releases)** 下载项目文件

2. 按需编译 **[begeerte_cs2](https://github.com/zetsr/begeerte/tree/main/begeerte_cs2)**、**[begeerte_dod](https://github.com/zetsr/begeerte/tree/main/begeerte_dod)**、**[begeerte_draconia](https://github.com/zetsr/begeerte/tree/main/begeerte_draconia)**、**[begeerte_dl](https://github.com/zetsr/begeerte/tree/main/begeerte_dl)**、**[begeerte_gttgg](https://github.com/zetsr/begeerte/tree/main/begeerte_gttgg)**

3. 使用**注入器**将编译后的DLL文件注入到目标进程。

## 使用方法（自动更新）

1. 下载 **[begeerte_loader](http://zetsr.github.io/begeerte_/begeerte_loader.exe)**

2. 运行**loader**，它会自动从我的 **[存储库](https://github.com/zetsr/zetsr.github.io/tree/main/begeerte_)** 下载最新的编译版本。

## 文档

### 一般

清理控制台：

    clear

获取 int 类型内存的值：

    mm_get_int <hex address> <hex offset>

获取 float 类型内存的值：

    mm_get_float <hex address> <hex offset>
    
设置 int 类型内存的值：

    mm_set_int <hex address> <hex offset> <int value>

设置 float 类型内存的值：

    mm_set_float <hex address> <hex offset> <float value>

### Counter-Strike 2

### Day of Dragons

启用/禁用 FrameGen 功能：

    set FrameGen <int value>

设置 RenderResolution 的值：

    set RR <float value>

设置 FrameRateLimit 的值：

    fps_max <float value>    

### Draconia

### Dragons Legacy

设置 FrameRateLimit 的值：

    fps_max <float value>    

### Golden Treasure: The Great Green

## 已知错误

### **loader** 只能在首次更新后正确删除自己

### **clear** 命令缺少格式检查

## 更新日志

### 2025/2/11

-改进**loader**

### 2025/2/10

-**loader**现在支持从云端下载以及更新。 

### 2025/2/9

-适配最新版本Version 1.2.T194

### 2025/1/14

-代码改进

### 2025/1/7

-添加指令 **fps_max**

-改进指令 **set FrameGen**

-改进指令 **set RR**

### 2025/1/5

-改进的控制台

### 2025/1/4

-项目改进与新内容

-为 **loader** 添加了一些安全措施（虽然无用）

### 2024/12/31

添加了一个 **loader**

现在使用 **指针** 而不是 **RPM** 进行内存操作。

### 2024/12/16

重新添加了对 **D3D11**、**Vulkan** 的支持。

### 2024/12/15

现在使用 **Present** 事件处理所有操作，而不是创建循环。

添加了一个操作 **渲染分辨率** 的方法，默认设置为 **75%** 。

### 2024/11/16

适配最新版本Version 1.1.7

### 2024/11/7

适配最新版本Version 1.1.4

### 2024/11/3

适配最新版本Version 1.1.3

### 2024/10/5

适配最新版本Version 1.1.1

### 2024/10/2

适配最新版本Version 1.1.1 OpenTest
