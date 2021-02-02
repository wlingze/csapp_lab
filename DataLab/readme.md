# datalab

[toc]

# Related chapter

*csapp chapter 2* :  *Representing and Manipulating Information*

# Knowledge pointers


## bit-level oprations 

* oprations : & ^ ~ 
* unsigned integer and signed integer conversion.
* binary complement and negative numbers.

## floating point 

* normalized value
* denormalized value
* special value (NaN and infinity)


# Solutions

> this question is for reference only, please complete the experiment independently.

![p1](https://i.loli.net/2021/02/02/Gc3mvrZ4kEjxoJh.png)

![p2](https://i.loli.net/2021/02/02/ApOUmskI4fBNYPe.png)


## bitXor

`a^b = (~a&b) | (~b&a)` and `a | b = ~(~a & ~b)` 

> it can be considered through the set.

## tmin 

signed numbers, the heighest bit is negative, the others are positive. 

so the other bits is 0, and the heighest is 1.

## isTmax

tmax is the highest bits is 0 and the other bits is 1;

`tmax+1 = tmin`, `tmin+tmin=0`(overflow), 

`tmax+tmax+2 = 0`, but `0xffffffff` has the same characteristics,

excluding this special case: `tmax+1=tmin!=0` but `0xffffffff+1=0`, 

## allOddBits 

first construct `0xAAAAAAAA`, 

then use `&` intercept the odd-bit, and use `^` to compare.

## negate 

feature of two's complement.

`-x = ~x+1` 

## isAsciiDigit 

using the tow's complement : `a - b = a + (~b+1)`, 

using the sign bits : `a > 0 => (a >> 31) & 1`, 

realization `0x30 <= x < 0x3a`, 


## conditional

the selection can be achieved by `&` and `|`, 

convert `0\1` to `0xffffffff\0` by `(!!x) - 1`, 

## isLessOrEqual 

it is similar to the idea of `isAsciiDigit`, 

however, if the symbols do not match, you can judge directly.

then the two judgments are connected by `|` 

## logicalNeg 

> how to determine 0;

`x` is not a negative number, `x-1` is a negative number, `x=0`.

## howManyBits 

use the idea of dichotomy to intercept bit.

## floatScale2 

`2*f` you can modify `E` to achieved this.

pay attention to the handling of special cases.

## floatFloat2Int 

convert from floating point to integer, paying attention to the size boundary of the data.

## floatPower2 
expression `2.0^x`, 

x is `E`, corvert to exp, and construct `M=1` (`frac=0`), 

note the special case, (`exp<0`, `exp>0xff`)


