#pragma once
#include "twain_session.h"
class MyTwainSession : public ctwain::TwainSession
{
public:
	virtual void OnFillAppId(TW_IDENTITY& appId) override;

	virtual void OnDeviceEvent(const TW_DEVICEEVENT& deviceEvent) override;

	virtual void OnTransferReady(ctwain::TransferReadyEventArgs& readyEvent) override;
	
	virtual void OnTransferredData(const ctwain::TransferredDataEventArgs& transferEvent) override;

	virtual void OnSourceDisabled() override;
};

