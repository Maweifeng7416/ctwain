// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MyTwainSession.h"
#include <memory>
#include <iostream>
#include <algorithm>
#include <string>

using namespace std;
using namespace ctwain;

int _tmain(int argc, _TCHAR* argv [])
{
	MyTwainSession twain;
	if (twain.Initialize()){
		cout << "DSM initialized" << endl;
		twain.OpenDsm();
		if (twain.IsDsmOpen()){
			cout << "DSM opened" << endl;


			auto sources = twain.GetSources();
			auto hit = find_if(begin(sources), end(sources),
				[](const TW_IDENTITY& test) { return strstr(test.ProductName, "TWAIN2 FreeImage Software Scanner") != nullptr; });


			if (hit != end(sources)){
				if (twain.OpenSource(*hit) == TWRC_SUCCESS){
					cout << "Opened sample source " << hit->ProductName << endl;
					/*if (twain.EnableSource(EnableSourceMode::kShowUI, false) == TWRC_SUCCESS){
						cout << "Enabled sample source" << endl;

						}
						else{
						cout << "Failed to enable sample source" << endl;
						}*/
				}
				else{
					cout << "Failed to open sample source" << endl;
				}
			}
			else{
				cout << "Didn't find sample source" << endl;
			}
		}
		else{
			cout << "Failed to open DSM" << endl;
		}
	}
	else{
		cout << "Failed to initialize DSM" << endl;
	}

	int dummy;
	cin >> dummy;
	return 0;
}

