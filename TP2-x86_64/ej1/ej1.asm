; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data
; (Aquí se pueden definir constantes o strings de formato si se requirieran)

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares:
extern malloc
extern free
extern str_concat
extern strlen
extern strcpy
extern strcat

; ----------------------------------------------------------------------------
; string_proc_list_create_asm:
;   C equivalente:
;     string_proc_list* string_proc_list_create(void){
;         string_proc_list* list = malloc(16);
;         if(list == NULL) return NULL;
;         list->first = NULL;
;         list->last  = NULL;
;         return list;
;     }
; ----------------------------------------------------------------------------
string_proc_list_create_asm:
    push rbp
    mov rbp, rsp
    mov edi, 16             ; size of string_proc_list (2 pointers)
    call malloc
    test rax, rax
    je .return_null_list
    ; Inicializa list->first y list->last en NULL.
    mov qword [rax], 0
    mov qword [rax+8], 0
    mov rsp, rbp
    pop rbp
    ret
.return_null_list:
    mov rsp, rbp
    pop rbp
    ret

; ----------------------------------------------------------------------------
; string_proc_node_create_asm:
;   C equivalente:
;     string_proc_node* string_proc_node_create(uint8_t type, char* hash){
;         string_proc_node* node = malloc(32);
;         if(node == NULL) return NULL;
;         node->next      = NULL;
;         node->previous  = NULL;
;         node->hash      = hash;
;         node->type      = type;
;         return node;
;     }
;   Notar que el primer parámetro (type) llega en dil y el segundo (hash) en rsi.
; ----------------------------------------------------------------------------
string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    ; Guardar el valor de type (uint8_t) en r12 (r12_8 = dil)
    movzx r12, dil
    mov edi, 32             ; tamaño de string_proc_node
    call malloc
    test rax, rax
    je .return_null_node
    ; Inicializar los campos:
    mov qword [rax], 0      ; node->next = NULL
    mov qword [rax+8], 0    ; node->previous = NULL
    mov qword [rax+16], rsi ; node->hash = hash (segundo parámetro)
    mov byte [rax+24], r12b   ; node->type = type
    mov rsp, rbp
    pop rbp
    ret
.return_null_node:
    mov rsp, rbp
    pop rbp

    ret

; ----------------------------------------------------------------------------
; string_proc_list_add_node_asm:
;   C equivalente:
;     void string_proc_list_add_node(string_proc_list* list,
;                                     uint8_t type, char* hash){
;         node = string_proc_node_create(type, hash);
;         if(node == NULL) return;
;         if(list->first == NULL){
;             list->first = node;
;             list->last  = node;
;         } else {
;             list->last->next = node;
;             node->previous = list->last;
;             list->last = node;
;         }
;     }
;   Parámetros:
;     RDI: list, RSI: type, RDX: hash.
; ----------------------------------------------------------------------------
string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    ; Conserva el puntero a la lista en RCX.
    mov rcx, rdi            ; rcx = list
    ; Guardo los parametros para despues pasarselos a string_proc_node_create_asm
    movzx rdi, sil            ; rdi = type
    mov rsi, rdx            ; rsi = hash
    ; Llamar a string_proc_node_create_asm(type, hash):
    call string_proc_node_create_asm
    test rax, rax
    je .return_null_node ; si node es NULL, retorna
    ; Guardo el puntero a node en 128
    mov r12, rax            ; r12 = node
    ; Si list->first es NULL:
    mov r13, [rcx]        ; r13 = list->first
    test r13, r13
    je .first_node_in_list_is_null
    ; Si list->first no es NULL:
    ; list->last->next = node
    mov r14, [rcx+8]       ; r14 = list->last
    mov qword [r14], r12    ; list->last->next = node
    ; node->previous = list->last
    mov qword [r12+8], r14  ; node->previous = list->last
    ; list->last = node
    mov qword [rcx+8], r12  ; list->last = node
    jmp .done_add_node
.first_node_in_list_is_null:
    ; Si list->first es NULL:
    ; list->first = node
    mov qword [rcx], r12    ; list->first = node
    ; list->last = node
    mov qword [rcx+8], r12  ; list->last = node
    jmp .done_add_node
.return_null_node:
    ; Si node es NULL, retorna.
    mov rsp, rbp
    pop rbp
    ret
.done_add_node:
    mov rsp, rbp
    pop rbp
    ret

; ----------------------------------------------------------------------------
; string_proc_list_concat_asm:
;   C equivalente:
;     char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash){
;         string_proc_node* current_node = list->first;
;         char* result = NULL;
;         while(current_node != NULL){
;             if(current_node->type == type){
;                 if(result == NULL){
;                     result = malloc(strlen(hash) +
;                                     strlen(current_node->hash) + 1);
;                     strcpy(result, hash);
;                     strcat(result, current_node->hash);
;                 } else {
;                     char* temp = str_concat(result, current_node->hash);
;                     free(result);
;                     result = temp;
;                 }
;             }
;             current_node = current_node->next;
;         }
;         return result;
;     }
;   Parámetros:
;     RDI: list, RSI: type, RDX: hash.
;   Se utiliza un espacio local de 24 bytes para:
;     [rbp-8]  : result (char*)
;     [rbp-16] : current_node (string_proc_node*)
;     [rbp-24] : hash original (guardado para llamadas a strlen, strcpy, strcat)
; ----------------------------------------------------------------------------
string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    ; Conserva el puntero a la lista en RCX.
    mov rcx, rdi            ; rcx = list
    ; Parametros
    ; sil = type
    ; Me guardo el puntero a hash en r15
    mov r15, rdx            ; r15 = hash
    ; rdx = hash
    ; string_proc_node* current_node = list->first;z
    mov r12 , [rcx]        ; r12 = current_node = list->first
    ; char* result = NULL;
    xor r13, r13            ; r13 = result = NULL
    ; char* hash_original = hash;
    mov rax, rdx          ; rax = hash_original
    ; while(current_node != NULL)
    . while_loop:
    ; if(current_node->type == type)
    movzx r14, byte [r12+16] ; r14 = current_node->type
    cmp r14, sil
    jne .next_node
    ; if(result == NULL)
    test r13, r13
    je .result_is_null
    ; result != NULL
    ; char* temp = str_concat(result, current_node->hash);
    push rax
    ; Alineo el stack
    sub rsp, 8
    mov rdi, r13            ; rdi = result
    mov rsi, [r12+16] ; rsi = current_node->hash
    call str_concat
    mov r14, rax       ; r14 = temp
    ; free(result);
    call free
    ; result = temp;
    mov r13, r14
    jmp .done_while_loop
.result_is_null:
    ; result = malloc(strlen(hash) + strlen(current_node->hash) + 1);
    mov rdi, 






    
    

