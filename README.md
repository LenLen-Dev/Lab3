# 算符优先分析（OPG）
- 读取文法（input.txt）
- 求解FirstVT & LastVT
- 构建优先关系表
- 移进规约操作
 ## `input.txt`文件参考
>S->#E#<br>
E->E+T|T<br>
T->T*F|F<br>
F->P^F|P<br>
P->(E)|i
