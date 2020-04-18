// hive.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>
#include <string.h>

typedef struct fbed 
{
	int distance; // how far from hive is our flower bed 
	int number_of_flowers; // how many flowers have our flower bed
	int flower_nectar[100]; // how much nectar has each flower
	BOOL isEmpty;
};


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

TCHAR szName1[]=TEXT("Local\\MyFileMappingObject1");
TCHAR szName2[]=TEXT("Local\\MyFileMappingObject2");

int main(int argc, char* argv[])
{
	// READING ARGUMENTS //
	if( argc != 4 )
	{
		printf("Too few arguments!!!@!@!");
		return -1;
	}
	int P1 = atoi(argv[1]);
	int P2 = atoi(argv[2]);
	int P3 = atoi(argv[3]);
	//----------------------


	// CREATING SHARED MEMORY 1
	HANDLE hMapFile1;
	fbed* pBuf1;

    hMapFile1 = CreateFileMapping(
                  INVALID_HANDLE_VALUE,    // use paging file
                  NULL,                    // default security
                  PAGE_READWRITE,          // read/write access
                  0,                       // maximum object size (high-order DWORD)
                  sizeof(fbed)*P1,                // maximum object size (low-order DWORD)
                  szName1);                 // name of mapping object

    if (hMapFile1 == NULL)
    {
       _tprintf(TEXT("Could not create file mapping object (%d).\n"),
              GetLastError());
       return 1;
    }
    pBuf1 = (fbed*) MapViewOfFile(hMapFile1,   // handle to map object
                         FILE_MAP_ALL_ACCESS, // read/write permission
                         0,
                         0,
                         sizeof(fbed)*P1);

    if (pBuf1 == NULL)
    {
       _tprintf(TEXT("Could not map view of file (%d).\n"),
              GetLastError());
	    
        CloseHandle(hMapFile1);

       return 1;
    }
	// -------------------------------------------


	// CREATING SHARED MEMORY 2
	HANDLE hMapFile2;
	hive_stats* pBuf2;

    hMapFile2 = CreateFileMapping(
                  INVALID_HANDLE_VALUE,    // use paging file
                  NULL,                    // default security
                  PAGE_READWRITE,          // read/write access
                  0,                       // maximum object size (high-order DWORD)
                  sizeof(hive_stats),                // maximum object size (low-order DWORD)
                  szName2);                 // name of mapping object

    if (hMapFile2 == NULL)
    {
       _tprintf(TEXT("Could not create file mapping object (%d).\n"),
              GetLastError());
       return 1;
    }
    pBuf2 = (hive_stats*) MapViewOfFile(hMapFile2,   // handle to map object
                         FILE_MAP_ALL_ACCESS, // read/write permission
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

	//--------------------------------------------

	// Creating our structure 1
	fbed* flowerbed = (fbed*)calloc(P1, sizeof(fbed));
	srand(time(NULL));
	for( int i=0; i<P1; ++i )
	{
		flowerbed[i].distance = rand()%241+10;
		flowerbed[i].number_of_flowers = rand()%100 + 1;
		flowerbed[i].isEmpty = FALSE;
		for( int j=0; j<flowerbed[i].number_of_flowers; ++j )
			flowerbed[i].flower_nectar[j] = rand()%21+5;
	}


    CopyMemory(pBuf1, flowerbed, sizeof(fbed) * P1);


	// Creating our structure 2
	hive_stats our_hive;
	our_hive.bees_inside=0;
	our_hive.bees_outside=0;
	our_hive.capacity_of_hive = P2;
	our_hive.number_of_honey = 0;
	our_hive.number_of_flowerbeds = P1;
	our_hive.still_running = TRUE;
	our_hive.current_number_of_bees = 0;
	our_hive.total_bees_created = 0;
	our_hive.dead_bees = 0;
	CopyMemory(pBuf2, &our_hive, sizeof(our_hive) );

	// -----------------------------------
	
     

	

	// Creating IN/OUT MUTEXES

	HANDLE hMutex_in;
	HANDLE hMutex_out;

	hMutex_in = CreateMutex(NULL,FALSE,TEXT("in"));
	if( !hMutex_in ) return -5;
	hMutex_out = CreateMutex(NULL,FALSE,TEXT("out"));
	if( !hMutex_out ) return -6;
	//---------------------

	// Creating mutex for incrementing and decrementing bees inside and outside

	HANDLE hMutex_bees;

	hMutex_bees = CreateMutex(NULL,FALSE,TEXT("bees"));
	if( !hMutex_bees ) return -7;
	// --------------------------------------


	// Creating Semaphores for flowerbeds
	HANDLE* ghSemaphore = (HANDLE*)calloc(P1, sizeof(HANDLE));
	
	for( int i=0; i<P1; ++i )
	{
		char buf[10];
		sprintf(buf,"%d",i);
		//printf("%s\n",buf);
		ghSemaphore[i] = CreateSemaphore( 
        NULL,           // default security attributes
		flowerbed[i].number_of_flowers,  // initial count
        flowerbed[i].number_of_flowers,  // maximum count
		(LPCWSTR)buf);          // unnamed semaphore
		if (ghSemaphore[i] == NULL) 
		{
			printf("CreateSemaphore error: %d\n", GetLastError());
			return 1;
		}
	}
	
	//-----------------------------------

	// Creating Mutexes for flowers
	
	HANDLE** hMutex = (HANDLE**)calloc(P1, sizeof(HANDLE*));
	for( int i=0; i<P1; ++i )
	{
		hMutex[i] = (HANDLE*)calloc(flowerbed[i].number_of_flowers, sizeof(HANDLE));
		for( int j=0; j<flowerbed[i].number_of_flowers; ++j )
		{
			char buf[10];
			sprintf(buf,"%d%d",i,j);
			//printf("%s\n",buf);
			hMutex[i][j] = CreateMutex( NULL, FALSE, (LPCWSTR)buf);
		}
	}
	
	//-------------------------------------


	// HIVE STATISTICS BEFORE A PROGRAM
	printf("STATS BEFORE\n");
	int all_nectar = 0;
	for( int i=0; i<P1; ++i )
	{
		for( int j=0; j<pBuf1[i].number_of_flowers; ++j )
			all_nectar += pBuf1[i].flower_nectar[j];
		printf("Rabata numer: %.2d z iloscia kwiatow: %.2d i liczbie nektaru: %.2d\n", i+1, pBuf1[i].number_of_flowers, all_nectar);
		all_nectar = 0;
	}
	//---------------------------------





	//--------------------------------
	// Creating QUEEN Process
	STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );


	//_getch();
    // Start the child process. 
	
    if( !CreateProcess( TEXT("C:\\Users\\kamil\\Desktop\\ró¿ne\\systemy_operacyjne\\queen\\Debug\\queen.exe"),   // No module name (use command line)
        NULL,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
	{
		printf( "CreateProcess failed (%d).\n", GetLastError() );
		return -1;
	}
	//--------------------------

	Sleep(P3);
	pBuf2->still_running = FALSE;

	WaitForSingleObject( pi.hProcess, INFINITE );
	TerminateThread( pi.hThread, 0 );
	TerminateProcess( pi.hProcess, 0);
	CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );


	// PRINT STATISTICS

	
	// HIVE STATISTICS AFTER A PROGRAM
	printf("\nSTATS AFTER:\n");
	all_nectar = 0;
	for( int i=0; i<P1; ++i )
	{
		for( int j=0; j<pBuf1[i].number_of_flowers; ++j )
			all_nectar += pBuf1[i].flower_nectar[j];
		printf("Rabata numer: %.2d z iloscia kwiatow: %.2d i liczbie nektaru: %.2d\n", i+1, pBuf1[i].number_of_flowers, all_nectar);
		all_nectar = 0;
	}

	printf("Liczba pszczolek wewnatrz: %d\n", pBuf2->bees_inside );
	printf("Liczba pszczolek na zewnatrz: %d\n", pBuf2->bees_outside );
	printf("Liczba miodku: %d\n", pBuf2->number_of_honey );
	printf("Liczba wszystkich stworzonych pszczolek %d\n", pBuf2->total_bees_created );
	printf("Liczba zgonow wsrod pszczolek %d\n", pBuf2->dead_bees ); 
	//---------------------------------
	

	

	


	//--------------------------------------------
	// Closing shared memory 1
    UnmapViewOfFile(pBuf1);
    CloseHandle(hMapFile1);
	//-------------------

	// Closing shared memory 2
	UnmapViewOfFile(pBuf2);
    CloseHandle(hMapFile2);
	// ---------------------

	// Closing IN/OUT Mutexes

	CloseHandle(hMutex_in);
	CloseHandle(hMutex_out);

	//---------------------

	// Close bees mutex

	CloseHandle(hMutex_bees);

	//------------


	// Closing flowerbed semaphores
	for( int i=0; i<P1; ++i )
		CloseHandle(ghSemaphore[i]);
	//-----------------------

	// Closing flowers mutexes
	for( int i=0; i<P1; ++i )
		for( int j=0; j<flowerbed[i].number_of_flowers; ++j )
			CloseHandle(hMutex[i][j]);

	//-----------------------




	system("pause");
	return 0;
}

