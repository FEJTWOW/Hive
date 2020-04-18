// worker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>
#pragma comment(lib, "user32.lib")

TCHAR szName1[]=TEXT("Local\\MyFileMappingObject1");
TCHAR szName2[]=TEXT("Local\\MyFileMappingObject2");


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





int main(int argc, char* argv[])
{
	srand(time(NULL)+GetCurrentProcessId());
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


	// READING FROM SHARED MEMORY 1

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
	//--------------------------------------------------------------

	// Our bee is starting work

	int number_of_honey = 5;
	int capacity = 0;
	int distance_to_fly = 0;
	int time_fly = 0;

	HANDLE hMutex_out;
	HANDLE hMutex_in;
	HANDLE hMutex_bees;

	// OPENING SEMAPHORES FOR FLOWERBEDS
	HANDLE* ghSemaphore = (HANDLE*)calloc(pBuf2->number_of_flowerbeds, sizeof(HANDLE)); 
	for( int i=0; i<pBuf2->number_of_flowerbeds; ++i )
	{
		char buf[10];
		sprintf(buf,"%d",i);
		ghSemaphore[i] = OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE, (LPCWSTR)buf); 
	}
	//--------------------------------------------------

	// OPENING MUTEXES FOR FLOWERS

	HANDLE** hMutex = (HANDLE**)calloc(pBuf2->number_of_flowerbeds, sizeof(HANDLE*));
	for( int i=0; i<pBuf2->number_of_flowerbeds; ++i )
	{
		hMutex[i] = (HANDLE*)calloc(pBuf1[i].number_of_flowers, sizeof(HANDLE));
		for( int j=0; j<pBuf1[i].number_of_flowers; ++j )
		{
			char buf[10];
			sprintf(buf,"%d%d",i,j);
			hMutex[i][j] = OpenMutex( MUTEX_ALL_ACCESS, FALSE, (LPCWSTR)buf);
		}
	}

	//-------------------------

	// OPENING IN/OUT MUTEXES
	hMutex_out = OpenMutex( 
		MUTEX_ALL_ACCESS,            // request full access
	    FALSE,                       // handle not inheritable
        TEXT("out"));  // object name

	hMutex_in = OpenMutex( 
		MUTEX_ALL_ACCESS,            // request full access
	    FALSE,                       // handle not inheritable
        TEXT("in"));  // object name
	// ---------------------------

	// OPENING BEES MUTEX
	hMutex_bees = OpenMutex(MUTEX_ALL_ACCESS,FALSE,TEXT("bees"));
	//----------------

	while( true )
	{
		BOOL boutContinue = TRUE;
		DWORD dwoutWaitResult; 
		int choosen_flowerbed;
		while(boutContinue) // bee is trying to leave hive
		{
			dwoutWaitResult = WaitForSingleObject( 
				hMutex_out,   // handle to mutex_out
				0L);           // zero-second time-out interval
			switch (dwoutWaitResult)
			{
				case WAIT_OBJECT_0: 
					{
						boutContinue = FALSE;
						Sleep(2);
						
						choosen_flowerbed = rand()%(pBuf2->number_of_flowerbeds); // choosing flowerbed while waiting
						if( pBuf1[choosen_flowerbed].isEmpty ) choosen_flowerbed = (choosen_flowerbed+1)%(pBuf2->number_of_flowerbeds); // if our flowerbed is empty we are going to choose next one
						distance_to_fly = pBuf1[choosen_flowerbed].distance;
						time_fly = pBuf1[choosen_flowerbed].distance / 5;

						BOOL bbeesoutContinue = TRUE;
						DWORD dwbeesoutWaitResult;
						while( bbeesoutContinue )
						{
							dwbeesoutWaitResult = WaitForSingleObject( hMutex_bees, 0L );
							switch(dwbeesoutWaitResult)
							{
								case WAIT_OBJECT_0:
									{
										bbeesoutContinue = FALSE;
										pBuf2->bees_inside -= 1;
										pBuf2->bees_outside += 1;
										ReleaseMutex(hMutex_bees);
										break;
									}
								case WAIT_ABANDONED: break;
							}
						}
						ReleaseMutex(hMutex_out);
						break;
					}
				case WAIT_ABANDONED: break;
			}
		}
		
		// Bee is flying to the flowerbed
		number_of_honey -= distance_to_fly/100 + 1;
		Sleep(time_fly);
		//------------------------------

		char buf[10];
		sprintf( buf,"%d",choosen_flowerbed );

		DWORD dwbedWaitResult; 
		BOOL bbedContinue=TRUE;

		while(bbedContinue)
		{
			dwbedWaitResult = WaitForSingleObject( 
				ghSemaphore[choosen_flowerbed],   // handle to semaphore
			    0L);           // zero-second time-out interval
			switch(dwbedWaitResult) 
			{
				case WAIT_OBJECT_0:
					{
						int j = 0;
						bbedContinue = FALSE;

						BOOL bflowerContinue = TRUE;
						DWORD dwflowerWaitResult;
						int empty_flowers = 0;
						while( bflowerContinue )
						{
							dwflowerWaitResult = WaitForSingleObject( hMutex[choosen_flowerbed][j], 0L );

							switch( dwflowerWaitResult )
							{
								case WAIT_OBJECT_0:
									{
										
										capacity += pBuf1[choosen_flowerbed].flower_nectar[j];
										
										// tutaj mozesz dodac funkcjonalnosc ktora sprawdza czy to jest puste
										if( capacity >= 100 ) 
										{
											pBuf1[choosen_flowerbed].flower_nectar[j] = capacity - 100;
											bflowerContinue = FALSE;
											Sleep(20);
											ReleaseMutex( hMutex[choosen_flowerbed][j] );
											break;
										}
										else 
										{
											pBuf1[choosen_flowerbed].flower_nectar[j] = 0;
											empty_flowers++;
											if( empty_flowers == pBuf1[choosen_flowerbed].number_of_flowers )
											{
												bflowerContinue = FALSE;
												pBuf1[choosen_flowerbed].isEmpty == TRUE;
											}
											ReleaseMutex( hMutex[choosen_flowerbed][j] );
											j = (j+1)%pBuf1[choosen_flowerbed].number_of_flowers;
											break;
										}
									}
								case WAIT_ABANDONED:
									{
										j = (j+1)%pBuf1[choosen_flowerbed].number_of_flowers;
										break;
									}
							}
						}
						ReleaseSemaphore( 
                        ghSemaphore,  // handle to semaphore
                        1,            // increase count by one
                        NULL);      // not interested in previous count
						break;

					}
				case WAIT_TIMEOUT: break;
			}
		}

		// now bee is coming back

		number_of_honey -= distance_to_fly/100 + 1;
		Sleep(time_fly);

		// she has a lot of flower nectar so there is a chance that she dies
		int random = rand()%100+1; 
		if( random <= 10 ) 
		{
			BOOL bbeesdiedContinue = TRUE;
			DWORD dwbeesdiedWaitResult;
			while( bbeesdiedContinue )
			{
				dwbeesdiedWaitResult = WaitForSingleObject( hMutex_bees, 0L );
				switch(dwbeesdiedWaitResult)
				{
					case WAIT_OBJECT_0:
					{
						bbeesdiedContinue = FALSE;
						pBuf2->bees_outside -= 1;
						pBuf2->current_number_of_bees -= 1;
						pBuf2->dead_bees++;
						ReleaseMutex(hMutex_bees);
						return -1;
						break; // we dont need this break here 
					}
					case WAIT_ABANDONED: break;
				}
			
			}
		}
		// ------------------------------------------------------------
		BOOL binContinue = TRUE;
		DWORD dwinWaitResult;
		//if( !(pBuf2->still_running) ) break;
		while( binContinue )
		{
			dwoutWaitResult = WaitForSingleObject( 
				hMutex_in,   // handle to mutex_out
				0L);           // zero-second time-out interval
			switch( dwoutWaitResult )
			{
				case WAIT_OBJECT_0:
					{
						binContinue = FALSE;
						Sleep(2);
						BOOL bbeesinContinue = TRUE;
						DWORD dwbeesinWaitResult;
						while( bbeesinContinue )
						{
							dwbeesinWaitResult = WaitForSingleObject( hMutex_bees, 0L );
							switch(dwbeesinWaitResult)
							{
								case WAIT_OBJECT_0:
									{
										bbeesinContinue = FALSE;
										pBuf2->bees_inside += 1;
										pBuf2->bees_outside -= 1;
										pBuf2->number_of_honey += capacity;
										capacity =0;
										pBuf2->number_of_honey -= 5 - number_of_honey;
										ReleaseMutex(hMutex_bees);
										break;
									}
								case WAIT_ABANDONED: break;
							}
						}
						ReleaseMutex(hMutex_in);
						break;
					}
				case WAIT_ABANDONED: break;
			}
		}


	}

	// Closing Semaphores

	for( int i=0; i<pBuf2->number_of_flowerbeds; ++i )
		CloseHandle(ghSemaphore[i]);
	free(ghSemaphore);

	//---------------------------
	// Closing Mutexes

	for( int i=0; i<pBuf2->number_of_flowerbeds; ++i )
	{
		for( int j=0; j<pBuf1[i].number_of_flowers; ++j )
			CloseHandle(hMutex[i][j]);
	}
	for( int i=0; i<pBuf2->number_of_flowerbeds; ++i )
		free(hMutex[i]);
	free(hMutex);

	//----------------------------

	// Closing shared memory 1
    UnmapViewOfFile(pBuf1);
    CloseHandle(hMapFile1);
	// -------------------------
	
	// Closing shared memory 2
	UnmapViewOfFile(pBuf2);
    CloseHandle(hMapFile2);
	//--------------------------
	//system("pause");
	return 0;
}

