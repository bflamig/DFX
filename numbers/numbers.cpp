// numbers.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cmath>
#include "EngrNum.h"

using namespace bryx;

/*

2^N   = X  = K 10^M + F


-20
-19
-18
-17
-16
-15
-14
-13
-12
-11
-10
 -9
 -8
 -7
 -6
 -5
 -4
 -3
 -2
 -1

 P2	 	      X		M		K		  F

  0	 	      1		0		1		  0 
  1 	      2		0		2		  0
  2 	      4     0       4         0
  3 	      8		0		8		  0
  4          16		1		1		  6
  5          32		1		3	  	  2
  6          64		1		6		  4
  7         128		2		1	     28
  8         256		2		2	     56
  9         512		2		5	     12
 10        1024		3		1	     24
 11        2048		3		2	     48
 12        4096		3		4        96
 13        8192		3		8       192
 14       16384		4		1      6384
 15       32768		4		3      2768
 16       65536		4		6      5536
 17      131072		5		1     31072
 18      262144		5		2     62144
 19      524288		5		5     24288
 20     1048576		6		1     48576
 21     2097152		6		2     97152
 22     4194304		6		4    194304
 23     8388608		6		8    388608
 24    16777216		7		1   6777216
 25    33554432		7		3   3554432 
 26    67108864		7       6   7108864
 27   134217728		8       1  34217728
 28   268435456     8		2  68435456
 29   536870912     8       5  36870912
 30  1073741824     9		1  73741824
 31  2147483648     9		2 147483648
 32  4294967296     9		4 294967296
 33  8589934592     9		8 589934592
 34                10       1  
 35                10       3
 36                10       6
 37				   11       1
 38				   11       2 
 39                11       5
 40                12       1
 41				   12       2
 42                12       4
 43                12       8
 44                13       1
 45                13       3
 46                13       6
 47                14       1 
 48                14       2
 49                14       5
 50                15       1 
 51                15       2
 52                15       4
 53                15       8
 54                16       1


*/

//using cout = std::cout;
//using endl = std::endl;

int test_positive()
{
	for (int i = 0; i < 56; i++)
	{
		int64_t p2 = static_cast<int64_t>(pow(2.0, i));
		double d10 = log10(double(p2));
		int p10 = int(d10);
		double xfactor = p2 / pow(10.0, p10);
		int64_t wpart = int64_t(xfactor);
		double fred = wpart * pow(10.0, p10);
		int64_t george = int64_t(fred);
		int64_t fpart = p2 - george;
		std::cout << i << ", " << p2 << ", " << p10 << ", " << wpart << ", " << fpart << std::endl;
	}
	return 0;
}

int test_negative()
{
	for (int i = 0; i < 16; i++)
	{
		double p2 = pow(2.0, -i);
		double d10 = log10(double(p2));
		int64_t p10 = int64_t(floor(d10));
		double xfactor = pow(10.0, p10);
		double mantissa = p2 / xfactor;
		//std::cout << i << ", " << p2 << ", " << d10 << ", " << p10 << ", " << xfactor << ", " << mantissa << std::endl;
		std::cout << i << ", " << p2 << ", " << mantissa << " x 10^" << p10 << std::endl;
	}
	return 0;
}

void testd()
{
	// 1234.56787e5 --> 123456787  --> 123.456787e6
	// 123.456787e5 --> 12345678.7 --> 12.3456787e6
	double d = 123.456787e5;
	double md = d / 1000.0;
	double dlog = log10(d);
	double mlog = log10(md);
	double imlog = ceil(mlog);
	double xd = md / 1000.0;

}

void testf()
{
	// 1234.56787e5 --> 123,456,787  --> 123.456787e6
	// 123.456787e5 --> 12,345,678.7 --> 12.3456787e6

	double d = 123.456787e5;
	//double dlog = log10(d);
	//double ilog = floor(dlog);
	//double mlog = ilog / 3.0;
	//double imlog = floor(mlog);

	double mlog = log10(d) / 3.0;
	double imlog = floor(mlog);

	double f = pow(10.0, 3 * imlog);

	double new_d = d / f;
}

void convert_to_engr(double d, double& new_d, int& mexp)
{
	// THIS IS THE MAGIC FORMULA

	double mlog = log10(d) / 3.0;
	mexp = int(floor(mlog));
	double f = pow(10.0, 3 * mexp);
	new_d = d / f;
}

void testg()
{
	// 1234.56787e5 --> 123,456,787  --> 123.456787e6
	// 123.456787e5 --> 12,345,678.7 --> 12.3456787e6

	double d = 123.456787e7;

	double new_d;
	int mexp;

	convert_to_engr(d, new_d, mexp);

	std::cout << d << " ---> " << new_d << " x 10^" << (mexp * 3) << std::endl;

	d = 123.456787e-4;

	convert_to_engr(d, new_d, mexp);

	std::cout << d << " ---> " << new_d << " x 10^" << (mexp*3) << std::endl;
}

void testengr(const char *src)
{
	EngrNum engr_num(64, 3);
	engr_num.parse(std::cout, src);
	std::cout << src << " --> " << engr_num << std::endl;
}

void testh()
{
	EngrNum engr_num(64, 3);

	//engr_num.Parse("-1234.56789e23");

	testengr("000.1234");    // Not legal json?

	testengr("45.0034");

	testengr("0");
	testengr("12");
	testengr("123");
	testengr("1234");
	testengr("12345");
	testengr("123456");
	testengr("1234567");
	testengr("12345678");
	testengr("123456789");
	testengr("-1234");
	testengr("123.4");
	testengr("12.34");
	testengr("1.234");
	testengr("0.1234");
	testengr("0.01234");
	testengr("0.001234");
	testengr("0.0001234");
	testengr("0.00001234");
	testengr("0.000001234");
	testengr("0.0000001234");
	testengr("0.00000001234");
	testengr("0.000000001234");
	testengr("1234.56789e18");
	testengr("1234.56789e19");
	testengr("1234.56789e20");
	testengr("1234.56789e21");
	testengr("1234.56789e22");
	testengr("1234.56789e23");
	testengr("1234.56789e24");
	testengr("1234.56789e25");
	testengr("1234.56789e26");
	testengr("1234.56789e-18");
	testengr("1234.56789e-19");
	testengr("1234.56789e-20");
	testengr("1234.56789e-21");
	testengr("1234.56789e-22");
	testengr("1234.56789e-23");
	testengr("1234.56789e-24");
	testengr("1234.56789e-25");
	testengr("1234.56789e-26");

	testengr("1234.56789%");
	testengr("123.456789X");
	testengr("12.3456789dB");
	testengr("1.23456789gold");

	testengr("123456789k");
	testengr("1234.56789k");
	testengr("123.456789k");
	testengr("12.3456789k");
	testengr("1.23456789k");
	//testengr("123456789k");
}


void testdb()
{
	//Lexi sun();
	//auto sbuf = f.rdbuf();
	//SetStreamBuf(fbuf);

	std::string test = "-30db";

	LexiNumberTraits traits;

	Lexi::CollectQuotedNumber(test, traits);

}

int main()
{
	//test_positive();
	//test_negative();
	//testd();
	//testf();
	//testg();
	//testh();
	testdb();
}
