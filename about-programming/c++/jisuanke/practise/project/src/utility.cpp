#include "utility.h"

char GetOp(BinaryOperator B)
{
	switch (B)
	{
	case BinaryOperator::Plus:
		return '+';
	case BinaryOperator::Minus:
		return '-';
	case BinaryOperator::Multiply:
		return '*';
	case BinaryOperator::Divide:
		return '/';
	}
}
bool Is(const char*& Stream, const char* Text)
{
	size_t len = strlen(Text);
	const char* Read = Stream;
	while (*Read == ' ')Read++; // read skip space
	if (strncmp(Read, Text, len) == 0) // compare ,if true ,add stream to end, false, no modify, using *& like **
	{
		Stream = Read + len;
		return true;
	}
	else
	{
		return false;
	}
}
