# MIPT-JIT-Compiler
> Just-in-time arithmetic expressions compiler for ARM processors

## Description

The Compiler involves recursive descent parser used for analysis of arithmetic expression that may contain:

1. Integer constants
2. Names of extern integer variables
3. Names of integer functions which can take from 0 to 4 arguments
4. `+`, `-`, `*` operators
5. Subexpressions in brackets

All spaces are ignored.

To compile expression you should pass the following arguments to `jit_compile_expression_to_arm` function in `compiler.cpp`:

- `expression` - expression to calculate
- `externs` - array of struct's that describes extern variables and functions. (The last element should have zero fields: `.name`=0 and `.pointer`=0)
- `out_buffer` - pointer to the allocated part of memory which contains the machine code of the program. This program calculates the result of expression.

`arm-linux-gnueabi-gcc` and `qemu-arm` are required to run the compiler.

#### Build example:

```bash
arm-linux-gnueabi-gcc -lstdc++ -marm -test.c compiler.cpp -o compiler
```

#### Execution example:
```bash
qemu-arm -L $LINARO_SYSROOT compiler < input_file.txt > output_file.txt
```

#### Input format
```
.vars
ab=1 a=234 y=-1 zsdf=0
.expression
dec((-3))-sum((-28), ((-ab)*(-y)))-((-y)+dec((-a)))+5-(-a)+(-20)
```

(This functions are declared in `test.c` file)

