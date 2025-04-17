; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern str_concat

string_proc_list_create_asm:
    push rbp
    mov rbp, rsp
    
    mov rdi, 16
    call malloc
    
    test rax, rax
    jz .error
    
    mov qword [rax], NULL    ; first
    mov qword [rax + 8], NULL ; last
    
    pop rbp
    ret
    
.error:
    xor rax, rax
    pop rbp
    ret

string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    
    push rdi 
    push rsi 
    mov rdi, 32
    call malloc
    
    pop rsi
    pop rdi
    
    test rax, rax
    jz .error
    
    mov qword [rax], NULL       ; next
    mov qword [rax + 8], NULL   ; previous
    mov byte [rax + 16], dil    ; type 
    mov qword [rax + 24], rsi   ; hash
    
    pop rbp
    ret
    
.error:
    xor rax, rax
    pop rbp
    ret

string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    
    ; Save arguments
    mov rbx, rdi
    mov r12b, sil
    mov r13, rdx
    
    movzx edi, r12b
    mov rsi, r13
    call string_proc_node_create_asm
    
    test rax, rax
    jz .end
    
    cmp qword [rbx], NULL
    jne .not_empty
    
    mov [rbx], rax      ; first = node
    mov [rbx + 8], rax  ; last = node
    jmp .end
    
.not_empty:
    mov rcx, [rbx + 8]  ; last node
    mov [rax + 8], rcx  ; new node->previous = last
    mov [rcx], rax      ; last->next = new node
    mov [rbx + 8], rax  ; last = new node
    
.end:
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

my_strlen:
    push rbp
    mov rbp, rsp
    xor rax, rax    ; Initialize counter to 0
    
.loop:
    cmp byte [rdi + rax], 0
    je .done
    inc rax
    jmp .loop
    
.done:
    pop rbp
    ret

string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov rbx, rdi        ; list
    mov r12b, sil       ; type
    mov r13, rdx        ; hash
    
    mov rdi, r13
    call my_strlen     
    
    add rax, 1          
    mov rdi, rax
    call malloc
    
    test rax, rax
    jz .end
    
    mov r14, rax        ; r14 = result
    mov rsi, r13        ; source = hash
    mov rdi, r14        ; destination = result
    
.copy_loop:
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .copy_done
    inc rsi
    inc rdi
    jmp .copy_loop
    
.copy_done:
    mov r15, [rbx]      ; current_node = list->first
    
.loop:
    test r15, r15
    jz .end             ; if current_node == NULL, done
    
    movzx eax, byte [r15 + 16]  ; current_node->type
    cmp al, r12b
    jne .next
    
    mov rdi, r14
    mov rsi, [r15 + 24] ; current_node->hash
    call str_concat
    
    mov rdi, r14
    mov r14, rax       
    call free
    
.next:
    mov r15, [r15]      ; current_node = current_node->next
    jmp .loop
    
.end:
    mov rax, r14        ; return result
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    add rsp, 16
    pop rbp
    ret