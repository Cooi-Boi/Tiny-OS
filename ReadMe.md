## Tiny-OS的项目介绍 ^^

-------
**关于这个Tiny-OS 这个是我随着《操作系统真象还原》从零开始自写的项目 当然现在里面的代码都是最后版本的代码了
\
除了本书中最后的三个功能 `Exec` `Wait` `Exit`和管道没有实现 其余的全部实现了
\
全部的实现流程都用博客记录了下来 CSDN : (https://love6.blog.csdn.net) 
\
里面很详细的记录了每一章我的实现过程 并且也对本书中的错误进行了勘误**


**这个项目实现起来还是相当不容易的 我大概花了整整40多天的时间**

**从早到晚一直看书 一直敲代码才最终实现了出来**

**Debug与调试几天也是家常便饭 当然现在也实现出来了 还是非常喜悦的事情**

**尽管来说 代码绝大部分都是书上的代码 但是能够理解绝大部分代码 并且自己再动手敲一遍 并实现正确 还是一件很令人激动的事情**

**这些代码现在也就开源 希望对各位学习操作系统或者一些感兴趣的兄弟们有所帮助 哈哈**

**刚刚统计了一下代码行数 算上MakeFile以及头文件的行数的话 大概是7800行 如果抛去那些只算核心代码行的话 大概有6000行左右 哈哈 我也没想到自己能坚持下来写这么多行 还是挺不容易的**

**那ReadMe就先写到这里 没想到这里面也用的是MarkDown的语法 作为第一个在GitHub上传的项目还是挺开心的 那各位江湖再见！**

**Hope u can enjoy this tiny os~ Sharing and Getting.**

---

**全流程记录博客链接如下：**
\
**[《操作系统真象还原》第一章 ---- 安装Vmware Station 安装Ubuntu 装载配置Bochs 安装Vmware tools 开始乘帆历险！](https://love6.blog.csdn.net/article/details/117751327)**
\
**[《操作系统真象还原》第二章 ---- 编写MBR主引导记录 初尝编写的快乐 雏形已显！](https://love6.blog.csdn.net/article/details/117782012)**
\
**[《操作系统真象还原》第三章 ---- 完善MBR 尝汇编先苦涩后甘甜而再战MBR！](https://love6.blog.csdn.net/article/details/117813233)**
\
**[《操作系统真象还原》第四章 ---- 剑指Loader 刃刺GDT 开启新纪元保护模式 解放32位](https://love6.blog.csdn.net/article/details/117839108)**
\
**[《操作系统真象还原》第五章 ---- 轻取物理内存容量 启用分页畅游虚拟空间 力斧直斩内核先劈一角 闲庭信步摸谈特权级](https://love6.blog.csdn.net/article/details/117871478)**
\
**[《操作系统真象还原》第六章 ---- 开启c语言编写函数时代 首挑打印函数小试牛刀 费心讨力重回gcc降级 终尝多日调试之喜悦](https://love6.blog.csdn.net/article/details/117964307)**
\
**[《操作系统真象还原》第七章 ---- 终进入中断处理拳打脚踢 操作系统日渐成熟 目前所有代码总览](https://love6.blog.csdn.net/article/details/118002341)**
\
**[《操作系统真象还原》第八章 ---- 初入内存管理系统 涉足MakeFile 了解摸谈一二](https://love6.blog.csdn.net/article/details/119042923)**
\
**[《操作系统真象还原》第九章 ---- 终进入线程动斧开刀 豁然开朗拨云见日 还需解决同步机制才能长舒气](https://love6.blog.csdn.net/article/details/119107389)**
\
**[《操作系统真象还原》第十章 ---- 线程打印尚未成功 仍需此章锁机制完善努力 在前往最终章的路上激流勇进](https://love6.blog.csdn.net/article/details/119179925)**
\
**[《操作系统真象还原》第十一章 ---- 实现用户进程 欺骗CPU通彻进程原理 眺望终点到达还需砥砺前行](https://love6.blog.csdn.net/article/details/119274248)**
\
**[《操作系统真象还原》第十二章 ---- 实现系统调用深入浅出 进一步完善堆内存分配与Printf函数 让用户进程有话可说（上）](https://love6.blog.csdn.net/article/details/119315561)**
\
**[《操作系统真象还原》第十二章 ---- 实现系统调用深入浅出 进一步完善堆内存分配与Printf函数 让用户进程有话可说（下）](https://love6.blog.csdn.net/article/details/119349419)**
\
**[《操作系统真象还原》第十三章 ---- 编写硬盘驱动软件 行百里者半九十终成时喜悦溢于言表](https://love6.blog.csdn.net/article/details/119354851)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（上一）](https://love6.blog.csdn.net/article/details/119421194)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（上二）](https://love6.blog.csdn.net/article/details/119494541)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（中一）](https://love6.blog.csdn.net/article/details/119523612)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（中二）](https://love6.blog.csdn.net/article/details/119551196)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（下一）](https://love6.blog.csdn.net/article/details/119574834)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（下二）](https://love6.blog.csdn.net/article/details/119602546)**
\
**[《操作系统真象还原》第十四章 ---- 实现文件系统 任务繁多 饭得一口口吃路得一步步走啊（总结篇）](https://love6.blog.csdn.net/article/details/119615926)**
\
**[《操作系统真象还原》第十五章 ---- 实现系统交互 操作系统最终章 四十五天的不易与坚持终完结撒花（上）](https://love6.blog.csdn.net/article/details/119638450)**
\
**[《操作系统真象还原》第十五章 ---- 实现系统交互 操作系统最终章 四十五天的不易与坚持终完结撒花（中）](https://love6.blog.csdn.net/article/details/119672216)**
\
**[《操作系统真象还原》第十五章 ---- 实现系统交互 操作系统最终章 四十五天的不易与坚持终完结撒花（下）](https://love6.blog.csdn.net/article/details/119685650)**

------
**Tiny_OS's Some Pics**

**System interaction**

![image](https://user-images.githubusercontent.com/72536813/142836107-ddbf47ce-d88b-4880-a409-85e16df0a63a.png)

----
**File system**

![image](https://user-images.githubusercontent.com/72536813/142836202-77b9e15f-aa63-4436-b27a-8c7db1e3fb62.png)

----
**Malloc & Free**

![image](https://user-images.githubusercontent.com/72536813/142834974-eebdb794-5375-480f-90b5-20983a1cf86b.png)

----
**Process & Thread**

![image](https://user-images.githubusercontent.com/72536813/142835202-425d822c-1a54-45c3-8240-a84a30619d2b.png)







