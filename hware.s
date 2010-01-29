section .text
;

     global check_mmx
     global _check_mmx


check_mmx: 
_check_mmx:

 push ebx
 push ecx
 push edx
 pushfd
 pop  eax           ;get EFLAGS into eax
 mov  edx,eax
;         10987654321098765432109876543210
 xor  eax,00000000001000000000000000000000B
 push eax
 popfd
 pushfd
 pop  eax
 xor  eax,edx
 and  eax,00000000001000000000000000000000B
 jz   chkcpux    ; if we can not change ID flag, cpuid not supported 
 xor  eax,eax
 cpuid
 dec  eax
 jns  id_ok
 xor  eax,eax    ; Not Pentium or above
 jmp  chkcpux
id_ok: mov  eax,1
 cpuid
 and  edx,11100000000000000000000000B
 mov  eax,edx
 shr  eax,23 
chkcpux: pop edx
 pop  ecx
 pop  ebx
 ret