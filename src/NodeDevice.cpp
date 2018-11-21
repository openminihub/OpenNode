// **********************************************************************************
// OpenNode library for OpenMiniHub IoT
// **********************************************************************************
// Copyright Martins Ierags (2017), martins.ierags@gmail.com
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
#include "NodeDevice.h"
#include "OpenNode.h"
#include "OpenProtocol.h"

NodeDevice::NodeDevice(unsigned char deviceId, DeviceData_t deviceData, NodeValue valueFunc, unsigned long reportingInterval)
{
  mId = deviceId;
  mData = deviceData;
  mIsEnqueued = false;
  mReportingInterval = reportingInterval;
  mValueFunc = valueFunc;
  mSignedMsg = true;
  this->intervalReset();
  
  OpenNode *node = OpenNode::node();
  if (node) {
    node->addDevice(this);
  }
}

bool NodeDevice::sendReport(unsigned char destination, bool useDestinationValue, bool doRefresh)
{
  OpenNode *node = OpenNode::node();
  if (node) {
    if (!useDestinationValue)
      destination = node->getGateway();
    if (doRefresh) {
      if (this->refreshValue()) {
        OpenProtocol::buildDeviceValuePacket(this);
        return node->send(destination, this->isSignedMsg());
      }
      else {
        // Serial.println("I'm broking node 1");
        // OpenProtocol::buildDeviceValuePacket(this);
        // return node->send(destination, this->isSignedMsg());
        node->getRadio()->sleep();
        return false;
      }
    }
    else {
      OpenProtocol::buildDeviceValuePacket(this);
      return node->send(destination, this->isSignedMsg());
    }
  } else {
    return false;
  }
}

bool NodeDevice::refreshValue()
{
  bool success = false;
  if (mValueFunc) {
    success = mValueFunc(mId); //can add id and datatype
  }
  return success;
}

void NodeDevice::intervalTick(unsigned long time)
{

  if (mReportingInterval > 0) {
    if (time >= mNextReport) {
      // mNextReport = 0;
      this->sendReport();
      this->intervalReset();
      if (mIsEnqueued) mIsEnqueued = false;
    } else {
      mNextReport -= time;
    }
    // if (mNextReport == 0) {
    //   this->sendReport();
    //   this->intervalReset();
    // }
  } else if (mIsEnqueued) {
    mIsEnqueued = false;
    this->sendReport();
  }
}
