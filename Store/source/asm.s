.intel_syntax noprefix
.text

.global    cpu_enable_wp
.global    cpu_disable_wp


cpu_enable_wp:
  mov rax, cr0
  or rax, 0x10000
  mov cr0, rax
  ret

cpu_disable_wp:
  mov rax, cr0
  and rax, ~0x10000
  mov cr0, rax
  ret

  .intel_syntax noprefix
.text

.global syscall, syscall1, syscall2, syscall3, syscall4 ,Sysctl, Fork


Fork:
  push rbp
  mov rbp,rsp
  
  mov rax,2 
  syscall
  
  leave
  ret

Sysctl:
  push rbp
  mov rbp,rsp
  
  mov rax,202  
  mov r10, rcx
  syscall
  
  leave
  ret



syscall:
    mov rax,0
    mov r10,rcx
    syscall
    jb err
    ret

syscall1:
    mov rax,rdi
    mov rdi,rsi
    syscall
    ret

syscall2:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    syscall
    ret

syscall3:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    syscall
    ret

syscall4:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    mov r10,r8
    syscall
    ret


err:
    push rax
    call __error
    pop rcx
    mov [rax], ecx
    mov rax,-1
    mov rdx,-1
    ret



