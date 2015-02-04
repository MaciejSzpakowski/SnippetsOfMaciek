/* RULES OF CONVERTING TO POSTFIX

1. Print operands as they arrive.

2. If the stack is empty or contains a left parenthesis on top, push the incoming operator onto the stack.

3. If the incoming symbol is a left parenthesis, push it on the stack.

4. If the incoming symbol is a right parenthesis, pop the stack and print the operators until you see a left parenthesis.
Discard the pair of parentheses.

5. If the incoming symbol has higher precedence than the top of the stack, push it on the stack.

6. If the incoming symbol has equal precedence with the top of the stack, use association. If the association is left to right, 
pop and print the top of the stack and then push the incoming operator. If the association is right to left, push the incoming operator.

7. If the incoming symbol has lower precedence than the symbol on the top of the stack, 
pop the stack and print the top operator. Then test the incoming operator against the new top of stack.

8. At the end of the expression, pop and print all operators on the stack. (No parentheses should remain.)

*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
//e and pi
#define EULER 2.7182818284590452
#define PI    3.1415926535897932
//enum for operators
#define ADD 1
#define SUB 2
#define MUL 3
#define DIV 4
#define OPEN 5 //open paren.
#define CLOSE 6 //close paren.
#define POW 7
#define SQRT 8
#define LN 9
#define SIN 10
#define COS 11
#define TAN 12
//enum for operators precedence
#define PADD 1
#define PSUB 1
#define PMUL 2
#define PDIV 2
#define PPOW 3
#define PSQRT 4
#define PLN 4
#define PSIN 4
#define PCOS 4
#define PTAN 4
//modes in string reading
#define READNUM 1
#define READOP 2
#define PREDOT 1
#define POSTDOT 2

typedef struct _stackElement stackElement;
typedef struct _stack stack;

stackElement* pop(stack* s);
void push(stack* s, stackElement* se);

struct _stackElement
{
	double num;
	int operator_;
	int precedence;
	stackElement* next;
};

struct _stack
{
	stackElement* top;
};

//push number onto stack (it mallocs)
void pushNum(stack* s, double num)
{
	stackElement* se = (stackElement*)malloc(sizeof(stackElement));
	se->num = num;
	se->operator_ = 0;
	se->precedence = 0;
	push(s, se);
}

//push stackElement onto stack
void push(stack* s, stackElement* se)
{
	se->next = s->top;
	s->top = se;
}

//push operator to operator stack (it mallocs)
void pushOp(stack* op, stack* postfix, int operator_, int precedence)
{
	stackElement* se = (stackElement*)malloc(sizeof(stackElement));
	se->num = 0;
	se->operator_ = operator_;
	se->precedence = precedence;
	if (operator_ == CLOSE)
	{
		while (1)
		{
			if (op->top->operator_ == OPEN)
			{
				free(pop(op));
				free(se);
				return;
			}
			push(postfix, pop(op));
		}
	}
	if (op->top == 0)
	{
		se->next = 0;
		op->top = se;
	}
	else
	{
		//incoming has higher precedence (or you just opened parenthasis or it's open parenthasis operator ), push it
		if (op->top->precedence < precedence || op->top->operator_ == OPEN || operator_ == OPEN)
			push(op, se);
		//if incoming has lower or equal precedence, pop it (and push to postfix) and check again
		else
		{
			free(se);
			push(postfix, pop(op));
			pushOp(op,postfix,operator_,precedence); //recursion ! no way :D
		}
	}
}

//pops the stack and returns the pop
//warning: catch whatever it's returning and free if you dont need it
stackElement* pop(stack* s)
{
	stackElement* se = 0;
	if (s->top == 0)
		printf("Nothing to pop");
	else
	{
		se = s->top;
		s->top = se->next;
	}
	return se;
}

//takes stack that stores information in correct postfix notation and evaluates
double evaluateStackProper(stack* s)
{
	stack numbers = { 0 };
	while (s->top)
	{
		stackElement* se = pop(s);
		if (se->operator_ == 0)
			push(&numbers, se);
		else if (se->operator_ == ADD)
		{
			stackElement* num1 = pop(&numbers);
			stackElement* num2 = pop(&numbers);
			pushNum(&numbers, num1->num + num2->num);
			free(num1);
			free(num2);
		}
		else if (se->operator_ == SUB)
		{
			stackElement* num1 = pop(&numbers);
			stackElement* num2 = pop(&numbers);
			pushNum(&numbers, num2->num - num1->num );
			free(num1);
			free(num2);
		}
		else if (se->operator_ == MUL)
		{
			stackElement* num1 = pop(&numbers);
			stackElement* num2 = pop(&numbers);
			pushNum(&numbers, num1->num * num2->num);
			free(num1);
			free(num2);
		}
		else if (se->operator_ == DIV)
		{
			stackElement* num1 = pop(&numbers);
			stackElement* num2 = pop(&numbers);
			pushNum(&numbers, num2->num / num1->num);
			free(num1);
			free(num2);
		}
		else if (se->operator_ == POW)
		{
			stackElement* num1 = pop(&numbers);
			stackElement* num2 = pop(&numbers);
			pushNum(&numbers, pow(num2->num, num1->num));
			free(num1);
			free(num2);
		}
		else if (se->operator_ == SQRT)
		{
			stackElement* num1 = pop(&numbers);
			pushNum(&numbers, sqrt(num1->num));
			free(num1);
		}
		else if (se->operator_ == LN)
		{
			stackElement* num1 = pop(&numbers);
			pushNum(&numbers, log(num1->num));
			free(num1);
		}
		else if (se->operator_ == SIN)
		{
			stackElement* num1 = pop(&numbers);
			pushNum(&numbers, sin(num1->num));
			free(num1);
		}
		else if (se->operator_ == COS)
		{
			stackElement* num1 = pop(&numbers);
			pushNum(&numbers, cos(num1->num));
			free(num1);
		}
		else if (se->operator_ == TAN)
		{
			stackElement* num1 = pop(&numbers);
			pushNum(&numbers, tan(num1->num));
			free(num1);
		}
	}
	stackElement* result = pop(&numbers);
	double val = result->num;
	free(result);
	return val;
}

double mathParser(char* str)
{
	int mode = READNUM;
	stack opStack = { 0 }; //operator stack
	stack postStack = { 0 }; //postfix stack
	int index = 0;
	double build = 0; //number that is being constructed before push
	int started = 0; //tells whther num is being constructed or not, so we can now if we can push it or not
	int doubleBuildMode = PREDOT;
	int decimalPlace = 1;
	for (index = 0;; index++)
	{
		char nextChar = str[index];
		if (mode == READNUM)
		{
			if (isdigit(nextChar) && doubleBuildMode == PREDOT)
			{
				started = 1;
				build *= 10;
				build += nextChar - '0';
			}
			else if (isdigit(nextChar) && doubleBuildMode == POSTDOT)
			{
				started = 1;
				build += (double)(nextChar - '0') / (decimalPlace * 10.0f);
				decimalPlace *= 10;
			}
			else if (nextChar == '.' && doubleBuildMode == PREDOT)
			{
				doubleBuildMode = POSTDOT;
			}
			else if (nextChar == '.')
			{
				printf("Another . in number\n");
				return 1;
			}
			else
			{
				doubleBuildMode = PREDOT;
				if (nextChar == '+')
				{
					if (started)
						pushNum(&postStack, build);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, ADD, PADD);
				}
				else if (nextChar == '-')
				{
					if (started)
						pushNum(&postStack, build);
					//add 0 in the front if we start with negative number
					if (index == 0)
						pushNum(&postStack, 0);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, SUB, PSUB);
				}
				else if (nextChar == '*')
				{
					if (started)
						pushNum(&postStack, build);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, MUL, PMUL);
				}
				else if (nextChar == '/')
				{
					if (started)
						pushNum(&postStack, build);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, DIV, PDIV);
				}
				else if (nextChar == '(')
				{
					if (started)
						pushNum(&postStack, build);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, OPEN, 0);
				}
				else if (nextChar == ')')
				{
					if (started)
						pushNum(&postStack, build);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, CLOSE, 0);
				}
				else if (nextChar == '^')
				{
					if (started)
						pushNum(&postStack, build);
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, POW, PPOW);
				}
				else if (memcmp("PI", str + index, 2) == 0 || memcmp("pi", str + index, 2) == 0)
				{
					index++;
					pushNum(&postStack, PI);
					started = 0;
					build = 0;
					decimalPlace = 1;
				}
				else if (memcmp("e", str + index, 1) == 0)
				{
					pushNum(&postStack, EULER);
					started = 0;
					build = 0;
					decimalPlace = 1;
				}
				else if (memcmp("sqrt", str + index, 4) == 0)
				{
					index += 3;
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, SQRT, PSQRT);
				}
				else if (memcmp("ln", str + index, 2) == 0)
				{
					index += 1;
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, LN, PLN);
				}
				else if (memcmp("sin", str + index, 3) == 0)
				{
					index += 2;
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, SIN, PSIN);
				}
				else if (memcmp("cos", str + index, 3) == 0)
				{
					index += 2;
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, COS, PCOS);
				}
				else if (memcmp("tan", str + index, 3) == 0)
				{
					index += 2;
					started = 0;
					build = 0;
					decimalPlace = 1;
					pushOp(&opStack, &postStack, TAN, PTAN);
				}
			}
			if (str[index + 1] == '\0')
			{
				if (started)
					pushNum(&postStack, build);
				started = 0;
				break;
			}
		}
	}
	//push reminder from op to post
	while (opStack.top)
	{
		stackElement* se = pop(&opStack);
		push(&postStack, se);
	}
	stack newStack = { 0 };
	//reverse stack
	while (postStack.top)
		push(&newStack, pop(&postStack));

	return evaluateStackProper(&newStack);
}

int main()
{
	#define BUFFER 5000
	char strIn[BUFFER];
	for (int i = 0; i < BUFFER; i++)
		strIn[i] = 0;
	char c;
	int index = 0;
	while ((c = getchar()) != '\n')
	{
		strIn[index] = c;
		index++;
	}

	printf("Result: %f",mathParser(strIn));
	printf("\n");
	return 0;
}