	.global _start

	.text
_start:
	mov 	$4, %eax	# arg 1, the write system call
	mov	$1, %ebx	# arg 2, stdout
	mov	$txt, %ecx	# arg 3, buffer
	mov	$13, %edx	# arg 4, buffer size
	int	$0x80		# call

	mov	$1, %eax	# arg 1, the exit system call
	mov	$0, %ebx	# arg 2, the return value
	int	$0x80		# call

txt:
	.ascii "Hello World\n"
