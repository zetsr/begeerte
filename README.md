# begeerte

## DO NOT USE THIS PROJECT IN ENVIRONMENTS PROTECTED BY EAC. / 避免在受EAC保护的环境中使用本项目！

简易的内存与钩子~~黑客~~，目前的唯一用处是在帧生成错误被修复前强制使其保持启用状态。（~~或许还是GPT代码与公开存储库的粘贴应用~~）

没有太多的维护价值，它被创建的最初目的只是用于逆向练习。

以及，**纪念我在DoD花费的一万小时**。

## 使用方法

1. 前往 **[begeerte](https://github.com/zetsr/begeerte/archive/refs/heads/main.zip)** 或 **[Releases](https://github.com/zetsr/begeerte/releases)** 下载项目文件

1. 编译 **[begeerte_loader](https://github.com/zetsr/begeerte/tree/main/begeerte_loader)**

2. 按需编译 **[begeerte_dod](https://github.com/zetsr/begeerte/tree/main/begeerte_dod)**、**[begeerte_gttgg](https://github.com/zetsr/begeerte/tree/main/begeerte_gttgg)**

3. 将 **[begeerte_loader](https://github.com/zetsr/begeerte/tree/main/begeerte_loader)** 以及 **[begeerte_dod](https://github.com/zetsr/begeerte/tree/main/begeerte_dod)**、**[begeerte_gttgg](https://github.com/zetsr/begeerte/tree/main/begeerte_gttgg)** 放在同一个目录。

4. 运行 **[begeerte_loader](https://github.com/zetsr/begeerte/tree/main/begeerte_loader)**

## 更新日志

### 2025/1/4

-项目改进与新内容

-为loader添加了一些安全措施（虽然无用）

### 2024/12/31

添加了一个**loader**

现在使用 **指针** 而不是 **RPM** 进行内存操作。

### 2024/12/16

重新添加了对D3D11、Vulkan的支持。

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
