# BombLab
[toc]
# Knowlegde pointers

* assembly language 
* tools:
    * gdb
    * objdump 

# Solutions 


> this solutions is for reference only, please complete the expreiment independently.

![p1](https://i.loli.net/2021/02/03/8uMGNTgilfnYRPC.png)
phase4 and phase5 answers are not unique.

please check `./solve.txt`


> tips:
> * `objdump --disassemble=<func> -M intel  ./bomb`

## phase 1

explicit comparison 

## phase 2


read 6 numbers, which we write as `arr[6]`, and `arr[0] = 1`, `arr[i] = 2 * arr[i-1]`,

## phase 3 

read 2 numbers, which we write as `arr[2]`, and `arr[0] = 7`, `arr[1] = 0x147`, 

` jmp    QWORD PTR [rax*8+0x402470]`, there are jumps in the form of `tables[i]`, which can be followed by debugging or by looking at memory and calculating it.

## phase 4
read 2 numbers, `numb1, numb2`, 

restricted is `numb1<0xe`, `numb2=0`, `func4(numb1)=0`, 

```python
def func4(a1, a2, a3):
	a = a3 - a2;
	c = a >> 0x1f 
	a += c 
	a = a >> 1
	c = a + a2 

	if(c > a1): 
		a = func4(a1, a2, c-1)
		return (a*2)
	else:
		a = 0
		if(c >= a1):
			return a

		a = func4(a1, c+1, a3);
		return a
```

only `(c <= a1) && (c >= a1)`, `c=a1` can run the `a = 0; return a;`.

no matter how many times you recurse, you can return 0 only when `a1=c`, which is satisfied when `a1=1` or `a1=0`, 


## phase 5 

the length of the string is 6, take the lower bit of each character and index it in `table1`, and then compare the pieced string with `str_target`, 

````python
table1 = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"`

str_target = "flyers"
````

## phase 6 

read 6 numbers, range from 0 to 6 and are not equal to each other, 

each number (x) is taken as `7-x`, and then the `node` arrays are arranged in order, 

and associate them in the order before and after.

**then compare** the size of the data in the node, which must be sorted by size.

![p2](https://i.loli.net/2021/02/03/BVEslq2NGk9R3Cv.png)
