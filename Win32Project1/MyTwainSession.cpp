#include "stdafx.h"
#include <iostream>
#include <sstream>
#include "MyTwainSession.h"

using namespace std;
using namespace ctwain;


void MyTwainSession::OnFillAppId(TW_IDENTITY& appId) {
	TwainSession::OnFillAppId(appId);
	// put strings relevant to this exe
}

void MyTwainSession::OnDeviceEvent(const TW_DEVICEEVENT& deviceEvent) {
	cout << "Received TWAIN device event " << deviceEvent.Event << endl;
}

void MyTwainSession::OnTransferReady(TransferReadyEventArgs& readyEvent) {
	ostringstream msg;
	cout << "Received TWAIN transfer ready event " << endl;
}