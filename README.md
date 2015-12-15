# LC3 汇编器

本程序在以下环境中编译通过
```
gcc (GCC) 5.3.0
Linux Castle 4.2.5-1-ARCH #1 SMP PREEMPT Tue Oct 27 08:13:28 CET 2015 x86_64 GNU/Linux
```

使用方法:
----------
```
make
./lc3as <name>.asm
```

生成文件:
* `<name>.sym` 符号表
* `<name>.obj` 二进制文件


注意事项:
----------
* 汇编码应符合本目录下 `ISA.jpg` 中给出的格式.
* OPCODE 与 LABEL 均不区分大小写
* LABEL 的长度不能超过 30
* .ORIG 前面不能有其他内容
* .FILL 填充的数据的值不能超过 0xFFFF
* 由于换行符的问题, 建议在 linux 平台上运行
