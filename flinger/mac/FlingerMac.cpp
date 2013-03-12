//
//  FlingerMac.cpp
//  flinger
//
//  Created by Mischa Spiegelmock on 3/11/13.
//
//

#include "FlingerMac.h"

using namespace std;

namespace flinger {
    
MacDriver::MacDriver() {
    windowList = NULL;
    
    listOptions = kCGWindowListExcludeDesktopElements;
    listOptions |= kCGWindowListOptionOnScreenOnly;
}
    
void MacDriver::updateWindowInfo() {
    windowList = CGWindowListCopyWindowInfo(listOptions, kCGNullWindowID);
}

const flingerWinRef MacDriver::getWindowAt(double x, double y) {
    updateWindowInfo();
    flingerWinRef ret = NULL;
    set<int> checkedPids;
    
    for (int i = 0; i < CFArrayGetCount(windowList); i++) {
        // check each application's windows in turn. windows appear to be ordered by "depth" already
        
        CFDictionaryRef info = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
        
        // get pid
        int pid;
        CFNumberRef pidRef = (CFNumberRef)CFDictionaryGetValue(info, kCGWindowOwnerPID);
        CFNumberGetValue(pidRef, kCFNumberIntType, &pid);
        
        if (checkedPids.find(pid) != checkedPids.end()) {
            // already checked this app
            continue;
        }
        checkedPids.insert(pid);
        
        // layer
        int layer;
        CFNumberRef layerRef = (CFNumberRef)CFDictionaryGetValue(info, kCGWindowLayer);
        CFNumberGetValue(layerRef, kCFNumberIntType, &layer);
        if (layer != 0) continue; // maybe? can't find docs on layer, but 0 seems to be app
        /*
        // window num
        CFNumberRef winNumRef = (CFNumberRef)CFDictionaryGetValue(info, kCGWindowNumber);
        
        // bounds
        CFDictionaryRef boundsDict = (CFDictionaryRef)CFDictionaryGetValue(info, kCGWindowBounds);
        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);

        cout << "Window pid: " << pid << ", layer: " << layer;
        CFStringRef appNameRef = (CFStringRef)CFDictionaryGetValue(info, kCGWindowOwnerName);
        const char *ownerName = NULL;
        if (appNameRef) {
            
            ownerName = CFStringGetCStringPtr(appNameRef, kCFStringEncodingUTF8);
            if (ownerName) {
                cout << ", owner: " << ownerName;
            } else {
                char ownerNameTmp[300];
                if (CFStringGetCString(appNameRef, ownerNameTmp, sizeof(ownerNameTmp), kCFStringEncodingUTF8))
                    cout << ", owner: " << ownerNameTmp;
            }
        }
        cout << endl;
         */
        
        // search for window with a rect that includes the test point
        cout << "Checking point in " << x << "," << y << endl;
        ret = findWindowForPID(pid, x, y);
        if (ret) break;
    }

    return ret;
}
    
const flingerWinRef MacDriver::findWindowForPID(int pid, double x, double y) {
    flingerWinRef ret = NULL;
    
    // get all windows for this application
    AXUIElementRef appRef = AXUIElementCreateApplication(pid);
    if (! appRef) {
        cerr << "Unable to create AXUIElementRef for the application with PID: " << pid << endl;
        return NULL;
    }
    
    AXError err;
    // get app windows
    CFArrayRef appWindowsRef;
    if ((err = AXUIElementCopyAttributeValue(appRef, kAXWindowsAttribute, (CFTypeRef *)&appWindowsRef)) != kAXErrorSuccess) {
        cerr << "Failed to get focused window: " << err << "\n";
        CFRelease(appRef);
        return NULL;
    }
    
    for (int i = 0; i < CFArrayGetCount(appWindowsRef); i++) {
        AXUIElementRef winRef = (AXUIElementRef)CFArrayGetValueAtIndex(appWindowsRef, i);
        
        CFStringRef titleRef;
        char titleTmp[300];
        bool hasTitle = false;
        if (AXUIElementCopyAttributeValue(winRef, kAXTitleAttribute, (CFTypeRef *)&titleRef) == kAXErrorSuccess) {
            if (CFStringGetCString(titleRef, titleTmp, sizeof(titleTmp), kCFStringEncodingUTF8))
                hasTitle = true;
        } else {
            cerr << "failed to get window title\n";
        }
        
        CFTypeRef encodedSize, encodedPosition;
        AXUIElementCopyAttributeValue(winRef, kAXSizeAttribute, (CFTypeRef *)&encodedSize);
        AXUIElementCopyAttributeValue(winRef, kAXPositionAttribute, (CFTypeRef *)&encodedPosition);
        CGSize size; CGPoint position;
        
        if (! AXValueGetValue((AXValueRef)encodedSize, kAXValueCGSizeType, (void *)&size)) {
            cerr << "Failed to get encodedSize value\n";
            goto done;
        }
        if (! AXValueGetValue((AXValueRef)encodedPosition, kAXValueCGPointType, &position)) {
            cerr << "Failed to get encodedPosition value\n";
            goto done;
        }
        
        cout << i << ": (" << position.x << ", " << position.y << ", " << size.width << ", " << size.height << ")" << endl;
        
        CFRelease(encodedSize);
        CFRelease(encodedPosition);
        
        CGPoint testPoint = CGPointMake(x, y);
        CGRect winRect = CGRectMake(position.x, position.y, size.width, size.height);
        if (CGRectContainsPoint(winRect, testPoint)) {
            cout << "found window: " << (hasTitle ? titleTmp : "(no title)") << endl;
            ret = (void *)CFRetain(winRef);
            goto done;
        }
    }
    
done:
    CFRelease(appWindowsRef);
    return ret;
}

void MacDriver::releaseWinRef(flingerWinRef win) {
    CFRelease(win);
}

const Leap::Vector MacDriver::getWindowPosition(const flingerWinRef win) {
    return Leap::Vector();
}
    
void MacDriver::setWindowPosition(const flingerWinRef win, Leap::Vector &pos) {
    // get window size
    CFTypeRef encodedSize;
    AXUIElementCopyAttributeValue((AXUIElementRef)win, kAXSizeAttribute, (CFTypeRef *)&encodedSize);
    CGSize size;
    if (! AXValueGetValue((AXValueRef)encodedSize, kAXValueCGSizeType, (void *)&size)) {
        cerr << "Failed to get encodedSize value\n";
        return;
    }
    
    CGPoint posPoint = CGPointMake(pos.x, pos.y);
    // use pos to set center point
    posPoint.x -= size.width / 2;
    posPoint.y -= size.height / 2;
        
    // move window
    CFTypeRef posRef = (CFTypeRef)AXValueCreate(kAXValueCGPointType, &posPoint);
    AXUIElementSetAttributeValue((AXUIElementRef)win, kAXPositionAttribute, (CFTypeRef *)posRef);
    
    CFRelease(posRef);
}

}
