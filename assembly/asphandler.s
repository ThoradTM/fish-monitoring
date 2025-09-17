	.def setAsp

.thumb

.text

setAsp:
	MRS R0, CONTROL
	ORR R0, #2
	MSR CONTROL, R0
	ISB
	BX LR
