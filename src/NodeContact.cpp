// **********************************************************************************
// OpenNode library for OpenMiniHub IoT
// **********************************************************************************
// Copyright Martins Ierags (2016), martins.ierags@gmail.com
// http://openminihub.com/
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>
#include "NodeContact.h"
#include "OpenNode.h"
#include "OpenProtocol.h"

NodeContact::NodeContact(unsigned char contactId, ContactData_t contactData, NodeValue valueFunc, unsigned long reportingInterval)
{
  mId = contactId;
  mData = contactData;
  mIsEnqueued = false;
  mReportingInterval = reportingInterval;
  mValueFunc = valueFunc;
  mSignedMsg = true;
  this->intervalReset();
  
  OpenNode *node = OpenNode::node();
  if (node) {
    node->addContact(this);
  }
}

bool NodeContact::sendReport(unsigned char destination, bool useDestinationValue, bool doRefresh)
{
  OpenNode *node = OpenNode::node();
  if (node) {
    if (!useDestinationValue)
      destination = node->getGateway();
    if (doRefresh)
      this->refreshValue();

    OpenProtocol::buildContactValuePacket(this);
    return node->send(destination, this->isSignedMsg());
  } else {
    return false;
  }
}

void NodeContact::refreshValue()
{
  if (mValueFunc) {
    bool success = mValueFunc(mId); //can add id and datatype
  }
}

void NodeContact::intervalTick(unsigned long time)
{

  if (mReportingInterval > 0) {
    if (time >= mNextReport) {
      mNextReport = 0;
    } else {
      mNextReport -= time;
    }
    
    if (mNextReport == 0) {
      this->sendReport();
      this->intervalReset();
    }
  } else if (mIsEnqueued) {
    mIsEnqueued = false;
    this->sendReport();
  }
}
