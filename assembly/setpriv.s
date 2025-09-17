	.def setPriv
	.def setUnPriv

.thumb

.text

setPriv:
	MRS R0, CONTROL
	ORR R0, #0
	MSR CONTROL, R0
	ISB
	BX LR

setUnPriv:
	MRS R0, CONTROL
	ORR R0, #1
	MSR CONTROL, R0
	BX LR


