//C++ (actually it's 99.99% C, there is namespace,template)
/* TWO FUNCTIONS TO BE USED EXTERNALLY
namespace parser2
{
void testParser(); //have several tests
double mathParser(const char* str); //this one evaluates string, in case of an error it usually returns 0
}
*/

//VARIABLES:
// variables syntax: expression,x=exrepssion,y=expression...
// variables must be letters other than e, there must be ',' after every expression except the last one
// variables can be expressed in terms of other variables: x+y,x=y+6,y=3 and x+y+z,x=y+z+2,y=z+3,z=4 are legal
// variable can be used in expression if it is later defined as constant(or evaluated to const.): 
// x+y,x=3,y=x+6 ILLEGAL! because when evaluating x+6 for y, function cannot find x because it's been defined before

//FUNCTIONS:
// functions are operators as well: sin(pi) == sinpi, ln(4) == ln4, but sqrt7+2 != sqrt9 because sqrt has higher precedence than +
// that's why brackets are recommended
// if function takes more than one variable, separate all variables by brackets, remeber about order
// log(base)(number): log(2)(64) == log2(64) == 6

//SUM:
// syntax: sum(lower bound,upper bound,expression with 'i'), it must be i, i is variable of iteration
// lower and upper bounds might be expressions as well but they are truncated to int, sum inside sum is buggy, i think
// it has something to do with variable letter

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
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>

using namespace std;

namespace parser2
{
	//e and pi
	#define EULER 2.7182818284590452
	#define PI    3.1415926535897932
	#define OPEN 5
	#define CLOSE 6

	typedef struct tagVAR
	{
		double val;
		char letter;
	} VAR, *PVAR;

	double mathParser(const char* str);

	typedef struct _MyOperator
	{
		string str;
		int len;
		int num; //enum like to recognize it fast
		int precedence;
		double (*func)(double* args);
		int operands; //how many to pop to do operation on
	} MyOperator;

	typedef struct _stackElement stackElement;
	typedef struct _stack stack;

	stackElement* pop(stack* s);
	void push(stack* s, stackElement* se);

	struct _stackElement
	{
		double num;
		int operator_;
		int precedence;
		double(*func)(double* args);
		int operands;
		string expression;
		stackElement* next;
	};

	struct _stack
	{
		stackElement* top;
	};

	template <typename T>
	bool contains(T* arr, T element, int size)
	{
		for (int i = 0; i < size; i++)
		{
			if (arr[i] == element)
				return true;
		}
		return false;
	}

	//push stackElement onto stack
	void push(stack* s, stackElement* se)
	{
		se->next = s->top;
		s->top = se;
	}

	//push number onto stack (it mallocs)
	void pushNum(stack* s, double num)
	{
		stackElement* se = (stackElement*)malloc(sizeof(stackElement));
		se->num = num;
		se->operator_ = 0;
		se->precedence = 0;
		push(s, se);
	}	

	//push operator to operator stack (it mallocs)
	void pushOp(stack* op, stack* postfix, int operator_, int precedence, double(*func)(double* args) , int operands)
	{
		stackElement* se = (stackElement*)malloc(sizeof(stackElement));
		se->num = 0;
		se->operator_ = operator_;
		se->precedence = precedence;
		se->func = func;
		se->operands = operands;
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
				pushOp(op, postfix, operator_, precedence,func,operands); //recursion ! no way :D
			}
		}
	}

	//pops the stack and returns the pop
	//warning: catch whatever it's returning and free if you dont need it
	stackElement* pop(stack* s)
	{
		stackElement* se = 0;
		if (s->top)
		{
			se = s->top;
			s->top = se->next;
		}
		return se;
	}

	//takes stack that stores information in correct postfix notation and evaluates
	double evaluateStackProper(stack* s, MyOperator* operators)
	{
		stack numbers = { 0 };
		while (s->top)
		{
			stackElement* se = pop(s);
			if (se->operator_ == 0)
				push(&numbers, se);
			else
			{
				double* args = (double*)malloc(sizeof(double)*se->operands);
				for (int i = 0; i < se->operands; i++)
				{
					stackElement* num = pop(&numbers);
					args[i] = num->num;
					free(num);
				}
				pushNum(&numbers, se->func(args));
				free(args);
			}
		}
		stackElement* result = pop(&numbers);
		double val = 0;
		if (result)
			val = result->num;
		free(result);
		return val;
	}	

	//find all occurenced of c in cstr (it better have 0 at the end)
	int strCount(const char* cstr, char c)
	{
		int count = 0;
		for (int i = 0; cstr[i] != 0; i++)
		{
			if (cstr[i] == c)
				count++;
		}
		return count;
	}

	//etracts variables from string
	//example: x+y,x=5,y=3
	//will convert to 5+3
	//will return numbers of chars obtained
	//it allocates PVARS and strores data there
	int extractVars(PVAR* vars, const char* str)
	{
		int count = strCount(str, '=');
		if (count == 0)
			return 0;
		*vars = (PVAR)malloc(sizeof(VAR)*count);
		const char* iterator = str;

		for (int i = 0; i<count; i++)
		{
			iterator = strchr(iterator, '=');
			//e is reserved
			if (iterator[-1] == 'e')
			{
				free(*vars);
				return 0;
			}
			(*vars)[i].val = mathParser(iterator + 1);
			(*vars)[i].letter = iterator[-1];
			if (!isalpha((*vars)[i].letter))
			{
				free(*vars);
				return 0;
			}
			iterator++;
		}
		return count;
	}

	//when string is being evaluated, this functions finds appropriate variable and returns PVAR with it
	PVAR getVar(PVAR vars, char c, int n)
	{
		for (int i = 0; i<n; i++)
		{
			if (vars[i].letter == c)
				return &vars[i];
		}
		return NULL;
	}

	//checks if current string is an operator, returns index of hte operator or -1 if nothing found
	int getOperator(const char* str,MyOperator* operators,int opCount)
	{
		for (int i = 0; i<opCount; i++)
		{
			if (memcmp(operators[i].str.c_str(), str, operators[i].len) == 0)
				return i;
		}
		return -1;
	}

	//factorial
	double fac(double num1)
	{
		int inum1 = (int)num1;
		int result = 1;
		//non negative only
		if (num1 < 0)
			return 0;
		//int only
		if (num1 - (int)num1 != 0)
			return 0;
		for (int i = 1; i < inum1+1; i++)
			result *= i;
		return result;
	}

	//gets matching ending bracket: find close of this-->(()()(()))<-- this one
	int getEndBracket(const char* str,int start)
	{
		int open = 0;
		for (int i = 0;str[i] != 0; i++)
		{
			if (str[i] == '(')
				open++;
			else if (str[i] == ')')
				open--;
			if (open == -1)
				return i + start;
		}
		return -1;
	}	

	double mathParser(const char* str)
	{
		const int opCount = 25;
		MyOperator operators[opCount] =
		{
			//string,string length,unique id (simply increment),precedence (leave 4 for functions),lambda,operands (how many)
			{ "+", 1, 1, 1, [](double* args) {return args[1] + args[0]; }, 2 },
			{ "-", 1, 2, 1, [](double* args) {return args[1] - args[0]; }, 2 },
			{ "/", 1, 3, 2, [](double* args) {return args[1] / args[0]; }, 2 },
			{ "*", 1, 4, 2, [](double* args) {return args[1] * args[0]; }, 2 },
			{ "(", 1, 5, 0, NULL, 0 },
			{ ")", 1, 6, 0, NULL, 0 },
			{ "%", 1, 7, 2, [](double* args) {return fmod(args[1], args[0]); }, 2 },
			{ "^", 1, 8, 3, [](double* args) {return pow(args[1], args[0]); }, 2 },
			{ "sqrt", 4, 9, 4, [](double* args) {return sqrt(args[0]); }, 1 },
			{ "ln", 2, 10, 4, [](double* args) {return log(args[0]); }, 1 },
			{ "sin", 3, 11, 4, [](double* args) {return sin(args[0]); }, 1 },
			{ "cos", 3, 12, 4, [](double* args) {return cos(args[0]); }, 1 },
			{ "tan", 3, 13, 4, [](double* args) {return tan(args[0]); }, 1 },
			{ "asin", 4, 14, 4, [](double* args) {return asin(args[0]); }, 1 },
			{ "acos", 4, 15, 4, [](double* args) {return acos(args[0]); }, 1 },
			{ "atan", 4, 16, 4, [](double* args) {return atan(args[0]); }, 1 },
			{ "sinh", 4, 17, 4, [](double* args) {return sinh(args[0]); }, 1 },
			{ "cosh", 4, 18, 4, [](double* args) {return cosh(args[0]); }, 1 },
			{ "tanh", 4, 19, 4, [](double* args) {return tanh(args[0]); }, 1 },
			{ "asinh", 5, 20, 4, [](double* args) {return asinh(args[0]); }, 1 },
			{ "acosh", 5, 21, 4, [](double* args) {return acosh(args[0]); }, 1 },
			{ "atanh", 5, 22, 4, [](double* args) {return atanh(args[0]); }, 1 },
			{ "abs", 3, 23, 4, [](double* args) {return abs(args[0]); }, 1 },
			{ "fac", 3, 24, 4, [](double* args) {return fac(args[0]); }, 1 },
			/*25*/{ "log", 3, 25, 4, [](double* args) {return log(args[0]) / log(args[1]); }, 2 } //any base
		};
		PVAR vars = NULL;
		int varsCount = extractVars(&vars, str);
		stack opStack = { 0 }; //operator stack
		stack postStack = { 0 }; //postfix stack
		int index = 0;
		int currentNumIndex = -1;
		for (index = 0;; index++)
		{
			char nextChar = str[index];
			//if it's a digit keep going
			if (isdigit(nextChar))
			{
				if (currentNumIndex == -1)
					currentNumIndex = index;
			}
			else
			{
				int opDetected = -1;
				//2 dots in a row
				if (nextChar == '.')
				{
					if (str[index + 1] != 0 && str[index + 1] == '.')
						return 0;
				}
				//non number detected, push last number onto stack
				else if (currentNumIndex >= 0)
				{
					pushNum(&postStack, atof(str + currentNumIndex));
					currentNumIndex = -1;
				}
				//0 or ',' ends string parsing
				if (nextChar == 0 || nextChar == ',')
					break;
				//minus needs special treatment
				if (nextChar == '-')
				{
					//add 0 in the front if we start with negative number
					if (index == 0)
						pushNum(&postStack, 0);
					else if (str[index - 1] == '(')
						pushNum(&postStack, 0);
					pushOp(&opStack, &postStack, 2, 1, operators[1].func,2);
				}
				//sum
				else if (memcmp("sum", str + index, 3) == 0)
				{
					int closingBracket = getEndBracket(str+index+4,index+4);
					if (closingBracket == -1)
						return 0;
					char* closingBracketChar = (char*)(str+closingBracket);
					*closingBracketChar = ',';					
					int lowerBound = (int)mathParser(str + index + 4);
					char* firstComa = (char*)strchr(str + index, ',');
					int upperBound = (int)mathParser(firstComa + 1);
					char* preExpression = (char*)strchr(firstComa+1, ',') + 1;
					double sum = 0;
					for (int i = lowerBound; i <= upperBound; i++)
					{
						stringstream ss;
						ss << preExpression;
						char sumVar = 'i';
						for (int j = 0; j < varsCount; j++)
						{
							if (vars[i].letter == sumVar)
							{
								sumVar += 1;
								j = -1;
							}
						}
						ss << ',' << sumVar <<'=' << i;
						sum += mathParser(ss.str().c_str());
					}
					pushNum(&postStack, sum);
					index = closingBracket;
				}
				//pi is pi
				else if (memcmp("PI", str + index, 2) == 0 || memcmp("pi", str + index, 2) == 0)
				{
					index++;
					pushNum(&postStack, PI);
				}
				//e is e
				else if (nextChar == 'e')
				{
					pushNum(&postStack, EULER);
				}
				//try if it's operator
				else if ((opDetected = getOperator(str+index,operators,opCount)) != -1)
				{
					index += operators[opDetected].len - 1;
					pushOp(&opStack, &postStack, operators[opDetected].num, operators[opDetected].precedence, operators[opDetected].func, operators[opDetected].operands);
				}
				else
				{
					//else try var
					PVAR var = getVar(vars, str[index], varsCount);
					if (var)
						pushNum(&postStack, var->val);
					else if (isalpha(nextChar))
						return 0;
				}
			}
		}
		if (vars)
			free(vars);
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

		return evaluateStackProper(&newStack,operators);
	}

	void testParser()
	{
		int lines = 10;
		string arr[] = {
			"1+3.5", "4.5",
			"e^3*sin(pi/2)+1*(4-3+tan(0.1))", "21.18587",
			"e*pi+e^ln(1)", "9.53973",
			"sin(e*atan(0.01))^2+cos(e*atan(0.01))^2", "1",
			"ln(e^7)", "7",
			"x^y,x=25/16,y=1/2", "1.25",
			"a*(b+c+d),a=sin(b),b=pi/c,c=1+d,d=abs(cos(pi))", "4.57079",
			"(1+log(pi)(pi))/(sin(pi/(log10(100)))*2)", "1",
			"x+6 y", "0",
			"x+6 x=6", "0",
		};
		for (int i = 0; i < lines * 2; i += 2)
			cout <<"["<< arr[i] <<"]"<< " = " << mathParser(arr[i].c_str()) << ", real answer: " << arr[i + 1] << endl;
	}
}
