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
//        Leap::Hand hand = latestFrame.hands()[0];
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
        
            default:
                break;
        }
    }
    
    // one-finger pointing
    if (currentWin && latestFrame.pointables().count() == 1) {
        Leap::Vector hitPoint = pointableScreenPos(latestFrame.pointables()[0], screens);
        driver->setWindowCenter(currentWin, hitPoint);
        return;
    }
    
    // scale currently selected window with two-finger pointing
    if (currentWin && latestFrame.pointables().count() == 2) {
//        cout << "distance: " << pointDistance << endl;
        
        // get pointables from this frame and last frame, find delta
        Leap::Frame lastFrame = controller.frame(1);
        if (lastFrame.isValid()) {
            Leap::Pointable latestPointer1 = latestFrame.pointables()[0];
            Leap::Pointable latestPointer2 = latestFrame.pointables()[1];
            
            Leap::Pointable lastPointer1 = lastFrame.finger(latestPointer1.id());
            Leap::Pointable lastPointer2 = lastFrame.finger(latestPointer2.id());
            if (lastPointer1.isValid() && lastPointer2.isValid()) {
                Leap::Vector latestHitPoint1 = pointableScreenPos(latestFrame.pointables()[0], screens);
                Leap::Vector latestHitPoint2 = pointableScreenPos(latestFrame.pointables()[1], screens);
                Leap::Vector lastHitPoint1 = pointableScreenPos(lastFrame.pointables()[0], screens);
                Leap::Vector lastHitPoint2 = pointableScreenPos(lastFrame.pointables()[1], screens);
                if (latestHitPoint1.isValid() && latestHitPoint2.isValid() && lastHitPoint1.isValid() && lastHitPoint2.isValid()) {
                    
                    // use primary hand as scale, secondary hand as center anchor point
                    double x, y, dx, dy;
                    if (latestPointer1.tipPosition().x > latestPointer2.tipPosition().x) {
                        x = latestHitPoint1.x; y = latestHitPoint1.y;
                        // delta in distance between two fingers
//                        float pointDistance = latestFrame.pointables()[0].tipPosition().distanceTo(latestFrame.pointables()[1].tipPosition());
//                        dx = latestHitPoint1.x - lastHitPoint2.x;
//                        dy = latestHitPoint1.y - lastHitPoint2.y;

//                        dx = hitPoint2.x - lastHitPoint2.x;
//                        dy = hitPoint2.y - lastHitPoint2.y;
                        
                        dx = abs(latestPointer2.tipPosition().x) - abs(latestPointer1.tipPosition().x);
                        dy = abs(latestPointer2.tipPosition().y) - abs(latestPointer1.tipPosition().y);
                    } else {
                        x = latestHitPoint2.x; y = latestHitPoint2.y;
                        dx = latestHitPoint1.x - lastHitPoint1.x;
                        dy = latestHitPoint1.y - lastHitPoint1.y;
                    }
                    
                    // get dx/dy
                    cout << "dx: " << dx << ", dy: " << dy << endl;
                    
                    // average x/y
                    Leap::Vector centerPoint(x, y, 0);
                    
                    // scale window
                    driver->setWindowCenter(currentWin, centerPoint);
                    driver->scaleWindow(currentWin, dx * 2, dy * 2);
                }
            }
        }
    }
}
    
}