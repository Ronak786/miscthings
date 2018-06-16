/*************************************************************************
	> File Name: square.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Fri 15 Jun 2018 02:31:07 PM CST
 ************************************************************************/

#include<iostream>
#include <cmath>

long int findNextSquare(long int sq) {
  // Return the next square if sq if a perfect square, -1 otherwise
  bool flag = false;
  long int i = 0, mul = 0;
  while ((mul = i * i) <= sq) {
	  if (mul == sq) {
		  flag = true;
	  }
	  ++i;
  }
  return flag : mul : -1;
}


