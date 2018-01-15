#include "parser.h"
#include "utility.h"
using std::make_shared;
shared_ptr<Expression> GetNumber(const char*& Stream) // return a number or exception if no
{
	int Result = 0;
	bool GotNumber = false;
	/*�������*/
	const char* Read = Stream;
	/*���˿ո�*/
	while (*Read == ' ')Read++;
	while (true)
	{
		char c = *Read;
		/*����ַ��Ƿ�Ϊ����*/
		if ('0' <= c && c <= '9')
		{
			Result = Result * 10 + (c - '0');
			GotNumber = true;
			Read++;
		}
		else
		{
			break;
		}
	}
	if (GotNumber)
	{
		Stream = Read;

		return make_shared<NumberExpression>(Result);
	}
	else
	{
		throw Exception(Stream, "�˴���Ҫ����ʽ");
	}
}
shared_ptr<Expression> GetTerm(const char*& Stream) // return an expression in () or a number
{
	try
	{
		return GetNumber(Stream);
	}
	catch (Exception& e)
	{
		const char* Read = Stream;
		if (Is(Read, "("))
		{
			auto Result = GetExp(Read);
			if (Is(Read, ")"))
			{
				Stream = Read;
				return Result;
			}
			else
			{
				throw Exception(Stream, "�˴���Ҫ������");
			}
		}
		else
		{
			throw e;
		}
	}
}

shared_ptr<Expression> GetFactor(const char*& Stream) // return a expression in () or number or an expression xx */x
{
	const char* Read = Stream;
	shared_ptr<Expression> Result = GetTerm(Read);
	while (true)
	{
		BinaryOperator Operator;
		if (Is(Read, "*"))
		{
			Operator = BinaryOperator::Multiply;
		}
		else if (Is(Read, "/"))
		{
			Operator = BinaryOperator::Divide;
		}
		else
		{
			break;
		}

		Result = make_shared<BinaryExpression>(Operator, Result, GetTerm(Read));

	}
	Stream = Read;
	return Result;
}

shared_ptr<Expression> GetExp(const char*& Stream)
{
	const char* Read = Stream;
	shared_ptr<Expression> Result = GetFactor(Read);
	while (true)
	{
		BinaryOperator Operator;
		if (Is(Read, "+"))
		{
			Operator = BinaryOperator::Plus;
		}
		else if (Is(Read, "-"))
		{
			Operator = BinaryOperator::Minus;
		}
		else
		{
			break;
		}
		Result = make_shared<BinaryExpression>(Operator, Result, GetFactor(Read));

	}
	Stream = Read;
	return Result;

}