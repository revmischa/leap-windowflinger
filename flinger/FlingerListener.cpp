//
//  FlingerListener.cpp
//  flinger
//
//  Created by Mischa Spiegelmock on 3/10/13.
//
//

#include "FlingerListener.h"

using namespace std;

namespace flinger {
    
Listener::Listener() {
    currentWin = NULL;
    
#ifdef FLINGER_MAC
    driver = new MacDriver();
#else
    #error "Platform undefined"
#endif
}
    
Listener::~Listener() {
    setCurrentWin(NULL);
    delete(driver);
}

void Listener::setCurrentWin(flingerWinRef win) {
    if (currentWin)
        driver->releaseWinRef(currentWin);
    currentWin = win;
}

    
void Listener::onInit(const Leap::Controller &controller) {
    //    controller.enableGesture(Leap::Gesture::TYPE_KEY_TAP);
    //    controller.enableGesture(Leap::Gesture::TYPE_CIRCLE);
    controller.enableGesture(Leap::Gesture::TYPE_SCREEN_TAP);
    controller.enableGesture(Leap::Gesture::TYPE_SWIPE);
}
    
// where is this pointable pointing (on the screen, not in space)?
static Leap::Vector pointableScreenPos(const Leap::Pointable &pointable, const Leap::ScreenList &screens) {
    // need to have screen info
    if (screens.empty())
        return Leap::Vector();
    
    // get point location
    // get screen associated with gesture
    Leap::Screen screen = screens.closestScreenHit(pointable);
    
    Leap::Vector cursorLoc = screen.intersect(pointable, true);
    if (! cursorLoc.isValid()) {
        cout << "failed to find intersection\n";
        return Leap::Vector();
    }
    
    double screenX = cursorLoc.x * screen.widthPixels();
    double screenY = (1.0 - cursorLoc.y) * screen.heightPixels();
    //                cout << "Screen tapped at " << screenX << "," << screenY << endl;

    return Leap::Vector(screenX, screenY, 0);
}
    
    
void Listener::onFrame(const Leap::Controller &controller) {
    Leap::Frame latestFrame = controller.frame();
    Leap::Frame refFrame = controller.frame(handGestureFrameInterval);
    
    if (refFrame.isValid() && latestFrame.hands().count() == 1) {
        Leap::Hand hand = latestFrame.hands()[0];
//        double scaleFactor = hand.scaleFactor(refFrame);
//        cout << "Scale: " << scaleFactor << endl;
    }
    
    Leap::ScreenList screens = controller.calibratedScreens();
    Leap::GestureList gestures = latestFrame.gestures();

    // process gestures
    const int gestureCount = gestures.count();
    for (int i = 0; i < gestureCount; i++) {
        Leap::Gesture gesture = gestures[i];
        
        switch (gesture.type()) {
            case Leap::Gesture::TYPE_SCREEN_TAP: {
                // select new current window, or deselect current window
                if (currentWin) {
                    setCurrentWin(NULL);
                    cout << "Deselected window\n";
                    break;
                }
                
                // get tap location in screen coords
                if (gesture.pointables().empty()) continue;
                Leap::Vector screenLoc = pointableScreenPos(gesture.pointables()[0], screens);
                if (! screenLoc.isValid())
                    continue;
                
                // find window at tap location
                flingerWinRef win = driver->getWindowAt(screenLoc.x, screenLoc.y);
                if (win == NULL)
                    continue;
                
                // set new current window
                currentWin = win;
                currentWinOrigPosition = driver->getWindowPosition(win);
            } break;
        
            case Leap::Gesture::TYPE_SWIPE: {
                // deal only with one-finger swipes... for now
                if (latestFrame.pointables().count() > 1)
                    continue;
                
                // move window
                
                if (! currentWin)
                    continue;
                
                // get tap location in screen coords
                if (gesture.pointables().empty()) continue;
                Leap::Vector screenLoc = pointableScreenPos(gesture.pointables()[0], screens);
                if (! screenLoc.isValid())
                    continue;
                
                // get gesture dir/magnitude
                Leap::SwipeGesture swipe = Leap::SwipeGesture(gesture);
                Leap::Vector swipeMagnitude = swipe.position() - swipe.startPosition();
                if (swipe.state() == Leap::Gesture::STATE_STOP) {
                    cout << "Swipe mag(x)=" << swipeMagnitude.x << ", mag(y)=" << swipeMagnitude.y << endl;
                    cout << "Move to: " << screenLoc.x << "," << screenLoc.y << endl;
                    driver->setWindowPosition(currentWin, screenLoc);
                    onWindowMovedBy(controller, currentWin, swipeMagnitude.x, swipeMagnitude.y);
                }
            } break;
        
            default:
                break;
        }
    }
    
    // one-finger pointing
    if (currentWin && latestFrame.pointables().count() == 1) {
        Leap::Vector hitPoint = pointableScreenPos(latestFrame.pointables()[0], screens);
        driver->setWindowPosition(currentWin, hitPoint);
        return;
    }
    /*
    // move currently selected window with two-finger pointing
    if (currentWin && latestFrame.pointables().count() == 2) {
        float pointDistance = latestFrame.pointables()[0].tipPosition().distanceTo(latestFrame.pointables()[1].tipPosition());
        cout << "distance: " << pointDistance << endl;
        if (pointDistance < 35) {
            // finger tips close together
            Leap::Vector hitPoint1 = pointableScreenPos(latestFrame.pointables()[0], screens);
            Leap::Vector hitPoint2 = pointableScreenPos(latestFrame.pointables()[1], screens);
            if (hitPoint1.isValid() && hitPoint2.isValid()) {
                // average x/y
                Leap::Vector centerPoint;
                centerPoint.x = (hitPoint1.x + hitPoint2.x) / 2;
                centerPoint.y = (hitPoint1.y + hitPoint2.y) / 2;
                
                // move window
                driver->setWindowPosition(currentWin, centerPoint);
            }
        }
    }*/
}
    
}