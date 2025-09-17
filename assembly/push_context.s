	.def pushContext

.thumb

.text

pushContext:
	STR R4, [R0, #-4]!
	STR R5, [R0, #-4]!
	STR R6, [R0, #-4]!
	STR R7, [R0, #-4]!
	STR R8, [R0, #-4]!
	STR R9, [R0, #-4]!
	STR R10, [R0, #-4]!
	STR R11, [R0, #-4]!
	BX LR
