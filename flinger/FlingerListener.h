//
//  FlingerListener.h
//  flinger
//
//  Created by Mischa Spiegelmock on 3/10/13.
//
//

#ifndef __flinger__FlingerListener__
#define __flinger__FlingerListener__

//#include <iostream.h>
#include <Leap.h>
#include "FlingerDriver.h"

#ifdef FLINGER_MAC
    #include "FlingerMac.h"
#endif


namespace flinger {
    
static const unsigned int handGestureFrameInterval = 2;

class Listener : public Leap::Listener {
public:
    Listener();
    ~Listener();
    
    virtual void setCurrentWin(flingerWinRef win);
    
    virtual void onInit(const Leap::Controller &);
    virtual void onFrame(const Leap::Controller&);
    
    virtual void onWindowMovedBy(const Leap::Controller&, const flingerWinRef &win, double dx, double dy) = 0;
    
protected:
    Driver *driver;
    flingerWinRef currentWin = NULL;
    Leap::Vector currentWinOrigPosition;
};

}

#endif /* defined(__flinger__FlingerListener__) */
