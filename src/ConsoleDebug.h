#pragma once
#include <windows.h>
#include <stdio.h>
#include <iostream>

class CConsoleDebug
{
public:
    CConsoleDebug(): m_pFile(NULL)
    {
#ifdef _DEBUG
        AllocConsole();
        freopen_s(&m_pFile, "CONOUT$", "w", stdout);
         HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
//         SetConsoleTextAttribute(hCon, FOREGROUND_INTENSITY);
		 COORD   size;
		 size.X=80;
		 size.Y=3000;
		::SetConsoleScreenBufferSize(hCon, size);
        std::ios_base::sync_with_stdio();
#endif
    }

    virtual ~CConsoleDebug()
    {
#ifdef _DEBUG
        fclose(m_pFile);
        m_pFile = NULL;
        FreeConsole();
#endif
    }
private:
    FILE* m_pFile;
};