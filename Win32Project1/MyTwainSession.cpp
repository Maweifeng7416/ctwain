#include "stdafx.h"
#include <iostream>
#include <sstream>
#include "MyTwainSession.h"
#include "FreeImage.h"

using namespace std;
using namespace ctwain;


void MyTwainSession::OnFillAppId(TW_IDENTITY& appId) {
	TwainSession::OnFillAppId(appId);
	// put strings relevant to this exe
}

void MyTwainSession::OnDeviceEvent(const TW_DEVICEEVENT& e) {
	cout << "Received TWAIN device event " << e.Event << endl;
}

void MyTwainSession::OnTransferReady(TransferReadyEventArgs& e) {
	cout << "Received TWAIN transfer ready event " << endl;
}

void MyTwainSession::OnTransferredData(const TransferredDataEventArgs& e) {
	cout << "Received TWAIN data!" << endl;
}

void MyTwainSession::OnSourceDisabled() {
	cout << "TWAIN source disabled" << endl;
}