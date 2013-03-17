//
//  FlingerMac.h
//  flinger
//
//  Created by Mischa Spiegelmock on 3/11/13.
//
//

#ifndef __flinger__FlingerMac__
#define __flinger__FlingerMac__

#include <iostream>
#include <map>
#include <set>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include "FlingerDriver.h"

namespace flinger {
    
class MacDriver : public Driver {
public:
    MacDriver();
    virtual ~MacDriver() {};
    
    virtual void releaseWinRef(flingerWinRef win);

    virtual const flingerWinRef getWindowAt(double x, double y);
    virtual const Leap::Vector getWindowPosition(const flingerWinRef win);
    virtual void setWindowPosition(const flingerWinRef win, Leap::Vector &pos);
    
protected:
    CGWindowListOption listOptions;
    CFArrayRef windowList;
    
    void updateWindowInfo();
    const flingerWinRef findWindowForPID(int pid, int winIdx, double x, double y);
};
    
}

#endif /* defined(__flinger__FlingerMac__) */
