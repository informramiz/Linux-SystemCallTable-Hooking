#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <string.h>

#define MAX_LENGTH		12
#define NO_EXTS 		2
#define PASSWORD_LENGTH		6

using namespace std;

static char password[PASSWORD_LENGTH];
static char extensions[NO_EXTS] = { '0' , '0'  };
bool write ( );

int main ( void )
{
	int option = 0;
	strcpy ( password , "12345" );
	
	do
	{
		cout << "\n\n-----Select an option to protect/unprotect------" << endl;
		cout << "1. PDF" << ( extensions[0] != '0' ? "--protected" : "" ) << endl;
		cout << "2. PNG" << ( extensions[1] != '0' ? "--protected" : "" ) << endl;
		cout << "3. EXIT" << endl;
		cout << "Please enter option number : ";
		cin >> option;
		
		if ( option < 3 )
		{
			extensions[option-1] = (extensions[option-1] == '0' ? '1' : '0' );
			bool status = write ( );

			if ( status == false )
				extensions[option-1] = '0';
		}		

	}while ( option != 3 );

	return 0;
}

bool write ( )
{
	string str = password;
	str = str + "-" + extensions;

	ofstream outFile ( "/proc/fortune" );
	
	if ( outFile.is_open ( ) == false )
	{
		cout << "unable to open file" << endl;
		return false;
	}
		
	outFile << str.data( );
	outFile.close ( );

	return true;
}

void read ( )
{ARCH=arm CROSS_COMPILE=/path/to/mydroid/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi- make
	ifstream inFile ( "/proc/fortune" );
	
	if ( inFile.is_open ( ) == false )
	{
		cout << "unable to open file" << endl;
		
	}

	char str[1024];
	inFile >> str ;
	cout << str << endl;
	inFile.close ( );
}
