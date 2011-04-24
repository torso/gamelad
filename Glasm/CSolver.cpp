#include	<windows.h>

#define		CSOLVER_CPP

#include	"..\CString\CString.h"
#include	"..\Error.h"
#include	"CSolver.h"
#include	"Glasm.h"



#define		CSF_NUMBER			0x01
#define		CSF_EXPRESSION		0x02

#define		NUMBER_HEXADECIMAL	0x00000001
#define		NUMBER_DECIMAL		0x00000002
#define		NUMBER_OCTAL		0x00000004
#define		NUMBER_BINARY		0x00000008
#define		NUMBER_RADIX		0x0000000F



#define		CNF_NUMBER			0x01
#define		CNF_DWNUMBER		0x02



class CNumber
{
private:
	DWORD		dwFlags;
	DWORD		dwNumber;
	CString		*Number;

public:
	CNumber();
	~CNumber();

	void		operator =(DWORD dwNewNumber);
};



CNumber::CNumber()
{
	dwFlags = 0;
}



CNumber::~CNumber()
{
	if (dwFlags & CNF_NUMBER)
	{
		delete Number;
	}
}



void CNumber::operator =(DWORD dwNewNumber)
{
	if (dwFlags & CNF_NUMBER)
	{
		dwFlags &= ~CNF_NUMBER;
		delete Number;
	}

	dwFlags |= CNF_DWNUMBER;
	dwNumber = dwNewNumber;
}



CSolver::CSolver()
{
	dwFlags = 0;
	dwNumber = 0;
}



CSolver::~CSolver()
{
	if (dwFlags & CSF_EXPRESSION)
	{
		delete pExpression;
	}
}



void CSolver::operator =(DWORD dwNewNumber)
{
	if (dwFlags & CSF_EXPRESSION)
	{
		dwFlags &= ~CSF_EXPRESSION;
		delete pExpression;
	}

	dwFlags |= CSF_NUMBER;
	dwNumber = dwNewNumber;
}



BOOL CSolver::ReadNumber(CString &String, DWORD *Pos, CString &Number, DWORD *dwNewNumber, DWORD Flags)
{
	DWORD		Pos2, Pos3;


	Number = (char *)NULL;

	while (String[*Pos] == '0' || String[*Pos] == '1' || String[*Pos] == '2' || String[*Pos] == '3' || String[*Pos] == '4' || String[*Pos] == '5'
		|| String[*Pos] == '6' || String[*Pos] == '7' || String[*Pos] == '8' || String[*Pos] == '9'
		|| String[*Pos] == 'A' || String[*Pos] == 'B' || String[*Pos] == 'C' || String[*Pos] == 'D' || String[*Pos] == 'E' || String[*Pos] == 'F'
		|| String[*Pos] == 'a' || String[*Pos] == 'b' || String[*Pos] == 'c' || String[*Pos] == 'd' || String[*Pos] == 'e' || String[*Pos] == 'f')
	{
		if (Number += String[(*Pos)++])
		{
			return true;
		}
	}

	if (Number.GetLength() == 0)
	{
		CompileError(COMPILE_ERROR_BADNUMBER, NULL);
		*Pos = -1;
		return false;
	}

	switch (String[*Pos])
	{
	case 'h':
	case 'H':
		if (Flags & NUMBER_RADIX)
		{
			CompileError(COMPILE_ERROR_BADNUMBER, NULL);
			*Pos = -1;
			return false;
		}
		Flags |= NUMBER_HEXADECIMAL;
		*Pos++;
		break;

	case 'q':
	case 'Q':
		if (Flags & NUMBER_RADIX)
		{
			CompileError(COMPILE_ERROR_BADNUMBER, NULL);
			*Pos = -1;
			return false;
		}
		Flags |= NUMBER_OCTAL;
		*Pos++;
		break;

	case 'b':
	case 'B':
		if (Flags & NUMBER_RADIX)
		{
			CompileError(COMPILE_ERROR_BADNUMBER, NULL);
			*Pos = -1;
			return false;
		}
		Flags |= NUMBER_BINARY;
		*Pos++;
		break;
	}

	if (!(Flags & NUMBER_RADIX))
	{
		Flags |= NUMBER_DECIMAL;
	}

	Pos2 = 0;
	Pos3 = 0;
	while (Pos2 < Number.GetLength())
	{
		switch (Number[Pos2])
		{
		case 'f':
		case 'F':
		case 'e':
		case 'E':
		case 'd':
		case 'D':
		case 'c':
		case 'C':
		case 'b':
		case 'B':
		case 'a':
		case 'A':
			if (Flags & (NUMBER_DECIMAL | NUMBER_OCTAL | NUMBER_BINARY))
			{
				CompileError(COMPILE_ERROR_BADNUMBER, NULL);
				*Pos = -1;
				return false;
			}
			break;
		case '9':
			if (Flags & (NUMBER_OCTAL | NUMBER_BINARY))
			{
				CompileError(COMPILE_ERROR_BADNUMBER, NULL);
				*Pos = -1;
				return false;
			}
			break;
		case '8':
		case '7':
		case '6':
		case '5':
		case '4':
		case '3':
		case '2':
			if (Flags & NUMBER_BINARY)
			{
				CompileError(COMPILE_ERROR_BADNUMBER, NULL);
				*Pos = -1;
				return false;
			}
			break;
		case '1':
			break;

		case '0':
			if (Pos3 == Pos2)
			{
				Pos3++;
			}
			break;

		default:
			CompileError(COMPILE_ERROR_BADNUMBER, NULL);
			*Pos = -1;
			return false;
		}

		Pos2++;
	}

	if (Pos3 == Pos2)
	{
		*dwNewNumber = 0;
		Number = (char *)NULL;
		return false;
	}

	switch (Flags & NUMBER_RADIX)
	{
	case NUMBER_HEXADECIMAL:
		if (Number.GetLength() - Pos3 <= 8)
		{
			*dwNewNumber = 0;
			do
			{
				*dwNewNumber |= (Number[Pos3] - (Number[Pos3] >= 'a' ? 'a' : Number[Pos3] >= 'A' ? 'A' : '0')) << ((Number.GetLength() - Pos3 - 1) * 4);
			}
			while (++Pos3 < Pos2);
			Number = (char *)NULL;
			return false;
		}
		break;

	case NUMBER_DECIMAL:
		if (Number.GetLength() - Pos3 <= 10)
		{
			*dwNewNumber = 0;
			do
			{
				*dwNewNumber += (Number[Pos3] - '0') * ((Number.GetLength() - Pos3 - 1) * 10);
			}
			while (++Pos3 < Pos2);
			Number = (char *)NULL;
			return false;
		}
		break;

	case NUMBER_OCTAL:
		if (Number.GetLength() - Pos3 <= 16)
		{
			*dwNewNumber = 0;
			do
			{
				*dwNewNumber |= (Number[Pos3] - '0') << ((Number.GetLength() - Pos3 - 1) * 2);
			}
			while (++Pos3 < Pos2);
			Number = (char *)NULL;
			return false;
		}
		break;

	case NUMBER_BINARY:
		if (Number.GetLength() - Pos3 <= 32)
		{
			*dwNewNumber = 0;
			do
			{
				*dwNewNumber |= (Number[Pos3] - '0') << (Number.GetLength() - Pos3 - 1);
			}
			while (++Pos3 < Pos2);
			Number = (char *)NULL;
			return false;
		}
		break;
	}

	CompileError(COMPILE_ERROR_BADNUMBER, NULL);
	*Pos = -1;
	return false;
}



BOOL CSolver::operator =(CString &String)
{
	CString		Number;
	DWORD		Pos = 0, dwNumber;


	if (dwFlags & CSF_EXPRESSION)
	{
		dwFlags &= ~CSF_EXPRESSION;
		delete pExpression;
	}
	dwFlags &= ~CSF_NUMBER;

	while (Pos < String.GetLength())
	{
		if (String[Pos] == '0')
		{
			switch (String[Pos + 1])
			{
			case 'x':
			case 'X':
				Pos += 2;
				if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_HEXADECIMAL))
				{
					return true;
				}
				break;

			case 'n':
			case 'N':
				Pos += 2;
				if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_DECIMAL))
				{
					return true;
				}
				break;

			case 'q':
			case 'Q':
				Pos += 2;
				if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_OCTAL))
				{
					return true;
				}
				break;

			case 'b':
			case 'B':
				Pos += 2;
				if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_BINARY))
				{
					return true;
				}
				break;

			default:
				if (ReadNumber(String, &Pos, Number, &dwNumber, 0))
				{
					return true;
				}
			}
			if (Pos == -1)
			{
				return false;
			}
			if (Number == "")
			{
				//dwNumber
			}
			else
			{
			}
		}
		else
		{
			switch (String[Pos])
			{
			case ' ':
			case 9:
			case '[':
			case ']':
				Pos++;
				break;

			/*case '(':
				break;

			case ')':
				break;*/

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (ReadNumber(String, &Pos, Number, &dwNumber, 0))
				{
					return true;
				}
				if (Pos == -1)
				{
					return false;
				}
				if (Number == "")
				{
					//dwNumber
				}
				else
				{
				}
				break;

			case '$':
				if (String[Pos + 1] == ' ' || String[Pos + 1] == 9 || String[Pos + 1] == '\0')
				{
					Pos++;
				}
				else
				{
					Pos++;
					if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_HEXADECIMAL))
					{
						return true;
					}
					if (Pos == -1)
					{
						return false;
					}
					if (Number == "")
					{
						//dwNumber
					}
					else
					{
					}
				}
				break;

			case '&':
				Pos++;
				if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_OCTAL))
				{
					return true;
				}
				if (Pos == -1)
				{
					return false;
				}
				if (Number == "")
				{
					//dwNumber
				}
				else
				{
				}
				break;

			case '%':
				Pos++;
				if (ReadNumber(String, &Pos, Number, &dwNumber, NUMBER_BINARY))
				{
					return true;
				}
				if (Pos == -1)
				{
					return false;
				}
				if (Number == "")
				{
					//dwNumber
				}
				else
				{
				}
				break;
			}
		}
	}

	return false;
}



BOOL CSolver::operator =(CSolver *NewNumber)
{
	if (dwFlags & CSF_EXPRESSION)
	{
		delete pExpression;
	}

	dwFlags = NewNumber->dwFlags;
	dwNumber = NewNumber->dwNumber;

	if (NewNumber->dwFlags & CSF_EXPRESSION)
	{
		if (!(dwFlags & CSF_EXPRESSION))
		{
			if (!(pExpression = new CString()))
			{
				FatalError(FATAL_ERROR_OUTOFMEMORY, NULL);
				return true;
			}
		}
		if (*pExpression = NewNumber->pExpression)
		{
			return true;
		}
	}

	return false;
}



BOOL CSolver::operator +=(DWORD dwNewNumber)
{
	if (dwFlags & CSF_EXPRESSION)
	{
		return true;
	}

	if (!(dwFlags & CSF_NUMBER))
	{
		dwFlags |= CSF_NUMBER;
		dwNumber = 0;
	}

	if (0 - dwNumber > dwNewNumber)
	{
		return true;
	}

	dwNumber += dwNewNumber;

	return false;
}

