# Leesp  
A Lisp-like implementation written in C. 

# To Compile
On Linux and Mac
```
cc -std=c99 -Wall main.c mpc/mpc.c -ledit -lm -o leesp
```
On Windows
```
cc -std=c99 -Wall main.c mpc/mpc.c -o leesp
```

# Arithmetic operators
Leesp uses Polish Notation (prefix notation) for mathematical sequences. 
```
leesp> + 5 3
8

leesp> - 8 2
6

leesp> * 3 5
15

leesp> / 8 4
2

leesp> + 4 (* 3 2) (- 9 (/ 12 2))
13
```

# S-Expressions
Lisp-like symbolic expressions. These are simply one or more expressions inside parentheses.
```
leesp> (+ 2 2)
4
```

# Q-Expressions
Quoted expressions to mimic some functionality of Lisp macros. When evaluated, they are left as is and not handled by normal mechanisms. This allows code to be written, and be passed as data itself.
```
leesp> {+ 2 2}
{+ 2 2}
```

# Built in functions
Leesp has multiple built in functions that are always available, listed below in alphabetical order.

## eval
Takes a Q-Expressions and evaluates it as a S-Expression
```
leesp> eval {+ 1 2}
3
```

## def
Define a variable
```
leesp> def {a} 2
()
leesp> a
2
leesp> def {b c} 3 4
()
leesp> + a b c 1
10
```

## head
Takes a Q-Expression and returns a new Q-Expression containing only the first element
```
leesp> head {8 10 12}
{8}
```

## join
Takes one or more Q-Expressions and returns a Q-Expressions of them joined together
```
leesp> join {1 2} {3 4}
{1 2 3 4}
```

## list
Takes one ore more arguments and returns a new Q-Expression containing the arguments
```
leesp> list 1 2 3
{1 2 3}
```

## tail
Takes a Q-Expression and returns a new Q-Expression with the first element removed
```
leesp> tail {7 8 9}
{8 9}
```

## user defined functions
Combining the built in `def` function with the `\` lambda function allows a user to define their own functions.
```
leesp> def {double} (\ {x} {* x 2})
()
leesp> double 5
10
```
Leesp functions support partial evaluation.
```
leesp> def {multiply} (\ {x y} {* x y})
()
leesp> multiply
(\ {x y} {* x y})
leesp> multiply 2 4
8
leesp> def {double} (multiply 2)
()
leesp> double
(\ {y} {* x y})
leesp> double 8
16
```
