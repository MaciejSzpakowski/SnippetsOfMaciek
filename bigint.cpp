//except namespace, it's all C
#include "generic_header.h"

namespace bigint
{
	typedef unsigned int UINT;
	typedef unsigned char DIGIT, *PDIGIT;

	typedef struct tagBIG
	{
		UINT capacity;
		UINT size;
		SIGN sign;
		PDIGIT digits;
		PDIGIT mostSig;
	} BIG, *PBIG;

	void increaseCapacity(PBIG big, UINT newCapacity)
	{
		PDIGIT newArray = (PDIGIT)malloc(sizeof(DIGIT) * newCapacity);
		memcpy(newArray, big->digits, big->capacity*sizeof(DIGIT));
		memset(newArray + big->capacity, 0, sizeof(DIGIT)*(newCapacity - big->capacity));
		free(big->digits);
		big->digits = newArray;
		big->mostSig = newArray + big->size - 1;
		big->capacity = newCapacity;
	}
		
	//char to string char, e.g. 0 to '0'
	char inline ctoa(char val)
	{
		return val - '0';
	}

	void destroy(PBIG big)
	{
		free(big->digits);
		free(big);
	}

	int intToIntArr(int num, PDIGIT arr, int pos)
	{
		if (num > 9)
			pos += intToIntArr(num / 10, arr, pos);
		int digit = num - (num / 10) * 10;
		arr[pos] = (DIGIT)digit;
		return pos + 1;
	}

	PBIG bigFromInt(int num, int capacity)
	{
		PBIG result = (PBIG)malloc(sizeof(BIG));
		result->capacity = capacity;
		result->sign = num < 0 ? NEGATIVE : POSITIVE;
		result->digits = (PDIGIT)malloc(sizeof(DIGIT) * capacity);
		memset(result->digits, 0, sizeof(DIGIT)*capacity);
		result->size = 0;
		for (int i=0; num > 0;i++)
		{
			result->size++;
			int digit = num - (num / 10) * 10;
			result->digits[i] = (DIGIT)digit;
			num /= 10;
		}
		result->mostSig = result->digits + result->size - 1;
		return result;
	}

	PBIG bigFromBig(PBIG big)
	{
		PBIG result = (PBIG)malloc(sizeof(BIG));
		result->capacity = big->capacity;
		result->sign = big->sign;
		result->size = big->size;
		result->digits = (PDIGIT)malloc(sizeof(DIGIT) * result->capacity);
		memcpy(result->digits, big->digits, big->capacity * sizeof(DIGIT));
		result->mostSig = result->digits + result->size - 1;
		return result;
	}

	//string has to have 0 at the end
	PBIG bigFromStr(char* str, int capacity)
	{
		PBIG result = (PBIG)malloc(sizeof(BIG));
		result->size = 0;
		result->sign = str[0] == '-' ? NEGATIVE : POSITIVE;
		result->capacity = capacity;
		result->digits = (PDIGIT)malloc(sizeof(DIGIT)*capacity);
		memset(result->digits, 0, sizeof(DIGIT)*capacity);
		int index = 0;
		for (int i = 0; str[i] != 0 && i + 1 <= capacity; i++)
		{
			if (str[i] < '0' || str[i] > '9')
				continue;
			(result->digits)[index] = str[i] - '0';
			(result->size)++;
			index++;
		}
		result->mostSig = result->digits + index - 1;
		return result;
	}

	char* bigToStr(PBIG big)
	{
		//decide whether we need one more for minus sign
		int minus = big->sign == NEGATIVE ? 1 : 0;
		char* result = (char*)malloc(sizeof(char) * big->size + 1 + minus);
		if (minus)
			result[0] = '-';
		PDIGIT iterator = big->mostSig;
		for (int i = minus; iterator >= big->digits; i++)
			result[i] = *(iterator--) + '0';
		result[big->size + minus] = 0;
		return result;
	}

	char* bigToExp(PBIG _in)
	{
		return NULL;
	}

	//add one num to another
	void addBig(PBIG src, PBIG dst, int shift)
	{
		int val;
		int carry = 0;
		int srcSize = src->size;
		PDIGIT srcIterator = src->digits;
		PDIGIT dstIterator = dst->digits + shift;
		//add digits
		for (int i = 0; i < srcSize; i++)
		{
			//increase capacity if it overflows
			if (i + 1 > dst->capacity)
			{
				int offset = dstIterator - dst->digits;
				increaseCapacity(dst, dst->size * 2);
				dstIterator = dst->digits + offset;
			}
			val = *dstIterator + *(srcIterator++) + carry;
			//take care of carry
			carry = 0;
			if (val > 9)
			{
				carry = val / 10;
				val %= 10;
			}
			//add
			*(dstIterator++) = val;
		}
		//finish carry
		while (carry != 0)
		{
			if (dstIterator > dst->digits + dst->capacity - 1)
			{
				int offset = dstIterator - dst->digits;
				increaseCapacity(dst, dst->size * 2);
				dstIterator = dst->digits + offset;
			}
			val = *dstIterator + carry;
			carry = 0;
			if (val > 9)
			{
				carry = val / 10;
				val %= 10;
			}
			*(dstIterator++) = val;
		}
		dstIterator--;
		//adjust size and most sig digit
		if (dstIterator > dst->mostSig)
		{
			dst->mostSig = dstIterator;
			dst->size = dst->mostSig - dst->digits + 1;
		}
	}

	//add two numbers into new one
	PBIG sum(PBIG num1, PBIG num2)
	{
		PBIG result = bigFromBig(num2);
		addBig(num1, result, 0);
		return result;
	}

	//multiply big by one digit int
	void mulOneDigitNoCarry(int num1, PBIG dst)
	{
		if (num1 == 0)
		{
			memset(dst->digits, 0, sizeof(DIGIT)*dst->capacity);
			dst->size = 1;
			dst->mostSig = dst->digits;
		}
		if (num1 == 1)
			return;
		if (num1 == -1)
		{
			dst->sign = dst->sign == POSITIVE ? NEGATIVE : POSITIVE;
			return;
		}
		PDIGIT dstIterator = dst->digits;
		int dstSize = dst->size;
		//mul digits
		for (int i = 0; i < dstSize; i++)
			*(dstIterator++) *= num1;

	}
	
	//dont change sign in this one, it uses mulOneDigit which takes care of sign
	PBIG mul(PBIG src, PBIG dst)
	{		
		PBIG* arr = (PBIG*)malloc(sizeof(PBIG)*src->size);
		for (int i = 0; i < src->size; i++)
			arr[i] = bigFromBig(dst);
		PDIGIT srcIterator = src->digits;
		//multiply all digits
		PBIG* dstIterator = arr;
		for (int i = 0; srcIterator <= src->mostSig;i++)
			mulOneDigitNoCarry(*(srcIterator++), *(dstIterator++));
		PBIG result = bigFromInt(0, dst->size + src->size + 1);
		for (int i = 0; i < src->size; i++)
			addBig(arr[i], result, i);
		for (int i = 0; i < src->size; i++)
			free(arr[i]);
		return result;
	}
	
	PBIG factorial(unsigned int val)
	{
		if (val < 2)
			return bigFromStr("1", 1);
		PBIG result = bigFromStr("1", 10);
		for (int i = 2; i <= val; i++)
		{
			PBIG a = bigFromInt(i, 5);
			PBIG b = mul(a, result);
			destroy(result);
			destroy(a);
			result = b;
		}
		return result;
	}

	PBIG pow(PBIG base, unsigned int exp)
	{
		if (exp == 0)
			return bigFromInt(1,1);
		if (exp == 1)
			return bigFromBig(base);
		PBIG result = bigFromBig(base);
		for (int i = 2; i <= exp; i++)
		{
			PBIG b = mul(result, base);
			destroy(result);
			result = b;
		}
		return result;

	}
}
