# func
Combining the built in `def` function with the `\` lambda function allows a user to define their own functions. This is assinged to the `func` keyword.
```
leesp> def {func} (\ {args body} {def (head args) (\ (tail args) body)})
()
leesp> func {multiply x y} {* x y}
()
leesp> multiply 2 4
8
```
