
	EXPORT .AsmFixedMul			# export the code symbol

.AsmFixedMul:
	mullw	r5,r3,r4			# multiply low word
	mulhw	r6,r3,r4			# multiply high word (signed)
	rlwinm	r3,r5,16,16,31		# mask off low portion of result
	rlwimi	r3,r6,16,0,15		# insert high portion of result
	blr							# return to caller


#
#	Fixed AsmFixedDiv(Fixed a, Fixed b);
#
#	On entry, this routine takes the following parameters:
#		r3 - 	operand a (fixed point, frac, or long format)
#		r4 -	operand b (fixed point, frac, or long format)
#	On exit:
#		r3 -	fixed point, frac, or long quotient
#
#	Within the routine, r0 and r5-r9 are used as scratch registers.
#	Condition register fields cr0 and cr1 are also used.
#
#	If b is zero, the function returns:
#		0x80000000 if a is negative
#		0x7FFFFFFF if a is zero or positive
#	If division overflows, the function returns:
#		0x80000000 if the result is negative
#		0x7FFFFFFF if the result is positive
#

	EXPORT .AsmFixedDiv			# export the code symbol

# Internal register usage
rNumH:		EQU		r5
rNumL:		EQU		r6
rDen:		EQU		r4
rTemp1:		EQU		r7
rTemp2:		EQU		r8
rTemp3:		EQU		r9

bNumNeg:	EQU		4+0
bDenNeg:	EQU		0
bQuotNeg:	EQU		4+3

.AsmFixedDiv:
	li		r0,0				# create a handy copy of zero
	cmpwi	0,rDen,0			# check for divide by zero
	srawi	rNumH,r3,16			# mask off high word of fixed number and sign-extend
	rlwinm	rNumL,r3,16,0,15	# mask off low word of fixed number
	beq		0,FixDivByZero		# branch if divide by zero

	# At this point we have a 64-bit number in r5:r6 and a 32-bit
	# number in r4.

	cmpwi	1,rNumH,0			# check sign of numerator
	crxor	bQuotNeg,bDenNeg,bNumNeg
								# calculate sign of result, put it into bit 4
	bf		bDenNeg,FixDivCheckNumer 	
								# check sign of denominator
	neg		rDen,rDen			# make denominator positive if it was negative

FixDivCheckNumer:
	bf		bNumNeg,FixDiv64Common	
								# continue if numerator is positive
	subfc	rNumL,rNumL,r0		# negate denominator, carrying as appropriate
	subfe	rNumH,rNumH,r0

FixDiv64Common:
	cmplw	0,rNumH,rDen		# check for overflow

	cntlzw	rTemp1,rDen			# shift numer and denom left
	xor		rNumH,rNumH,rNumL	#  until denom’s most sig. bit is one
	slw		rDen,rDen,rTemp1
	rlwnm	rNumH,rNumH,rTemp1,0,31
	slw		rNumL,rNumL,rTemp1
	xor		rNumH,rNumH,rNumL
	
	bge-	FixDivOverflow		# branch if overflow

	srwi	rTemp2,rDen,16		
	divwu	rTemp3,rNumH,rTemp2	# perform 32-bit by 16-bit division
	mullw	rTemp2,rTemp3,rTemp2
	subf	rNumH,rTemp2,rNumH	# calculate remainder
	slwi	rNumH,rNumH,16
	rlwimi	rNumH,rNumL,16,16,31
	slwi	rNumL,rNumL,16
	rlwinm	rTemp2,rDen,0,16,31
	mullw	rTemp2,rTemp3,rTemp2
	subfc	rNumH,rTemp2,rNumH
	subfe.	rTemp2,rTemp2,rTemp2
	add		rNumL,rNumL,rTemp3
	bge		FixDiv64ComputeLow
FixDiv64CorrectHigh:
	addc	rNumH,rNumH,rDen
	addze.	rTemp2,rTemp2
	subi	rNumL,rNumL,1
	blt		FixDiv64CorrectHigh

FixDiv64ComputeLow:
	srwi	rTemp2,rDen,16
	divwu	rTemp3,rNumH,rTemp2
	mullw	rTemp2,rTemp3,rTemp2
	subf	rNumH,rTemp2,rNumH
	slwi	rNumH,rNumH,16
	rlwimi	rNumH,rNumL,16,16,31
	slwi	rNumL,rNumL,16
	rlwinm	rTemp2,rDen,0,16,31
	mullw	rTemp2,rTemp3,rTemp2
	subfc	rNumH,rTemp2,rNumH
	subfe.	rTemp2,rTemp2,rTemp2
	add		rNumL,rNumL,rTemp3
	bge		FixDiv64Done
FixDiv64CorrectLow:
	addc	rNumH,rNumH,rDen
	addze.	rTemp2,rTemp2
	subi	rNumL,rNumL,1
	blt		FixDiv64CorrectLow

FixDiv64Done:
	addco.	r3,rNumL,r0
	bt		bQuotNeg,FixDiv64QuotientNeg	
							# see if we need to negate answer
	blt-	FixDivOverflow	# check for overflow case
	blr						# return to caller

FixDiv64QuotientNeg:
	neg.	r3,r3			# negate answer
	bgt-	FixDivOverflow	# check for overflow
	blr						# return to caller


FixDivOverflow:
	lis		r3,-0x8000		# load max negative number
	btlr	bQuotNeg		# return if result is negative
	subi	r3,r3,1			# calc max positive number
	blr						# return to caller

FixDivByZero:
	cmpwi	0,r3,0			# is r3 negative?
	lis		r3,-0x8000		# load max negative number
	btlr	bNumNeg			# return if numerator was negative
	subi	r3,r3,1			# calc max positive number
	blr						# return to caller





#	wide *AsmWideAdd(wide *target, wide *source);
#	
#	On entry, this routine takes the following parameters:
#		r3 - 	pointer to target
#		r4 -	pointer to source
#	On exit:
#		r3 -	pointer to target
#
#	Within the routine, r6-r9 are used as scratch registers.
#

	EXPORT .AsmWideAdd		# export the code symbol

# Internal register usage
rAH:		EQU		r6
rAL:		EQU		r7
rBH:		EQU		r8
rBL:		EQU		r9

.AsmWideAdd:
	lwz		rAL,4(r3)		# load low word of operand A
	lwz		rBL,4(r4)		# load low word of operand B
	lwz		rAH,0(r3)		# load high word of operand A
	addc	rAL,rAL,rBL		# add low words together
	lwz		rBH,0(r4)		# load high word of operand B
	stw		rAL,4(r3)		# store low word of result
	adde	rAH,rAH,rBH		# add high words with carry
	stw		rAH,0(r3)		# store high word of result
	blr						# return to caller
	
	
#	wide *AsmWideMultiply(long multiplicand, long multiplier, wide *target);
#	
#	On entry, this routine takes the following parameters:
#		r3 - 	operand A
#		r4 -	operand B
#		r5 -	pointer to result location
#	On exit:
#		r3 -	pointer to result
#
#	Within the routine, r6-r9 are used as scratch registers.
#

	EXPORT .AsmWideMultiply	# export the code symbol

.AsmWideMultiply:
	mullw	r6,r3,r4		# multiply low word
	mulhw	r7,r3,r4		# multiply high word (signed)
	stw		r6,4(r5)		# store low word of result
	mr		r3,r5			# move return parameter into place
	stw		r7,0(r5)		# store low word of result
	blr						# return to caller
	
