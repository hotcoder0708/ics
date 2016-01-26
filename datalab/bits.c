/* 
 * CS:APP Data Lab 
 * 
 * <name:Jiaxing Hu   userid:jiaxingh>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
/* x ^ y = ~(~(~x & y) & ~(~y & x))*/
	int answer1 = ~x & y;
	int answer2 = x & ~y;
	int answer = 0;
	answer1 = ~answer1;
	answer2 = ~answer2;
	answer = answer1 & answer2;
	answer = ~answer;
	return answer;

}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
/*
 * min = 0x10000000
 */
	int answer = 0x80 << 24;
	return answer;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 */
int isTmax(int x) {
/*
 *  max = 0x7FFFFFFF
 */
	int x_pos = ~x;
	int former = ~(x + 1) ^ x;
	x_pos = !x_pos;
	return ! (former | x_pos);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
/* answer = !((x & 0xAAAAAAAA) ^ 0xAAAAAAAA)*/
	int cover = 0xAA;
	int cover1 = cover + (cover << 8);
	int cover2 = cover1 + (cover1 << 16);
	return !((x & cover2) ^ cover2);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
/*
 * negate = ~x + 1;
 */
	int answer = ~x + 1;
	return answer;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
/*  
 *  former 4 digits:former =  !((x & 11110000) ^ 00110000)
 *  latter 4 digits:latter =  !((00001001 + (~(x & 00001111) + 1)) & 10000000)
 *  answer = former & latter
 */
	int begin1 = x >> 8;
	int begin2 = begin1 ^ 0x00;
	int begin = !begin2;
	int former1 = x & 0xF0;
	int former2 = former1 ^ 0x30;
	int former = !former2;
	int latter1 = x & 0x0F;
	int latter2 = ~latter1 + 1;
	int latter3 = 0x09 + latter2;
	int latter4 = latter3 & 0x80;
	int latter = !latter4;
	int answer = former & latter & begin;
	return answer;
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
/*
 *   flagz = ~(!x) + 1;
 *   flagy = ~flagz;
 *   answer = y & flagy + z & flagz;
 */
	int flag = !x;
	int flagz = ~flag + 1;
	int flagy = ~flagz;
	int answer = (y & flagy) + (z & flagz);
	return answer;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
/*
 * y - x  < 0 implies 0
 */
	int flag_x1 = x >> 31;
	int flag_y1 = y >> 31;
	int flag_x = flag_x1 & 0x01;
    int flag_y = flag_y1 & 0x01;
	int flag1 = flag_x ^ flag_y;//0 present answer is right
	int flag2 = !flag1;//0 present flag_x is right
	int neg_x = ~x + 1;
	int answer1 = y + neg_x;
	int answer = answer1 >> 31;
	return ((!answer) & flag2) | (flag_x & flag1);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
/*
 *  *  if x is not positive and also not negative, then it should be 0
 *   *  x >> 31 + 1 == 1 implies x is not negative
 *    *  (~x + 1) >> 31 + 1 == 1 implies x is not positive
 *     *  then x should be 0
 *      */
        int is_negative = (x >> 31) + 1;
        int is_positive = ((~x + 1) >> 31) + 1;
        int answer = is_negative & is_positive;
	return answer;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *
 *  Rating: 4
 */
int howManyBits(int x) {
/*
 *    translate x to positive number
 *    this answer should be a five digits binary number
 *    so judge every digit
 */
	int pos_x= x ^ (x >> 31);
	int is_zero =!(pos_x ^ 0x0);
	int digit4,digit3,digit2,digit1,digit0,answer;
	is_zero = ~is_zero + 1;
	digit4 = !(!(pos_x >> 16)) << 4;
	answer = digit4;
	digit3 = !(!(pos_x >> (answer + 8))) << 3;
        answer = answer + digit3;
        digit2 = !(!(pos_x >> (answer + 4))) << 2;
        answer = answer + digit2;
        digit1 = !(!(pos_x >> (answer + 2))) << 1;
	answer = answer + digit1;
	digit0 = !(!(pos_x >> (answer + 1)));
	answer = answer + digit0;
	return answer + 2 + is_zero;
}
//float
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
	unsigned e = (uf & 0x7F800000) >> 23;
	unsigned s = uf & 0x80000000;
	if (e == 0xFF)//when uf is infinite or NaN
		return uf;
	else if (e == 0xFE)
		return (uf + 0x00800000) & 0xFF800000;
	else if (e < 0xFE && e > 0) //when it is normal
		return uf + 0x00800000;
	else if (e == 0){
		return (uf << 1) + s;
		}
	return 0;// if something happen...
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
	int s,x_pos,x_pos1,digit_number,f,e,shift,flag,flag2,answer,temp1;
	if (x == 0)
		return 0;
	s = 0;
	x_pos = x;
	if (x < 0){
		s = 0x80000000;
		x_pos = x * -1;
	}
	x_pos1 = x_pos;
	digit_number = 0;
	while((x_pos1 & 0x80000000) == 0){
		digit_number++;
		x_pos1 = x_pos1 << 1;
	}
	x_pos1 = x_pos1 & 0x7FFFFFFF;
	f = x_pos1 >> 8;
	digit_number = 32 - digit_number;
	e = digit_number + 126;
	
	//int f = 0;
	//int temp2 = digit_number - 1;
	//int temp = 0x1 << temp2;
	//f = x_pos - temp;
	shift = digit_number - 24;
	flag = 0;
	flag2 = 0;
	if (digit_number > 24){
		temp1 = 0x1 << (shift - 1);
		if (temp1 & x_pos){
			flag = 1;
			if(temp1 == (x_pos & (0xFFFFFFFF >> (32 - shift))))
				flag2 = 1;
		}
		//f = f >> shift;
	}
	//else
		//f = f << (-shift);
	answer = s + (e << 23) + f + flag;
	if(flag2 && (answer & 0x1))
		return answer - 1;
	return answer;
		
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
	int e;
	int f;
	int s;
	int e1,temp;
	s = uf >> 31;
	f = (uf & 0x007FFFFF) + 0x00800000;
	e = uf & 0x7F800000;
	e1 = e >> 23;
	e1 = e1 - 127;
	if (e == 0x7F800000)
		return 0x80000000u;
	else if (e < 0x3F800000)
		return 0;
	else {
		if (e1 > 30){
			return 0x80000000;
		}
		else if(e1 > 23){
			temp = e1 - 23;
			f = f << temp;
		}
		else{
			temp = 23 - e1;
			f = f >> temp;
		}
		if (s == 1){
			f = ~f + 1;
		}
		return f;
	}
}
