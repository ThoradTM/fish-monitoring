	.def getSvc

.thumb

.text

getSvc:
	MRS R0, PSP
	ADD R0, #24
	LDRB R0, [R0, #-2]
	BX LR
