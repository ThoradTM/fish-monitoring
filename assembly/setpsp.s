	.def setPsp

.thumb

.text

setPsp:
	MSR PSP, R0
	ISB
	BX LR
