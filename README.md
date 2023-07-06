### 一、前言
从大二开始，对操作系统感兴趣，当时对着哈尔滨工业大学李治军老师的公开课学习。但是一方面实验的平台是基于 **linux** 的，很多基础指令都不会使用；另一方面，当时知识面薄弱，很多方面理解不足，导致对操作系统的学习处于一个比较低的水平。  
  
现在是研二了，正处于找工作的时间段，想着试试操作系统相关的开发，在朋友的推荐下开始学习 **6.S081** 相关的课程。随着学习的深入，发现操作系统内核实在是有太多太多精妙的设计，自己的能力可能远远不够操作系统内核的开发。  
  
但是无论怎样，这次把 **6.S081** 所有的实验完成，这个过程就当是圆自己的一个**小小梦想**吧！^_^  

我把 **Traps** 这个实验放在第一个是因为之前的3个实验已经过去蛮久了，有点没印象了，之后会慢慢补上的。然后所有的实验内容目前来说没有参考任何资源，所以可能有不对的地方，请大家批评指正。

### 二、关于环境配置和相关包的一些安装
当时安装的过程挺不容易，这里给出2个核心的参考链接：  
https://www2014.aspxhtml.com/post-23428/  
https://blog.csdn.net/ConstineWhy/article/details/123291723  
和一些重要的注意事项：  
  
* **ubuntu** 版本需要 20.04
* 更改阿里镜像源（安装一些包的时候能方便很多） 参考 https://developer.aliyun.com/mirror/ubuntu?spm=a2c6h.13651102.0.0.3e221b113ijmV6  
* 安装 **SSH** 、配置 **IP** （一方面，可以让虚拟机联网下载包；另一方面，我在本机上使用 **VScode** 远程完成实验，这样开发起来比较方便）  
* 安装 **RISC-V** 交叉编译工具和 **QEMU**

上面这些注意事项在给出的第一个参考链接里面都有对应的内容，如果遇到问题，再看第二个链接或者自行解决。这样，实验环境就ok啦。  

## 实验之前很重要的一点，一定要耐下心、好好看文档+内核源码（其实看英文文献尤其锻炼咱们的耐心，看不明白的就 **chat-gpt** 帮忙翻译），多思考这么设计的原因和结果，可能会有不一样的感悟。

### 三、Traps实验  （https://pdos.csail.mit.edu/6.S081/2020/labs/traps.html）

#### **1. RISC-V assembly (easy)**  
user/call.c源码
```
int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  printf("%d %d\n", f(8)+1, 13);
  exit(0);
}
```  
其对应的汇编版本为  
```
0000000000000000 <g>:

int g(int x) {
   0:	1141                	addi	sp,sp,-16
   2:	e422                	sd	s0,8(sp)
   4:	0800                	addi	s0,sp,16
  return x+3;
}
   6:	250d                	addiw	a0,a0,3
   8:	6422                	ld	s0,8(sp)
   a:	0141                	addi	sp,sp,16
   c:	8082                	ret

000000000000000e <f>:

int f(int x) {
   e:	1141                	addi	sp,sp,-16
  10:	e422                	sd	s0,8(sp)
  12:	0800                	addi	s0,sp,16
  return g(x);
}
  14:	250d                	addiw	a0,a0,3
  16:	6422                	ld	s0,8(sp)
  18:	0141                	addi	sp,sp,16
  1a:	8082                	ret

000000000000001c <main>:

void main(void) {
  1c:	1141                	addi	sp,sp,-16
  1e:	e406                	sd	ra,8(sp)
  20:	e022                	sd	s0,0(sp)
  22:	0800                	addi	s0,sp,16
  printf("%d %d\n", f(8)+1, 13);
  24:	4635                	li	a2,13
  26:	45b1                	li	a1,12
  28:	00000517          	auipc	a0,0x0
  2c:	7c050513          	addi	a0,a0,1984 # 7e8 <malloc+0xea>
  30:	00000097          	auipc	ra,0x0
  34:	610080e7          	jalr	1552(ra) # 640 <printf>
  exit(0);
  38:	4501                	li	a0,0
  3a:	00000097          	auipc	ra,0x0
  3e:	27e080e7          	jalr	638(ra) # 2b8 <exit>
```
(1) 哪些寄存器包含了 **printf** 函数的参数？  
答：根据之前阅读的材料《**Calling Convention**》和此处的汇编代码，可以看到，寄存器 **a0** 、 **a1** 、 **a2** 包含了分别包含了 **printf** 的3个参数。  
  
(2) **f** 、 **g** 函数在 **main** 函数中的调用情况  
答：根据汇编代码，**f** 、 **g** 函数的调用过程似乎被编译器优化掉了。（这个有点不太确定）  

(3) **printf** 函数的地址  
答：0x640。
```
0000000000000640 <printf>:

void
printf(const char *fmt, ...)
{
 640:	711d                	addi	sp,sp,-96
 642:	ec06                	sd	ra,24(sp)
 644:	e822                	sd	s0,16(sp)
```

(4) **main** 函数中的 '**jalr**' 指令跳转到 **printf** 函数中去之后， **ra**（return address）寄存器值是多少？  
答：（本问是通过gdb查看的）在执行'**jalr**' 指令之前， **ra** 值是0x30，也就是上面的指令 “auipc ra, 0x0” 的地址；  
执行'**jalr**' 指令之后，**ra** 值是0x38，也就是上面的指令 “li a0, 0” 的地址。  
执行'**jalr**' 指令之前：  
![](https://github.com/2351889401/6.S081-Lab-Traps/blob/main/images/before_jalr.png)
执行'**jalr**' 指令之后：  
![](https://github.com/2351889401/6.S081-Lab-Traps/blob/main/images/after_jalr.png)

(5)代码段
```
unsigned int i = 0x00646c72;
printf("H%x Wo%s", 57616, &i);
```
的输出是什么？能产生该输出是在 **RISC-V** 是小端（ **little-endian** ）的条件下，如果是在大端（ **big-endian** ）的条件下，需要对 “**57616**” 和 “**i**” 的值做出改变吗？  
答：输出是 “**HE110 World**”。大端模式下：**i** 的值应当变成 “**0x726c6400**”，**57616** 不需要改变。  
个人的想法是 **“%x”** 是将数据作为16进制从高有效位到低有效位读取的，因此，无论大端小端，**57616** 从高到低有效位读取的结果都是 “**E110**”，不需要改变；而 **“%s”** 应当是与地址有关的读取，仍然需要从低地址到高地址为 **“0x72、0x6c、0x64、0x00”** ，所以 **i** 的值应当变成 “**0x726c6400**”。  

(6)代码段
```
printf("x=%d y=%d", 3);
```
会导致 **“y=”** 输出什么？为什么会发生这样的情况？  
答：**“y=”** 的输出结果是进入 “**printf**” 函数时的寄存器 **a2** 的值。  
在 **call.c** 中添加该代码段，经过编译后查看 **call.asm** 文件：
```
printf("x=%d y=%d\n", 3);
  24:	458d                	li	a1,3
  26:	00000517          	auipc	a0,0x0
  2a:	7d250513          	addi	a0,a0,2002 # 7f8 <malloc+0xe8>
  2e:	00000097          	auipc	ra,0x0
  32:	624080e7          	jalr	1572(ra) # 652 <printf>
  printf("%d %d\n", f(8)+1, 13);
  36:	4635                	li	a2,13
  38:	45b1                	li	a1,12
  3a:	00000517          	auipc	a0,0x0
  3e:	7ce50513          	addi	a0,a0,1998 # 808 <malloc+0xf8>
  42:	00000097          	auipc	ra,0x0
  46:	610080e7          	jalr	1552(ra) # 652 <printf>
```
两个部分主要的差别是上面的的少了给寄存器 **a2** 赋值的指令。如果执行过程没有报错的话，**“y=”** 的输出结果就是进入 “**printf**” 函数时的寄存器 **a2** 的值。
下图是 **gdb** 刚进入 “**printf**” 函数时寄存器的情况，可以看到 **a1** 保留了参数3，**a2** 参数为 **5237**，最终的输出也是 “**y=5237**”。
![](https://github.com/2351889401/6.S081-Lab-Traps/blob/main/images/a1a2.png)
![](https://github.com/2351889401/6.S081-Lab-Traps/blob/main/images/output.png)

**2. Backtrace (moderate)**  
实验目标：如果内核中有函数发生错误，那么最好能通过调用关系显示调用流程，逐步定位发生错误的点和原因。  
实验方法：利用内核中函数调用过程中分配的stack，stack会保存调用者（**caller**）的 **frame pointer** ，这样就可以沿着这条线一直走，直到最早的调用者。  
  
我们会在 “**kernel/printf.c**” 中实现函数 **backtrace()**，并在 “**sys_sleep**” 中调用 **backtrace()** 追踪函数调用流程中的所有返回地址。  
本次实验通过 **sys_sleep** 查看上述过程，我们知道 **xv6** 中系统调用的流程如下为： **trap.c/usertrap() -> syscall.c/syscall() -> sysproc.c/sys_*()**,  
在 **sys_sleep** 中加入 **backtrace()** 后，调用流程和 **frame pointer** 、 **return address** 的指向如图所示：  
![](https://github.com/2351889401/6.S081-Lab-Traps/blob/main/images/frame.png)
至于为什么到 “**trap.c**” 停止了，因为 “**trap.c**” 是被汇编程序 “**trampoline.S/uservec**” 调用，可能汇编调C不会产生内核栈？（这一点我也不确定）  
  
代码主要修改处（这里就忽略细节了）：  
“**kernel/riscv.h**”
```
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x) );
  return x;
}
```
“**kernel/printf.c**”
```
void backtrace(void) {
  uint64 _fp = r_fp(); //当前的frame_pointer在内核中的地址
  //利用它找到当前的return address 和 前一个stack frame的frame_pointer
  while(PGROUNDUP(_fp) - PGROUNDDOWN(_fp) == PGSIZE) {
    uint64* ra_pos = (uint64*)(_fp - 8);
    uint64 ra = *ra_pos;
    printf("%p\n", ra);
    
    uint64* pre_fp = (uint64*)(_fp - 16);
    _fp = *pre_fp;
  }
}
```
得到的结果为：  
![](https://github.com/2351889401/6.S081-Lab-Traps/blob/main/images/result2.png)  

**3. Alarm (hard)**  
实验的目标：是想教会我们：当用户程序异常、中断时，一种通用的内核中应当实现的处理方式。也就是说真正重要的是**面对应用程序的这种情况，我们应该在内核中有怎样的设计思想去处理**。  
广泛的设计思想是：当用户程序发生异常、中断时，我们应该像**系统调用**那样，先跑进内核，把用户断点保存，然后执行相应的中断/异常处理程序，处理完后，再从断点恢复。  
然而，因为异常、中断毕竟和**系统调用**不同，内核里面（**trap.c**）中需要它们自己的实现方式。  

实验方法：实现系统调用 “**sys_sigalarm(n, fn)**”，用户程序调用该系统调用。不断地计时，每隔n个时钟中断，执行（函数指针）**fn** 指向的函数。执行完 **fn** 指向的函数后，恢复用户程序（依靠 “**sys_sigreturn()**”），继续执行。
如果是 “**sys_sigalarm(0, 0)**”，表示（关闭计时、中断处理程序）流程。  
  
* **注意，本来这里应该按实验思路是一步一步走的（就是先通过 test0 ，然后再满足 test1、test2 的要求），但是完成之后，中间过程被修改了（因为仅仅满足 test0 还是有bug），我也有些遗忘了，所以这里就直接给出最后的答案，思路会随着代码给出**

（1）在用户空间给出关于系统调用函数的声明、**Makefile**文件的修改，略  
（2）满足 **test0** 的要求：  
a. 在 “**sysproc.c/sys_sigalarm(n, fn)**” 中记录当前用户空间传递过来的 “**n和fn**” 参数，很显然，我们可以直接使用下面的方式。因为在 “**trampoline.S**” 中已经将所有的寄存器保存在 **TRAPFRAME** 中。
而 **TRAPFRAME** 这一页在用户空间的页表中注册过，在**kernel**的页表里面没有，但是在**kernel**中可以使用 “**p->trapframe**” 的方式访问当前进程的 “**TRAPFRAME**”。这是因为虽然 “**p->trapframe**” 和 **TRAPFRAME** 分别在**kernel**页表和用户页表中注册，但实际上却是一个物理内存页。
参考“proc.c/proc_pagetable”中下面的代码段：
```
// map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }
```
的确是把“**p->trapframe**”注册到用户页表中“**TRAPFRAME**”的位置。  
  
因此，可以在 “**sysproc.c/sys_sigalarm(n, fn)**” 这样去获取两个参数（按照**Calling Convention**应当保存在寄存器**a0**和**a1**中）
```
myproc()->interval = myproc()->trapframe->a0;//第一个参数
myproc()->fn_pos = myproc()->trapframe->a1;//第二个参数
```
当然，这里面应当在“**kernel/proc.h**”中增加变量表示上述的“**n和fn**”
```
//加入3个新的变量
int interval; //tick间隔
int now_interval; //现在走过的时间间隔
uint64 fn_pos; //handler function的地址(user space)
```
  
同时在“**kernel/proc.c**”的“**allocproc()**”函数中将它们初始化
```
//一些初始化部分
p->interval = 0;
p->now_interval = 0;
p->fn_pos = 0;
```

当时钟中断到来时，我们的处理部分在“**kernel/trap.c/usertrap()**”如下的部分（这里只是为了先达到**test0**的要求，输出“**alarm!**”）：  
**有个很重要的点是：当在“trap.c/usertrapret()”中执行的时候，会将p->trapframe->epc的值写入到sepc寄存器，而在sret指令执行的时候，sepc寄存器会将值写入到pc寄存器（需要仔细看文献），所以在下面的代码段里面有 p->trapframe->epc = p->fn_pos; 这样一句代码，就把所有的流程都能接上了**
```
if(which_dev == 2)
  {
    yield();
    if(p->interval > 0) {
      if(p->now_interval < p->interval - 1) p->now_interval++;
      else {
        p->trapframe->epc = p->fn_pos;
        p->interval = 0;//这样是否可以保证在handler的过程中不会进来? 
        p->now_interval = 0; //重新计时
      }
    }
  }
```
到这里，应该就可以输出“**alarm!**”了。

（3）修改代码至正确，满足“**test0 test1 test2**”的要求
