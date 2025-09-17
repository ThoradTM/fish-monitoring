	.def getPsp

.thumb

.text

getPsp:
	MRS R0, PSP
	BX LR
