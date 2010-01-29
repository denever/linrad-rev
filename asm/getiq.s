
section .text

compress_rawdat:
  push ebx
  push ecx
  push edx
  push esi
  push edi
  mov  esi,[timf1_char]
  mov  edi,[rawsave_tmp]
  add  esi,[timf1p_pa]
  mov  ebx,[rx_read_bytes]
; Compress 4 byte words (int) to 18 bit.
cmpr:
  mov  ax,[esi+2]      ;byte 2,3  of word 0
  mov  ch,[esi+1]      ;byte 1 of word 0
  mov  [edi],ax
  shr  ch,2
  mov  ax,[esi+6]      ;byte 2,3  of word 1
  mov  cl,[esi+5]      ;byte 1 of word 1
  mov  [edi+2],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[esi+10]      ;byte 2,3  of word 2
  mov  cl,[esi+ 9]      ;byte 1 of word 2
  mov  [edi+4],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[esi+14]      ;byte 2,3  of word 3
  mov  cl,[esi+13]      ;byte 1 of word 3
  and  cl,11000000B
  or   ch,cl
  mov  [edi+6],ax
  mov  [edi+8],ch
  add  esi,16
  add  edi,9
  sub  ebx,16
  ja   cmpr
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret


expand_rawdat:
  push ebx
  push ecx
  push edx
  push esi
  push edi
  mov  esi,[timf1_char]
  mov  edi,[rawsave_tmp]
  add  esi,[timf1p_pa]
  mov  ebx,[rx_read_bytes]
; Expand 18 bit packed data.
; add 0.5 bits (=bit 19) to correct for the average error
; of 0.5 bits that we introduced by truncating to 187 bits.
; This will remove the spur at frequency=0 that would otherwise
; have been introduced
expnd:
  mov  eax,[edi-2]    
  mov  ecx,[edi+6]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi],eax
  mov  eax,[edi]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi+4],eax
  mov  eax,[edi+2]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi+8],eax
  mov  eax,[edi+4]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi+12],eax
  add  esi,16
  add  edi,9
  sub  ebx,16
  ja   expnd
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret





