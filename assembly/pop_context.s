	.def popContext
	.def popStart

.thumb

.text

popContext:
	LDR R11,[R0]
	LDR R10,[R0, #4]!
	LDR R9,[R0, #4]!
	LDR R8,[R0, #4]!
	LDR R7,[R0, #4]!
	LDR R6,[R0, #4]!
	LDR R5,[R0, #4]!
	LDR R4,[R0, #4]!
	ADD R0, #4
	BX LR

popStart:
	LDR R11,[R0]
	LDR R10,[R0, #4]!
	LDR R9,[R0, #4]!
	LDR R8,[R0, #4]!
	LDR R7,[R0, #4]!
	LDR R6,[R0, #4]!
	LDR R5,[R0, #4]!
	LDR R4,[R0, #4]!
	ADD R0, #4
	BX LR
