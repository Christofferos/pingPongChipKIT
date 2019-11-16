# labwork.S
# Written 2015-2017 by F Lundevall
# Skeleton file for IS1200/IS1500 lab 1.
# The macros PUSH and POP are in the public domain.
# Please add your own code at the end of the file.

#
# Please keep the two macros PUSH and POP unchanged
#
.global delay
.global time2string
.global enable_interrupt
.macro	PUSH reg
addi	$sp,$sp,-4
sw	\reg,0($sp)
.endm

.macro	POP reg
lw	\reg,0($sp)
addi	$sp,$sp,4
.endm
#
# Please add your own code below this line
#
.data
.align 2
.text


# simulate time delay
delay:
move 	$t0, $a0	# ms
li 	$t2, 3000		# For-looop constant
whileloop:
ble 	$t0, $0, delayend	# If ms > 0 continue
nop
addi	$t0, $t0, -1	# ms = ms - 1
li 	$t1, 0		# i = 0
forloop:
ble	$t2, $t1, whileloop	# If i < for-loop constant, continue the loop.
nop
addi	$t1, $t1, 1	# i++
j	forloop
nop
delayend:
jr	$ra
nop

# a0 = timeString pointer
# a1 = myTime
time2string:
move 	$t0, $a0	# Copy a0 to t0
move 	$t1, $a1	# Copy a1 to t1
PUSH	$ra
PUSH	$t0
PUSH	$t1
srl	$t1, $t1, 4
PUSH	$t0
PUSH	$t1
srl	$t1, $t1, 4
PUSH	$t0
PUSH	$t1
srl	$t1, $t1, 4
PUSH	$t0

move 	$a0, $t1	#Set parameter for hexasc
jal	hexasc
nop
POP	$t0
sb	$v0, 0($t0)

POP	$t1
move 	$a0, $t1
jal	hexasc
nop
POP	$t0
sb	$v0, 1($t0)

li	$t3, 0x3a
sb	$t3, 2($t0)		# Add colon

POP	$t1
move 	$a0, $t1
jal	hexasc
nop
POP	$t0
sb	$v0, 3($t0)

POP	$t1
move 	$a0, $t1
jal	hexasc
nop
POP	$t0
sb	$v0, 4($t0)

li	$t3, 0x00
sb	$t3, 5($t0)		# Add NULL

POP	$ra
jr 	$ra
nop

    # you can write your code for subroutine "hexasc" below this line
hexasc:
and 	$v0, $a0, 0xf		# Modulus 15 on a0, copied into v0.
addi 	$v0, $v0, 0x30		# Convert to ascii
li 	  $t0, 0x39		# 9

ble 	$v0,$t0,hexascend 	# value is accepted (within interval)
nop
addi	$v0, $v0, 7		# add 7 steps to get A-F

hexascend:
jr	$ra
nop

enable_interrupt:
  ei
  jr  $ra
  nop
