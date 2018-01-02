# UEFI FINAL PROJECT (推箱子)



### 简介

*  方向键控制人物移动

* 1 ~ 35 个 关卡 选择

* GUI界面

* 玩法与经典推箱子一致

  ​


### 使用方法

1. 在AppPkg\Applications 文件夹下新建一个文件夹, 将final.c 和 final.inf 添加进去

2. 在AppPkg.dsc 的 [Components] 标签下添加 final.inf

3. 在 vs 命令行提示符里, 进入工程目录,  输入以下指令:

   > edksetup.bat
   >
   > build -p AppPkg\AppPkg.dsc

   即可完成对final.c的编译, 生成的 Final.efi 在 工程 目录的 build 文件夹 下找

4.  把上一步得到的Final.efi ( 或使用我给出的Final.efi ) copy 到 SecMain.exe 的目录下

5.  运行 SecMain.exe, 输入 Final

6. 把Socoban 这个文件夹 copy 到 SecMain.exe 的目录下

7. 然后就可以玩了 