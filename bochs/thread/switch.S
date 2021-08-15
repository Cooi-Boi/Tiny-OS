[bits 32]
section .text
global switch_to
switch_to:
    push esi            ;这里是根据ABI原则保护四个寄存器 放到栈里面
    push edi
    push ebx
    push ebp
    
    mov eax,[esp+20]    ;esp+20的位置是cur cur的pcb赋值给eax
    mov [eax],esp       ;[eax]为pcb的内核栈指针变量 把当前环境的esp值记录下来
    
    mov eax,[esp+24]
    mov esp,[eax]       ;把要切换的线程的pcb 内核栈esp取出来

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret                 ;这里的返回地址为 kernel_thread的地址
