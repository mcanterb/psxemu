#pragma once

#if(TESTING == 1)

#define MOCKABLE(F) F ## __

#ifdef __x86_64__
#define MOCK(F) __asm__(\
	".globl test_" #F "\n"\
	".globl " #F "\n"\
	".text\n"\
	#F ":\n"\
	"movq	test_" #F "(%rip), %r11\n\t"\
	"testq	%r11, %r11\n\t"\
	"je	" #F "__\n\t"\
	"jmp	*%r11\n\t"\
);\
__asm__(\
	".data\n"\
    ".align 8\n"\
	"test_" #F ":\n\t"\
	".long 0\n\t"\
	".long 0\n\t"\
)


#else
#define MOCK(F) __attribute__((section(\
	".bss\n\t"\
	".globl test_" #F "\n\t"\
	".align 4\n\t"\
	".type	test_" #F ", @object\n\t"\
	".size	test_" #F ", 4\n"\
	"test_" #F ":\n\t"\
	".zero	4\n\t"\
	".text\n\t"\
	".p2align 4,,15\n\t"\
	".globl	" #F "\n\t"\
	".type	" #F ", @function\n"\
	#F ":\n\t"\
	".cfi_startproc\n\t"\
	"push %edx\n\t"\
	"push %edx\n\t"\
	"push %eax\n\t"\
	"movl test_" #F ", %eax\n\t"\
	"leal " #F "__, %edx\n\t"\
	"test %eax, %eax\n\t"\
	"cmove %edx, %eax\n\t"\
	"mov %eax, 8(%esp)\n\t"\
	"pop %eax\n\t"\
	"pop %edx\n\t"\
	"ret\n\t"\
	".cfi_endproc\n\t"\
	".size	" #F ", .-" #F "\n\t"\
	".section	.text"))) F ## __

#endif

#define DECLARE_MOCK(F) \
	__typeof__(F) F ## __;\
    extern __typeof__(F) *test_ ## F

#else

#define MOCKABLE(F) F
#define MOCK(F)
#define DECLARE_MOCK(F)

#endif