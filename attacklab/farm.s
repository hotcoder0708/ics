
farm.o:     file format elf32-i386


Disassembly of section .text:

00000000 <start_farm>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	b8 01 00 00 00       	mov    $0x1,%eax
   8:	5d                   	pop    %ebp
   9:	c3                   	ret    

0000000a <getval_462>:
   a:	55                   	push   %ebp
   b:	89 e5                	mov    %esp,%ebp
   d:	b8 48 89 c7 c3       	mov    $0xc3c78948,%eax
  12:	5d                   	pop    %ebp
  13:	c3                   	ret    

00000014 <setval_442>:
  14:	55                   	push   %ebp
  15:	89 e5                	mov    %esp,%ebp
  17:	8b 45 08             	mov    0x8(%ebp),%eax
  1a:	c7 00 51 58 90 90    	movl   $0x90905851,(%eax)
  20:	5d                   	pop    %ebp
  21:	c3                   	ret    

00000022 <addval_163>:
  22:	55                   	push   %ebp
  23:	89 e5                	mov    %esp,%ebp
  25:	8b 45 08             	mov    0x8(%ebp),%eax
  28:	2d b8 76 38 6d       	sub    $0x6d3876b8,%eax
  2d:	5d                   	pop    %ebp
  2e:	c3                   	ret    

0000002f <addval_287>:
  2f:	55                   	push   %ebp
  30:	89 e5                	mov    %esp,%ebp
  32:	8b 45 08             	mov    0x8(%ebp),%eax
  35:	2d a8 6f 6f 6f       	sub    $0x6f6f6fa8,%eax
  3a:	5d                   	pop    %ebp
  3b:	c3                   	ret    

0000003c <setval_385>:
  3c:	55                   	push   %ebp
  3d:	89 e5                	mov    %esp,%ebp
  3f:	8b 45 08             	mov    0x8(%ebp),%eax
  42:	c7 00 48 89 c7 c3    	movl   $0xc3c78948,(%eax)
  48:	5d                   	pop    %ebp
  49:	c3                   	ret    

0000004a <addval_248>:
  4a:	55                   	push   %ebp
  4b:	89 e5                	mov    %esp,%ebp
  4d:	8b 45 08             	mov    0x8(%ebp),%eax
  50:	2d b8 76 38 6b       	sub    $0x6b3876b8,%eax
  55:	5d                   	pop    %ebp
  56:	c3                   	ret    

00000057 <getval_226>:
  57:	55                   	push   %ebp
  58:	89 e5                	mov    %esp,%ebp
  5a:	b8 78 90 90 90       	mov    $0x90909078,%eax
  5f:	5d                   	pop    %ebp
  60:	c3                   	ret    

00000061 <setval_247>:
  61:	55                   	push   %ebp
  62:	89 e5                	mov    %esp,%ebp
  64:	8b 45 08             	mov    0x8(%ebp),%eax
  67:	c7 00 e3 33 58 c1    	movl   $0xc15833e3,(%eax)
  6d:	5d                   	pop    %ebp
  6e:	c3                   	ret    

0000006f <mid_farm>:
  6f:	55                   	push   %ebp
  70:	89 e5                	mov    %esp,%ebp
  72:	b8 01 00 00 00       	mov    $0x1,%eax
  77:	5d                   	pop    %ebp
  78:	c3                   	ret    
