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

bool FindSampleSource(const unique_ptr<TW_IDENTITY>& test){
	auto res = strstr(test->ProductName, "TWAIN2 FreeImage Software Scanner");
	return res != nullptr;
}

int _tmain(int argc, _TCHAR* argv [])
{
	MyTwainSession twain;
	if (twain.Initialize()){
		twain.OpenDsm();
		if (twain.IsDsmOpen()){
			auto sources = twain.GetSources();
			auto results = find_if(begin(sources), end(sources),
				//[&](std::shared_ptr<TW_IDENTITY> const& object) { return object->ProductName == "TWAIN2 FreeImage Software Scanner"; });
				FindSampleSource);
			auto hit = results != end(sources) ? results->get() : nullptr;

			if (hit){
				if (twain.OpenSource(*hit) == TWRC_SUCCESS){
					if (twain.EnableSource(EnableSourceMode::kShowUI, true) == TWRC_SUCCESS){

					}
					else{
						cout << "failed to enable source" << endl;
					}
				}
				else{
					cout << "failed to open source" << endl;
				}
			}
			else{
				cout << "Didn't find sample source" << endl;
			}
		}
		else{
			cout << "failed to open dsm" << endl;
		}
	}
	else{
		cout << "failed to initialize" << endl;
	}

	int dummy;
	cin >> dummy;
	return 0;
}

