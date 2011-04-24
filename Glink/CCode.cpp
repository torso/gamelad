#include	<windows.h>
#include	<iostream.h>
#include	"glink.h"
#include	"ccode.h"



CCode::CCode()
{
	Offset = 0;
	BankNo = 0;
	Machine = MACHINE_UNKNOWN;

	ZeroMemory(&Banks, sizeof(Banks));
}



CCode::~CCode()
{
	int		nBank;


	for (nBank = 1; nBank < MAX_BANK; nBank++)
	{
		if (Banks[nBank])
		{
			delete Banks[nBank];
		}
	}
}



BOOL CCode::SpaceUsed(WORD Size, WORD File, DWORD LineFrom, DWORD Lines)
{
	if (Offset >= 0x8000)
	{
		Error("Constant in RAM.");
		return true;
	}

	if ((Offset & ~0x4000) + Size >= 0x4000)
	{
		Error("Code exceeds size of bank.");
		return true;
	}

	while (Size--)
	{
		if (Banks[BankNo]->Used[((Offset + Size) & 0x3FFF) >> 3] & (1 << ((Offset + Size) & 7)))
		{
			while (Size--)
			{
				Banks[BankNo]->Used[((Offset + Size) & 0x3FFF) >> 3] |= (1 << ((Offset + Size) & 7));
			}
			Error("Code overlaps used ROM.");
			return true;
		}
		Banks[BankNo]->Used[((Offset + Size) & 0x3FFF) >> 3] |= (1 << ((Offset + Size) & 7));
		Banks[BankNo]->File[(Offset + Size) & 0x3FFF] = File;
		Banks[BankNo]->LineFrom[(Offset + Size) & 0x3FFF] = LineFrom;
		Banks[BankNo]->Lines[(Offset + Size) & 0x3FFF] = Lines;
	}

	return false;
}



BOOL CCode::InsertCode(DWORD Code, WORD Size, WORD File, DWORD LineFrom, DWORD LineTo)
{
	DWORD	Lines;


	Lines = LineTo - LineFrom;
	if (SpaceUsed(Size, File, LineFrom, Lines))
	{
		return true;
	}


	while (Size--)
	{
		Banks[BankNo]->Bank[Offset & 0x3FFF] = (BYTE)Code;
		Banks[BankNo]->Expression[Offset & 0x3FFF] = (char *)-1;
		Offset++;
		Code >>= 8;
	}

	return false;
}



BOOL CCode::InsertCode(BYTE *Code, WORD Size, WORD File, DWORD LineFrom, DWORD LineTo)
{
	DWORD	Lines;


	Lines = LineTo - LineFrom;
	if (SpaceUsed(Size, File, LineFrom, Lines))
	{
		return true;
	}


	while (Size--)
	{
		Banks[BankNo]->Bank[Offset & 0x3FFF] = Code[Size];
		Banks[BankNo]->Expression[Offset & 0x3FFF] = (char *)-1;
		Offset++;
	}

	return false;
}



BOOL CCode::InsertUnknownCode(WORD Size, WORD File, DWORD LineFrom, DWORD LineTo)
{
	DWORD	Lines;


	Lines = LineTo - LineFrom;
	if (SpaceUsed(Size, File, LineFrom, Lines))
	{
		return true;
	}


	Offset += Size;

	return false;
}



BOOL CCode::InsertExpression(char *Expression, BYTE Size, WORD File, DWORD LineFrom, DWORD LineTo)
{
	DWORD	Lines;


	Lines = LineTo - LineFrom;
	if (SpaceUsed(Size, File, LineFrom, Lines))
	{
		return true;
	}


	if (!(Banks[BankNo]->Expression[Offset & 0x3FFF] = new char[strlen(Expression) + 1]))
	{
		cout << "Insufficient memory." << endl;
		FatalError = true;
		return true;
	}

	strcpy(Banks[BankNo]->Expression[Offset & 0x3FFF], Expression);


	Offset += Size;

	return false;
}



BOOL CCode::SetOffset(BYTE NewBank, WORD NewOffset)
{
	if (NewOffset < 0x4000)
	{
		if (NewBank)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0x4000 && NewOffset < 0x8000)
	{
		if (NewBank >= MAX_BANK)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0x8000 && NewOffset < 0xA000)
	{
		if (NewBank > 1)
		{
			Error("Invalid bank.");
			return true;
		}
		if (NewBank)
		{
			Machine = MACHINE_CGB;
		}
	}
	if (NewOffset >= 0xA000 && NewOffset < 0xC000)
	{
		if (NewBank >= 16)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0xC000 && NewOffset < 0xD000)
	{
		if (NewBank)
		{
			Error("Invalid bank.");
			return true;
		}
	}
	if (NewOffset >= 0xD000 && NewOffset < 0xE000)
	{
		if (NewBank < 1 || NewBank > 7)
		{
			Error("Invalid bank.");
			return true;
		}
		if (NewBank)
		{
			Machine = MACHINE_CGB;
		}
	}
	if (NewOffset > 0xE000)
	{
		if (NewBank)
		{
			Error("Invalid bank.");
			return true;
		}
	}


	if (!Banks[NewBank])
	{
		if (!(Banks[NewBank] = new BANK))
		{
			cout << "Insufficient memory." << endl;
			FatalError = true;
			return true;
		}
		ZeroMemory(Banks[NewBank], sizeof(*Banks[NewBank]));
	}

	Offset = NewOffset;
	BankNo = NewBank;

	return false;
}



WORD CCode::GetOffset()
{
	return Offset;
}



BYTE CCode::GetBank()
{
	return BankNo;
}



BYTE CCode::GetMachine()
{
	return Machine;
}



BOOL CCode::SaveObj(HANDLE hFile)
{
	unsigned int	nBank, Offset, Size;
	DWORD			BytesWritten;


	for (nBank = 0; nBank < MAX_BANK; nBank++)
	{
		if (Banks[nBank])
		{
			for (Offset = 0; Offset < 0x4000; Offset++)
			{
				if (Banks[nBank]->Used[(Offset & 0x3FFF) >> 3] & (1 << (Offset & 7)))
				{
					//WORD		File
					WriteToFile(hFile, &Banks[nBank]->File[Offset], sizeof(WORD));
					//DWORD		Line
					WriteToFile(hFile, &Banks[nBank]->LineFrom[Offset], sizeof(DWORD));
					//BYTE		*Address
					WriteToFile(hFile, &nBank, sizeof(BYTE));
					WriteToFile(hFile, &Offset, sizeof(WORD));
					//DWORD		nLocals
					//BYTE		nLocalSymbols
					WriteToFile(hFile, "\1\0\0\0", sizeof(DWORD) + sizeof(BYTE));
					do
					{
						//DWORD		LineOffset
						WriteToFile(hFile, &Banks[nBank]->Lines[Offset], sizeof(DWORD));
						switch ((int)Banks[nBank]->Expression[Offset])
						{
						case -1:
							//BYTE	Type
							WriteToFile(hFile, "", sizeof(BYTE));
							//WORD	Size
							for (Size = 1; (Offset + Size) < 0x4000 && Banks[nBank]->Used[((Offset + Size) & 0x3FFF) >> 3] & (1 << ((Offset + Size) & 7))
								&& !Banks[nBank]->LineFrom[Offset + Size] && Banks[nBank]->Expression[Offset + Size] == 0; Size++);
							WriteToFile(hFile, &Size, sizeof(WORD));
							//BYTE	Code[Size]
							WriteToFile(hFile, &Banks[nBank]->Bank[Offset], Size);
							break;

						case 0:
							//BYTE	Type
							WriteToFile(hFile, "\1", sizeof(BYTE));
							//WORD	Size
							for (Size = 1; (Offset + Size) < 0x4000 && Banks[nBank]->Used[((Offset + Size) & 0x3FFF) >> 3] & (1 << ((Offset + Size) & 7))
								&& !Banks[nBank]->LineFrom[Offset + Size] && Banks[nBank]->Expression[Offset + Size] == (char *)-1; Size++);
							WriteToFile(hFile, &Size, sizeof(WORD));
							break;

						default:
							//BYTE	Type
							WriteToFile(hFile, "\2", sizeof(BYTE));
							//WORD	Size
							Size = Banks[nBank]->Bank[Offset];
							WriteToFile(hFile, &Size, sizeof(WORD));
							//char	szExpression[]
							WriteToFile(hFile, Banks[nBank]->Expression[Offset], strlen(Banks[nBank]->Expression[Offset]) + 1);
							break;
						}
						Offset += Size;
					}
					while (Offset < 0x4000 && Banks[nBank]->Used[(Offset & 0x3FFF) >> 3] & (1 << (Offset & 7)) && Banks[nBank]->LineFrom[Offset] == Banks[nBank]->LineFrom[Offset - Size] + Banks[nBank]->Lines[Offset - Size]);
				}
			}
		}
	}

	return false;
}

