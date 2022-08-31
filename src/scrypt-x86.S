# Copyright 2011 pooler@litecoinpool.org
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

#if defined(OPTIMIZED_SALSA) &&  defined(__i386__)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

#define gen_salsa8_core_quadround() \
	movl	52(%esp), %ecx; \
	movl	4(%esp), %edx; \
	movl	20(%esp), %ebx; \
	movl	8(%esp), %esi; \
	leal	(%ecx, %edx), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 4(%esp); \
	movl	36(%esp), %edi; \
	leal	(%edx, %ebx), %ebp; \
	roll	$9, %ebp; \
	xorl	%ebp, %edi; \
	movl	24(%esp), %ebp; \
	movl	%edi, 8(%esp); \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	40(%esp), %ebx; \
	movl	%ecx, 20(%esp); \
	addl	%edi, %ecx; \
	roll	$18, %ecx; \
	leal	(%esi, %ebp), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 24(%esp); \
	movl	56(%esp), %edi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %edi; \
	movl	%edi, 36(%esp); \
	movl	28(%esp), %ecx; \
	movl	%edx, 28(%esp); \
	movl	44(%esp), %edx; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %esi; \
	movl	60(%esp), %ebx; \
	movl	%esi, 40(%esp); \
	addl	%edi, %esi; \
	roll	$18, %esi; \
	leal	(%ecx, %edx), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 44(%esp); \
	movl	12(%esp), %edi; \
	xorl	%esi, %ebp; \
	leal	(%edx, %ebx), %esi; \
	roll	$9, %esi; \
	xorl	%esi, %edi; \
	movl	%edi, 12(%esp); \
	movl	48(%esp), %esi; \
	movl	%ebp, 48(%esp); \
	movl	64(%esp), %ebp; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	16(%esp), %ebx; \
	movl	%ecx, 16(%esp); \
	addl	%edi, %ecx; \
	roll	$18, %ecx; \
	leal	(%esi, %ebp), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	32(%esp), %edi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %edi; \
	movl	%edi, 32(%esp); \
	movl	%ebx, %ecx; \
	movl	%edx, 52(%esp); \
	movl	28(%esp), %edx; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %esi; \
	movl	40(%esp), %ebx; \
	movl	%esi, 28(%esp); \
	addl	%edi, %esi; \
	roll	$18, %esi; \
	leal	(%ecx, %edx), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 40(%esp); \
	movl	12(%esp), %edi; \
	xorl	%esi, %ebp; \
	leal	(%edx, %ebx), %esi; \
	roll	$9, %esi; \
	xorl	%esi, %edi; \
	movl	%edi, 12(%esp); \
	movl	4(%esp), %esi; \
	movl	%ebp, 4(%esp); \
	movl	48(%esp), %ebp; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	16(%esp), %ebx; \
	movl	%ecx, 16(%esp); \
	addl	%edi, %ecx; \
	roll	$18, %ecx; \
	leal	(%esi, %ebp), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 48(%esp); \
	movl	32(%esp), %edi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %edi; \
	movl	%edi, 32(%esp); \
	movl	24(%esp), %ecx; \
	movl	%edx, 24(%esp); \
	movl	52(%esp), %edx; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %esi; \
	movl	28(%esp), %ebx; \
	movl	%esi, 28(%esp); \
	addl	%edi, %esi; \
	roll	$18, %esi; \
	leal	(%ecx, %edx), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 52(%esp); \
	movl	8(%esp), %edi; \
	xorl	%esi, %ebp; \
	leal	(%edx, %ebx), %esi; \
	roll	$9, %esi; \
	xorl	%esi, %edi; \
	movl	%edi, 8(%esp); \
	movl	44(%esp), %esi; \
	movl	%ebp, 44(%esp); \
	movl	4(%esp), %ebp; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	20(%esp), %ebx; \
	movl	%ecx, 4(%esp); \
	addl	%edi, %ecx; \
	roll	$18, %ecx; \
	leal	(%esi, %ebp), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	36(%esp), %edi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %edi; \
	movl	%edi, 20(%esp); \
	movl	%ebx, %ecx; \
	movl	%edx, 36(%esp); \
	movl	24(%esp), %edx; \
	addl	%edi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %esi; \
	movl	28(%esp), %ebx; \
	movl	%esi, 24(%esp); \
	addl	%edi, %esi; \
	roll	$18, %esi; \
	leal	(%ecx, %edx), %edi; \
	roll	$7, %edi; \
	xorl	%edi, %ebx; \
	movl	%ebx, 28(%esp); \
	xorl	%esi, %ebp; \
	movl	8(%esp), %esi; \
	leal	(%edx, %ebx), %edi; \
	roll	$9, %edi; \
	xorl	%edi, %esi; \
	movl	40(%esp), %edi; \
	movl	%ebp, 8(%esp); \
	movl	44(%esp), %ebp; \
	movl	%esi, 40(%esp); \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	4(%esp), %ebx; \
	movl	%ecx, 44(%esp); \
	addl	%esi, %ecx; \
	roll	$18, %ecx; \
	leal	(%edi, %ebp), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	%ebx, 4(%esp); \
	movl	20(%esp), %esi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %esi; \
	movl	%esi, 56(%esp); \
	movl	48(%esp), %ecx; \
	movl	%edx, 20(%esp); \
	movl	36(%esp), %edx; \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %edi; \
	movl	24(%esp), %ebx; \
	movl	%edi, 24(%esp); \
	addl	%esi, %edi; \
	roll	$18, %edi; \
	leal	(%ecx, %edx), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	%ebx, 60(%esp); \
	movl	12(%esp), %esi; \
	xorl	%edi, %ebp; \
	leal	(%edx, %ebx), %edi; \
	roll	$9, %edi; \
	xorl	%edi, %esi; \
	movl	%esi, 12(%esp); \
	movl	52(%esp), %edi; \
	movl	%ebp, 36(%esp); \
	movl	8(%esp), %ebp; \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	16(%esp), %ebx; \
	movl	%ecx, 16(%esp); \
	addl	%esi, %ecx; \
	roll	$18, %ecx; \
	leal	(%edi, %ebp), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	32(%esp), %esi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %esi; \
	movl	%esi, 32(%esp); \
	movl	%ebx, %ecx; \
	movl	%edx, 48(%esp); \
	movl	20(%esp), %edx; \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %edi; \
	movl	24(%esp), %ebx; \
	movl	%edi, 20(%esp); \
	addl	%esi, %edi; \
	roll	$18, %edi; \
	leal	(%ecx, %edx), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	%ebx, 8(%esp); \
	movl	12(%esp), %esi; \
	xorl	%edi, %ebp; \
	leal	(%edx, %ebx), %edi; \
	roll	$9, %edi; \
	xorl	%edi, %esi; \
	movl	%esi, 12(%esp); \
	movl	28(%esp), %edi; \
	movl	%ebp, 52(%esp); \
	movl	36(%esp), %ebp; \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	16(%esp), %ebx; \
	movl	%ecx, 16(%esp); \
	addl	%esi, %ecx; \
	roll	$18, %ecx; \
	leal	(%edi, %ebp), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	%ebx, 28(%esp); \
	movl	32(%esp), %esi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %esi; \
	movl	%esi, 32(%esp); \
	movl	4(%esp), %ecx; \
	movl	%edx, 4(%esp); \
	movl	48(%esp), %edx; \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %edi; \
	movl	20(%esp), %ebx; \
	movl	%edi, 20(%esp); \
	addl	%esi, %edi; \
	roll	$18, %edi; \
	leal	(%ecx, %edx), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	%ebx, 48(%esp); \
	movl	40(%esp), %esi; \
	xorl	%edi, %ebp; \
	leal	(%edx, %ebx), %edi; \
	roll	$9, %edi; \
	xorl	%edi, %esi; \
	movl	%esi, 36(%esp); \
	movl	60(%esp), %edi; \
	movl	%ebp, 24(%esp); \
	movl	52(%esp), %ebp; \
	addl	%esi, %ebx; \
	roll	$13, %ebx; \
	xorl	%ebx, %ecx; \
	movl	44(%esp), %ebx; \
	movl	%ecx, 40(%esp); \
	addl	%esi, %ecx; \
	roll	$18, %ecx; \
	leal	(%edi, %ebp), %esi; \
	roll	$7, %esi; \
	xorl	%esi, %ebx; \
	movl	%ebx, 52(%esp); \
	movl	56(%esp), %esi; \
	xorl	%ecx, %edx; \
	leal	(%ebp, %ebx), %ecx; \
	roll	$9, %ecx; \
	xorl	%ecx, %esi; \
	movl	%esi, 56(%esp); \
	addl	%esi, %ebx; \
	movl	%edx, 44(%esp); \
	roll	$13, %ebx; \
	xorl	%ebx, %edi; \
	movl	%edi, 60(%esp); \
	addl	%esi, %edi; \
	roll	$18, %edi; \
	xorl	%edi, %ebp; \
	movl	%ebp, 64(%esp); \


	.text
	.align 32
gen_salsa8_core:
	gen_salsa8_core_quadround()
	gen_salsa8_core_quadround()
	ret
	
	
	.text
	.align 32
	.globl scrypt_core
	.globl _scrypt_core
scrypt_core:
_scrypt_core:
	pushl	%ebx
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	
	# Check for SSE2 availability
	movl	$1, %eax
	cpuid
	andl	$0x04000000, %edx
	jnz xmm_scrypt_core
	
gen_scrypt_core:
	movl	20(%esp), %edi
	movl	24(%esp), %esi
	subl	$72, %esp
	
#define scrypt_core_macro1a(p, q) \
	movl	p(%edi), %eax; \
	movl	q(%edi), %edx; \
	movl	%eax, p(%esi); \
	movl	%edx, q(%esi); \
	xorl	%edx, %eax; \
	movl	%eax, p(%edi); \
	movl	%eax, p(%esp); \

	
#define scrypt_core_macro1b(p, q) \
	movl	p(%edi), %eax; \
	xorl	p(%esi, %edx), %eax; \
	movl	q(%edi), %ebx; \
	xorl	q(%esi, %edx), %ebx; \
	movl	%ebx, q(%edi); \
	xorl	%ebx, %eax; \
	movl	%eax, p(%edi); \
	movl	%eax, p(%esp); \

	
#define scrypt_core_macro2(p, q) \
	movl	p(%esp), %eax; \
	addl	p(%edi), %eax; \
	movl	%eax, p(%edi); \
	xorl	q(%edi), %eax; \
	movl	%eax, q(%edi); \
	movl	%eax, p(%esp); \

	
#define scrypt_core_macro3(p, q) \
	movl	p(%esp), %eax; \
	addl	q(%edi), %eax; \
	movl	%eax, q(%edi); \

	
	leal	131072(%esi), %ecx
gen_scrypt_core_loop1:
	movl	%esi, 64(%esp)
	movl	%ecx, 68(%esp)
	
	scrypt_core_macro1a(0, 64)
	scrypt_core_macro1a(4, 68)
	scrypt_core_macro1a(8, 72)
	scrypt_core_macro1a(12, 76)
	scrypt_core_macro1a(16, 80)
	scrypt_core_macro1a(20, 84)
	scrypt_core_macro1a(24, 88)
	scrypt_core_macro1a(28, 92)
	scrypt_