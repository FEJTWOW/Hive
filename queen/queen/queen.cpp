// queen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>
#pragma comment(lib, "user32.lib")

//TCHAR szName1[]=TEXT("Local\\MyFileMappingObject1");
TCHAR szName2[]=TEXT("Local\\MyFileMappingObject2");


/*typedef struct fbed 
{
	int distance; // how far from hive is our flower bed 
	int number_of_flowers; // how many flowers have our flower bed
	int flower_nectar[100]; // how much nectar has each flower
};*/



typedef struct hive_stats 
{
	int number_of_honey;
	int capacity_of_hive;
	int number_of_flowerbeds;
	int bees_outside;
	int bees_inside;
	int current_number_of_bees;
	int total_bees_created;
	int dead_bees;
	BOOL still_running;
};

int main(int argc, char* argv[])
{

	// READING FROM SHARED MEMORY 2
	HANDLE hMapFile2;
    hive_stats* pBuf2;

    hMapFile2 = OpenFileMapping(
                    FILE_MAP_ALL_ACCESS,   // read/write access
                    FALSE,                 // do not inherit the name
                    szName2);               // name of mapping object

    if (hMapFile2 == NULL)
    {
       _tprintf(TEXT("Could not open file mapping object (%d).\n"),
              GetLastError());
       return 1;
    }

    pBuf2 = (hive_stats*) MapViewOfFile(hMapFile2, // handle to map object
                FILE_MAP_ALL_ACCESS,  // read/write permission
                0,
                0,
                sizeof(hive_stats));

    if (pBuf2 == NULL)
    {
       _tprintf(TEXT("Could not map view of file (%d).\n"),
              GetLastError());

       CloseHandle(hMapFile2);

       return 1;
    }
	//--------------------------------------------------------------


	/* READING FROM SHARED MEMORY 1

	HANDLE hMapFile1;
    fbed* pBuf1;

    hMapFile1 = OpenFileMapping(
                    FILE_MAP_ALL_ACCESS,   // read/write access
                    FALSE,                 // do not inherit the name
                    szName1);               // name of mapping object

    if (hMapFile1 == NULL)
    {
       _tprintf(TEXT("Could not open file mapping object (%d).\n"),
              GetLastError());
       return 1;
    }

    pBuf1 = (fbed*) MapViewOfFile(hMapFile1, // handle to map object
                FILE_MAP_ALL_ACCESS,  // read/write permission
                0,
                0,
				sizeof(fbed)*pBuf2->number_of_flowerbeds);

    if (pBuf1 == NULL)
    {
       _tprintf(TEXT("Could not map view of file (%d).\n"),
              GetLastError());

       CloseHandle(hMapFile1);

       return 1;
    }
	//--------------------------------------------------------------*/

	// OPENING BEES MUTEX 
	HANDLE hMutex_bees = OpenMutex(MUTEX_ALL_ACCESS,FALSE,TEXT("bees"));
	// ------------------
	int i=0;
	while( true )
	{
		srand(time(NULL)+GetCurrentProcessId());
		STARTUPINFOA si[1024] = {0};
		PROCESS_INFORMATION pi[1024] = {0};


		if( !pBuf2->still_running )
		{
			for( int i=0; i< pBuf2->current_number_of_bees; ++i )
			{
				if( pi[i].hThread != NULL && pi[i].hProcess != NULL )
				{
					TerminateThread( pi[i].hThread, 0 );
					TerminateProcess( pi[i].hProcess, 0);
					CloseHandle( pi[i].hProcess );
					CloseHandle( pi[i].hThread );
				}
			}
			break;
		}
		if( pBuf2->current_number_of_bees < pBuf2->capacity_of_hive )
		{
			if( !CreateProcessA( "C:\\Users\\kamil\\Desktop\\ró¿ne\\systemy_operacyjne\\worker\\Debug\\worker.exe",   // No module name (use command line)
			NULL,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si[i++],            // Pointer to STARTUPINFO structure
			&pi[i++] )           // Pointer to PROCESS_INFORMATION structure
			) 
			{
			printf( "CreateProcess failed (%d).\n", GetLastError() );
			break;
			}
			pBuf2->total_bees_created++;
			BOOL bbeesnewContinue = TRUE;
			DWORD dwbeesnewWaitResult;
			while( bbeesnewContinue )
			{
				dwbeesnewWaitResult = WaitForSingleObject( hMutex_bees, 0L );
				switch(dwbeesnewWaitResult)
				{
					case WAIT_OBJECT_0:
					{
						bbeesnewContinue = FALSE;
						pBuf2->bees_inside += 1;
						pBuf2->current_number_of_bees += 1;
						ReleaseMutex(hMutex_bees);
						break; // we dont need this break here 
					}
					case WAIT_ABANDONED: break;
				}
			
			}

			Sleep(rand()%100+101);
			
		}
	}

	






	/* Closing shared memory 1 
    UnmapViewOfFile(pBuf1);
    CloseHandle(hMapFile1);
	 -------------------------*/ 
	CloseHandle(hMutex_bees);
	// Closing shared memory 2
	UnmapViewOfFile(pBuf2);
    CloseHandle(hMapFile2);
	//--------------------------
	//system("pause");

	return 0;
}

