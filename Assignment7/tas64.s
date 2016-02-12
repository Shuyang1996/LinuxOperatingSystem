# 1 "tas64.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "tas64.S"
 .text
.globl tas
 .type tas,@function
tas:
 pushq %rbp
 movq %rsp, %rbp
 movq $1, %rax
#APP
 lock;xchgb %al,(%rdi)
#NO_APP
 movsbq %al,%rax
 pop %rbp
 ret
.Lfe1:
 .size tas,.Lfe1-tas
