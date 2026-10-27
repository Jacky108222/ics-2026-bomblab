markdown
# BombLab 实验报告

## 第 1 关：函数调用、参数传递、返回值

### 关键汇编

`phase_01` 要求 1 个 u32（`valid_u32_fields` 参数 1，格式串 `"%u"`），调用 `p1_compute` 与输入比较：

```asm
call valid_u32_fields      # 参数 1
call sscanf                # "%u"
cmp  $0x1,%eax
jne  失败
mov  %r12,%rdi             # challenge
call p1_compute
cmp  %eax,-0x14(%rbp)      # 输入 vs 返回值
sete %al
p1_compute 拆 challenge：

asm
shr  $0x20,%rax            # hi = challenge >> 32
mov  %edi,%r8d             # lo = challenge 低32位
shr  $0x10,%rdx            # mid = challenge >> 16
call p1_step2              # (hi, lo)
call p1_step0              # (lo, mid)
call p1_step1              # (c, t)
三个 step：

asm
p1_step0: sub %esi,%edi; lea 0xa42c(%rdi,%rdi,2),%eax   # 3*(a-b)+0xa42c
p1_step1: lea (%rdi,%rsi,8),%eax; xor $0xa57,%eax       # (a+8b)^0xa57
p1_step2: lea (%rdi,%rdi,4),%eax; add $0xbae6,%esi; xor %esi,%eax  # (5a)^(b+0xbae6)
恢复出的计算关系
c
hi  = challenge >> 32;
lo  = (uint32_t)challenge;
mid = (uint32_t)(challenge >> 16);
t = (5*hi) ^ (lo + 0xbae6);
c = 3*(lo - mid) + 0xa42c;
answer = (c + 8*t) ^ 0xa57;
32 位无符号运算，按模 2³² 回绕。

推理过程
sscanf 格式串 "%u"、valid_u32_fields 参数 1，确定 1 个整数。cmp %eax,-0x14(%rbp) 比较输入和 p1_compute 返回值。shr $0x20 取高 32 位，shr $0x10 取中间 32 位。lea 偏移(基址,变址,比例) 按 基址+变址×比例+偏移 翻译成乘加。三个 step 按 p1_compute 调用顺序拼起来。

验证
bash
./defuser --seed 42 --phase 1 | ./bomb --seed 42 --phase 1    # Phase 1 defused.
./defuser --seed 43 --phase 1 | ./bomb --seed 43 --phase 1    # 非公开 seed 通过
python3 public_test.py --phase 1                               # 10/10
https://images/phase1.png

第 2 关：数组、循环、递推关系
关键汇编
phase_02 要求 6 个 u32（格式串 "%u %u %u %u %u %u"）。第 1 个数：

asm
mov -0x40(%rbp),%edi
shr $0x9,%rax
movzbl %al,%eax            # (challenge >> 9) & 0xFF
cmp %eax,%edi
循环（40217d～40219f）：

asm
40216f: mov $0xd,%r8d
402175: mov $0x3,%esi
40217a: sub %edx,%r8d
40217d: lea (%r8,%rdx,1),%ecx   # ecx = 13 + 4*(i-1)
402184: shr %cl,%rax            # challenge >> cl
402187: and $0xf,%eax           # & 0xF
40218a: add %edi,%eax           # + 上一项
40218c: mov 0x4(%rdx),%edi      # edi = v[i]
40218f: xor %esi,%eax           # ^ esi（3*i）
402191: cmp %eax,%edi
402195: add $0x3,%esi
402198: add $0x4,%rdx
40219c: cmp $0x12,%esi
40219f: jne 40217d
恢复出的计算关系
c
v[0] = (challenge >> 9) & 0xFF;
for (i = 1; i <= 5; i++) {
    cl = 13 + 4*(i-1);           // 13, 17, 21, 25, 29
    k  = (challenge >> cl) & 0xF;
    v[i] = (v[i-1] + k) ^ (3*i);
}
推理过程
格式串 6 个 %u，count=6。第 1 个数由 shr $0x9 + movzbl 得 (challenge>>9)&0xFF。循环里 ecx 初看恒 13，但 rdx 每轮 +4，cl 实际是 13,17,21,25,29。add %edi,%eax 用上一项（edi 在 mov 0x4(%rdx),%edi 之前），xor %esi 的 esi 每轮 +3，即 3*i。

调试中曾误以为 cl 恒 13，GDB 观察第 2 次 eax 与输入不符后修正。

验证
bash
./defuser --seed 42 --phase 2 | ./bomb --seed 42 --phase 2    # Phase 2 defused.
./defuser --seed 43 --phase 2 | ./bomb --seed 43 --phase 2    # 非公开 seed 通过
python3 public_test.py --phase 2                               # 10/10
https://images/phase2.png

第 3 关：switch 分支与跳转表
关键汇编
phase_03 要求 2 个 u32（"%u %u"）。第 1 个：

asm
and $0x7,%edi
cmp %edi,-0x18(%rbp)       # v[0] == challenge & 7
第 2 个由 p3_dispatch 计算：

asm
shr $0x3,%rax
shr $0x20,%rsi
xor %eax,%esi              # esi = (challenge>>32) ^ (challenge>>3)
call p3_dispatch
cmp %eax,-0x14(%rbp)
p3_dispatch 跳转表 0x405820（GDB x/8gx 0x405820）：

idx	表项	实际 case
0	0x402268	p3_case_6
1	0x402270	p3_case_7
2	0x402278	p3_case_2
3	0x402280	p3_case_0
4	0x402288	p3_case_1
5	0x402290	p3_case_5
6	0x402258	p3_case_4
7	0x402260	p3_case_3
8 个 case 的公式：

c
case 0: 3x + 0x4a34;          case 1: (x^0xe699) + (x>>2);
case 2: (2x) ^ 0x7c1b;        case 3: (x^0x2c4b) + (x>>1);
case 4: 5x + 0x8e70;          case 5: (2x) ^ 0xe13c;
case 6: 5x + 0x19627;         case 7: 5x + 0x1699a;
恢复出的计算关系
c
idx = challenge & 7;
x   = (uint32_t)((challenge >> 32) ^ (challenge >> 3));
v[0] = idx;
v[1] = p3_case_by_idx(idx, x);
p3_case_by_idx 按跳转表实际指向映射，不是按 case 编号。

推理过程
格式串 2 个 %u，count=2。p3_dispatch 按 challenge & 7 查表。跳转表项和 p3_case_N 编号不一一对应：索引 4 指向 p3_case_1，索引 6 指向 p3_case_4。最初按“idx N → case N”写，idx=4 时用错公式，GDB 看 eax=2704097841 和跳转表后发现索引 4 实际指向 p3_case_1（5x+0x8e70），修正后通过。

验证
bash
./defuser --seed 42 --phase 3 | ./bomb --seed 42 --phase 3    # Phase 3 defused.
./defuser --seed 43 --phase 3 | ./bomb --seed 43 --phase 3    # 非公开 seed 通过
python3 public_test.py --phase 3                               # 10/10
https://images/phase3.png

第 4 关：递归调用与返回过程
关键汇编
phase_04 要求 1 个 u32，调用递归函数 p4_walk：

asm
mov %r12d,%edi          # target = challenge & 0x7f
xor %esi,%esi           # lo = 0
mov $0x7f,%edx          # hi = 0x7f
shr $0x7,%rcx
shr $0x20,%rax
xor %eax,%ecx           # key = (challenge>>32) ^ (challenge>>7)
call p4_walk
cmp %eax,-0x14(%rbp)
p4_walk：

asm
mov %edx,%edi; sub %esi,%edi; shr $1,%edi; add %esi,%edi  # mid
cmp %eax,%edi; je 402380                                  # mid == target
jb 402360                                                 # target < mid
# 右：p4_walk(target, mid+1, hi, key); 5*r ^ (mid+key) ^ 0x7
# 左：p4_walk(target, lo, mid-1, key); 5*r ^ (mid+key) ^ 0x16
402380: lea 0x6f(%rdi),%eax; xor %ecx,%eax                # (mid+0x6f) ^ key
恢复出的计算关系
c
uint32_t p4_walk(target, lo, hi, key) {
    mid = lo + (hi - lo) / 2;
    if (mid == target) return (mid + 0x6f) ^ key;
    if (target < mid) {
        r = p4_walk(target, lo, mid - 1, key);
        return (5*r) ^ (mid + key) ^ 0x16;
    } else {
        r = p4_walk(target, mid + 1, hi, key);
        return (5*r) ^ (mid + key) ^ 0x7;
    }
}
入口参数：target = challenge & 0x7f，lo = 0，hi = 0x7f，key = (challenge>>32) ^ (challenge>>7)。

推理过程
格式串 "%u"，count=1。p4_walk 是二分查找递归：mid 相等时返回 (mid+0x6f)^key；target < mid 走左分支，返回 (5r)^(mid+key)^0x16；否则走右分支，返回 (5r)^(mid+key)^0x7。递归深度最多 log2(128)=7，不会爆栈。

验证
bash
./defuser --seed 42 --phase 4 | ./bomb --seed 42 --phase 4    # Phase 4 defused.
./defuser --seed 43 --phase 4 | ./bomb --seed 43 --phase 4    # 非公开 seed 通过
python3 public_test.py --phase 4                               # 10/10
https://images/phase4.png

第 5 关：结构体布局、指针、链表遍历
关键汇编
phase_05 要求 2 个 u32，调用 p5_traverse：

asm
lea -0x14(%rbp),%rsi
mov %r12,%rdi
call p5_traverse
cmp %edx,-0x1c(%rbp)       # v[0] == 返回值
cmp %eax,-0x18(%rbp)       # v[1] == *out
p5_traverse：

asm
and $0x7,%eax
mov 0x405b60(,%rax,8),%rdx  # 链表头 = p5_heads[challenge & 7]
shr $0x6,%rax; shr $0x20,%rdi; xor %edi,%eax; xor (%rdx),%eax; xor $0x8574,%eax
add %esi,%eax               # += p->f1
mov %esi,(%r8)              # *out = p->f1
and $0x3,%edi               # steps = (challenge >> 3) & 3
# 循环：p = p->next; acc ^= p->f0; acc += p->f1; *out = p->f1
节点结构（从 .data 读出）：

c
struct p5_node { uint32_t f0; uint32_t f1; const struct p5_node *next; };
p5_heads 8 个链表头（地址 0x405b60），12 个节点在 0x405ba0～0x405c50，共用节点，起点不同。

恢复出的计算关系
c
uint32_t p5_traverse(challenge, *out) {
    idx = challenge & 7;
    p = p5_heads[idx];
    acc = (challenge>>6) ^ (challenge>>32) ^ p->f0 ^ 0x8574;
    acc += p->f1;
    *out = p->f1;
    steps = (challenge >> 3) & 3;
    for (i = 1; i <= steps; i++) {
        p = p->next;
        acc ^= p->f0;
        acc += p->f1;
        *out = p->f1;
    }
    return acc;
}
推理过程
格式串 2 个 %u，count=2。p5_traverse 用 challenge & 7 选链表头，steps = (challenge>>3)&3 决定走几步。节点数据用 GDB x/2wx + x/1gx 从 bomb 的 .data 读出，硬编码进 defuser.c。节点定义按链表尾到头顺序，避免前向引用。

验证
bash
./defuser --seed 42 --phase 5 | ./bomb --seed 42 --phase 5    # Phase 5 defused.
./defuser --seed 43 --phase 5 | ./bomb --seed 43 --phase 5    # 非公开 seed 通过
python3 public_test.py --phase 5                               # 10/10
https://images/phase5.png

第 6 关：函数指针表与间接调用
关键汇编
phase_06 要求 1 个 u32，调用 p6_dispatch：

asm
mov %r12,%rdi
call p6_dispatch
cmp %eax,-0x14(%rbp)
p6_dispatch：

asm
shr $0x6,%rdi
shr $0x20,%rax
xor %eax,%edi              # edi = (challenge>>6) ^ (challenge>>32)
and $0x38,%ebx             # ebx = challenge & 0x38（字节偏移）
and $0x7,%eax              # eax = challenge & 7
call *0x405e00(,%rax,8)    # 第一次：f[challenge & 7]
xor $0xe194,%eax
mov %eax,%edi
call *0x405e00(%rbx)       # 第二次：f[(challenge & 0x38) >> 3]
add $0xcae4,%eax
函数指针表 0x405e00（GDB x/8gx）：

idx	地址	函数
0,1,2	0x402550	p6_op_5
3	0x402530	p6_op_3
4	0x402540	p6_op_4
5	0x4024e0	p6_op_0
6	0x402520	p6_op_2
7	0x402500	p6_op_1
6 个函数：

c
p6_op_0: ((x>>5)^x) + 0x2150;       p6_op_1: (2x+0x5fb2) ^ (3x);
p6_op_2: (x+0x8ae) ^ (x>>1);        p6_op_3: rol32(x,7) ^ 0xe9f0;
p6_op_4: ror32(x,1);                p6_op_5: (x^0xb034) + (x<<7);
恢复出的计算关系
c
idx1 = challenge & 7;
idx2 = (challenge & 0x38) >> 3;
x    = (challenge >> 6) ^ (challenge >> 32);
r = p6_op(idx1, x);
r ^= 0xe194;
r = p6_op(idx2, r);
r += 0xcae4;
关键：第二次调用用 idx2 = (challenge & 0x38) >> 3，不是 idx1。0x38 = 7 << 3，challenge & 0x38 取第 3～5 位，当字节偏移用。

推理过程
格式串 "%u"，count=1。第一次调用用 challenge & 7 索引，第二次用 challenge & 0x38 当字节偏移，即索引 (challenge & 0x38) >> 3。最初误以为两次索引相同，完整流程第 6 关 BOOM；GDB 直接调 p6_dispatch 得 1699549366，Python 按同索引算得 1189984710，发现第二次索引是 3 不是 5，修正后通过。

验证
bash
./defuser --seed 42 --phase 6 | ./bomb --seed 42 --phase 6    # Phase 6 defused.
./defuser --seed 43 --phase 6 | ./bomb --seed 43 --phase 6    # 非公开 seed 通过
python3 public_test.py --phase 6                               # 10/10
https://images/phase6.png

Honor（Secret 关）
关键汇编
secret_gate(input, challenge, context)：

asm
call strlen
sub $0x24,%rax          # strlen - 36
cmp $0x7f,%rax
jbe 402770              # 必须 36 <= strlen <= 163
call secret_copy
cmpl $0x4e45504f,-0xb0(%rbp)  # 前 4 字节 == "OPEN"
jne 失败
call secret_phase(&buf[4], challenge, context)
secret_phase 要求 3 个 u32（"%u %u %u"）：

asm
# 第 1 个：a == context & 3
# 第 2 个：b == secret_mix(x ^ y) ^ w
# 第 3 个：c == b ^ (y + c0 + 0xf22a + (x >> 5))
其中 secret_mix(v) = ror32(v, 5) ^ 0x7416，x = challenge ^ (challenge>>32)，y = context ^ (context>>32)，z = context >> 16，w = (0x863c * c0) ^ z。

恢复出的计算关系
c
c0 = context & 3;
x  = (uint32_t)(challenge ^ (challenge >> 32));
y  = (uint32_t)(context ^ (context >> 32));
z  = (uint32_t)(context >> 16);
w  = (0x863c * c0) ^ z;
b  = (ror32(x ^ y, 5) ^ 0x7416) ^ w;
c2 = b ^ (y + c0 + 0xf22a + (x >> 5));
输出格式："OPEN" + 填充到总长 ≥ 36 + 三个整数。

推理过程
secret_gate 检查输入前 4 字节 "OPEN"，且 strlen 在 36～163 之间。secret_phase 用 challenge 和 context 算 3 个整数。context 由前六关口令得到，defuser --secret 会先调六关求解函数算出。最初输出 "OPEN %u %u %u" 长度 29，不满足 strlen >= 36，改为 "OPEN%*s%u %u %u"（%*s 宽度 32）后长度 60，通过长度检查。

验证
bash
{ ./defuser --seed 42; ./defuser --secret --seed 42; } | ./bomb --seed 42
# Secret phase defused.

python3 public_test.py --secret
# Secret public tests: 10/10
https://images/honor.png

参考资料与工具使用
CS:APP x86-64 GDB 速查表：http://csapp.cs.cmu.edu/3e/docs/gdbnotes-x86-64.pdf

GDB 官方文档：https://www.sourceware.org/gdb/documentation/

objdump 手册（man objdump）

课程 BombLab 实验文档

辅助工具：使用 AI 工具辅助理解 lea 的算术含义、跳转表索引与 case 编号的对应关系、以及 p6_dispatch 两次调用索引不同的问题。所有公式和代码均通过 GDB 在线核对与公开测试验证，能独立解释。