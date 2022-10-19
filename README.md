# DLPACK_USERSPACE
### policy_load文件夹
包含四个策略编写文件

### src文件夹
包含pcheck、pload的源代码

### pcheck
执行时会检查同级目录下policy_load文件夹中的策略文件编写是否合理。结果将保存到同级目录文件result中。

### pload
执行时会解析同级目录下result文件并写入内核。
