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
    if (! AXAPIEnabled()) {
        // try elevating privs to request AX access
        // this is broken because we can't launch with AX setgid privs and load the libLeap shared lib...
        // http://lists.apple.com/archives/accessibility-dev/2009/Oct/msg00014.html
        //if (! enableAXTrust()) return;
        needAXAccess();
        exit(1);
    } else {
        cout << "AXAPI is enabled.\n";
    }
    
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
    map<int,int> checkedProcWindows;
    
    for (int i = 0; i < CFArrayGetCount(windowList); i++) {
        // check each application's windows in turn. windows appear to be ordered by "depth" already
        
        CFDictionaryRef info = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
        
        // get pid
        int pid;
        CFNumberRef pidRef = (CFNumberRef)CFDictionaryGetValue(info, kCGWindowOwnerPID);
        CFNumberGetValue(pidRef, kCFNumberIntType, &pid);
        if (! pid) continue;
        
        // which window did we last check for this process? check the next one in the stack
        int procWinIdx;
        if (checkedProcWindows.find(pid) != checkedProcWindows.end())
            procWinIdx = checkedProcWindows[pid] + 1;
        else
            procWinIdx = 0;
        checkedProcWindows[pid] = procWinIdx;
        
        
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
        ret = findWindowForPID(pid, procWinIdx, x, y);
        if (ret) break;
    }

    return ret;
}
    
const flingerWinRef MacDriver::findWindowForPID(int pid, int winIdx, double x, double y) {
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
    
    // check next window in the stack, which we should find at winIdx
    long winCount = CFArrayGetCount(appWindowsRef);
    if (winIdx >= winCount) {
        CFRelease(appWindowsRef);
        return ret;
    }
    
    AXUIElementRef winRef = (AXUIElementRef)CFArrayGetValueAtIndex(appWindowsRef, winIdx);
    CFRetain(winRef);
    CFRelease(appWindowsRef);
    
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
        CFRelease(winRef);
        return ret;
    }
    if (! AXValueGetValue((AXValueRef)encodedPosition, kAXValueCGPointType, &position)) {
        cerr << "Failed to get encodedPosition value\n";
        CFRelease(winRef);
        return ret;    }
    
    cout << winIdx << ": (" << position.x << ", " << position.y << ", " << size.width << ", " << size.height << ")" << endl;
    
    CFRelease(encodedSize);
    CFRelease(encodedPosition);
    
    CGPoint testPoint = CGPointMake(x, y);
    CGRect winRect = CGRectMake(position.x, position.y, size.width, size.height);
    if (CGRectContainsPoint(winRect, testPoint)) {
        cout << "found window: " << (hasTitle ? titleTmp : "(no title)") << endl;
        ret = (void *)CFRetain(winRef);
    }
    
    CFRelease(winRef);
    return ret;
}

void MacDriver::releaseWinRef(flingerWinRef win) {
    CFRelease(win);
}

const Leap::Vector MacDriver::getWindowPosition(const flingerWinRef win) {
    CGPoint loc = _getWindowPosition(win);
    return Leap::Vector(loc.x, loc.y, 0);
}
    
Leap::Vector MacDriver::getWindowSize(const flingerWinRef win) {
    CGSize size = _getWindowSize(win);
    return Leap::Vector(size.width, size.height, 0);
}
    
void MacDriver::scaleWindow(const flingerWinRef win, double dx, double dy) {
    CGSize size = _getWindowSize(win);
    CGPoint loc = _getWindowPosition(win);
    CGRect bounds = CGRectMake(loc.x, loc.y, size.width, size.height);
    cout << "got rect " << loc.x << ", " << loc.y << endl;

    CGRect scaled = CGRectInset(bounds, dx, dy);
    if (CGRectIsNull(scaled)) {
        cerr << "Failed to inset rect by " << dx << ", " << dy << endl;
        return;
    }
    
    Leap::Vector posVector(scaled.origin.x, scaled.origin.y, 0);
    Leap::Vector sizeVector(scaled.size.width, scaled.size.height, 0);
    cout << "setting rect " << posVector.x << ", " << posVector.y << endl;
    setWindowPosition(win, posVector);
    setWindowSize(win, sizeVector);
}
    
CGSize MacDriver::_getWindowSize(const flingerWinRef win) {
    CFTypeRef encodedSize;
    AXUIElementCopyAttributeValue((AXUIElementRef)win, kAXSizeAttribute, (CFTypeRef *)&encodedSize);
    CGSize size;
    if (! AXValueGetValue((AXValueRef)encodedSize, kAXValueCGSizeType, (void *)&size)) {
        cerr << "Failed to get encodedSize value\n";
        return CGSize();
    }
    return size;
}
    
CGPoint MacDriver::_getWindowPosition(const flingerWinRef win) {
    CFTypeRef encodedPoint;
    AXUIElementCopyAttributeValue((AXUIElementRef)win, kAXPositionAttribute, (CFTypeRef *)&encodedPoint);
    CGPoint point;
    if (! AXValueGetValue((AXValueRef)encodedPoint, kAXValueCGPointType, (void *)&point)) {
        cerr << "Failed to get encodedPoint value\n";
        return CGPoint();
    }
    cout << "got point " << point.x << ", " << point.y << endl;
    return point;
}

void MacDriver::setWindowCenter(const flingerWinRef win, Leap::Vector &pos) {
    assert(pos.isValid());
    
    // get window size
    CGSize size = _getWindowSize(win);
    
    CGPoint posPoint = CGPointMake(pos.x, pos.y);
    // use pos to set center point
    posPoint.x -= size.width / 2;
    posPoint.y -= size.height / 2;
        
    // move window
    CFTypeRef posRef = (CFTypeRef)AXValueCreate(kAXValueCGPointType, &posPoint);
    AXUIElementSetAttributeValue((AXUIElementRef)win, kAXPositionAttribute, (CFTypeRef *)posRef);
    
    CFRelease(posRef);
}

void MacDriver::setWindowPosition(const flingerWinRef win, Leap::Vector &pos) {
    assert(pos.isValid());
    
    // move window
    CGPoint posPoint = CGPointMake(pos.x, pos.y);
    CFTypeRef posRef = (CFTypeRef)AXValueCreate(kAXValueCGPointType, &posPoint);
    AXUIElementSetAttributeValue((AXUIElementRef)win, kAXPositionAttribute, (CFTypeRef *)posRef);
    
    CFRelease(posRef);
}

void MacDriver::setWindowSize(const flingerWinRef win, Leap::Vector &size) {
    assert(size.isValid());
    
    // get window size
    CFTypeRef encodedSize;
    AXUIElementCopyAttributeValue((AXUIElementRef)win, kAXSizeAttribute, (CFTypeRef *)&encodedSize);
    CGSize _size = CGSizeMake(size.x, size.y);
    
    // move window
    CFTypeRef sizeRef = (CFTypeRef)AXValueCreate(kAXValueCGSizeType, &_size);
    AXUIElementSetAttributeValue((AXUIElementRef)win, kAXSizeAttribute, (CFTypeRef *)sizeRef);
    
    CFRelease(sizeRef);
}

    
// routines 

void MacDriver::needAXAccess() {
    cerr << "You must enable access for assistive devices in the Universal Access preference pane to use this application.\n";
    
    // convert to CF
    
    //    int result = NSRunAlertPanel(@"Enable Access for Assistive Devices." , @"To continue, please enable access for assistive devices in the Universal Access pane in System Preferences. Then, relaunch the application." , @"Open System Preferences", @"Quit", nil);
    //
    //	if(result == NSAlertDefaultReturn)
    //	{
    //		[[NSWorkspace sharedWorkspace]openFile:@"/System/Library/PreferencePanes/UniversalAccessPref.prefPane"];
    //	}
    //
    //	//nothing else we can do right now
    //	[NSApp terminate:nil];
}

bool MacDriver::enableAXTrust() {
    //authentication based on file:///Developer/Documentation/DocSets/com.apple.ADC_Reference_Library.CoreReference.docset/Contents/Resources/Documents/documentation/Security/Conceptual/authorization_concepts/03authtasks/chapter_3_section_4.html
    // auth helper code from http://caffeinatedcocoa.com/blog/?p=12
    
    AuthorizationFlags myFlags = kAuthorizationFlagDefaults;
    AuthorizationRef myAuthorizationRef;
    
    OSStatus authCreateStatus = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, myFlags, &myAuthorizationRef);
    
    if (authCreateStatus != errAuthorizationSuccess) {
        needAXAccess();
        return false;
    }
    
    AuthorizationItem myItems = {kAuthorizationRightExecute, 0, NULL, 0};
    AuthorizationRights myRights = {1, &myItems};
    
    myFlags = kAuthorizationFlagDefaults |
    kAuthorizationFlagInteractionAllowed |
    kAuthorizationFlagPreAuthorize |
    kAuthorizationFlagExtendRights;
    OSStatus authRightsStatus = AuthorizationCopyRights(myAuthorizationRef, &myRights, NULL, myFlags, NULL );
    
    if (authRightsStatus != errAuthorizationSuccess) {
        AuthorizationFree(myAuthorizationRef, kAuthorizationFlagDefaults);
        cout << "Failed AuthorizationCopyRights\n";
        needAXAccess();
        return false;
    }
    
    //we pass the path to our bundle to the agent so it can relaunch us when it makes us trusted
    CFBundleRef mainBundle;
    mainBundle = CFBundleGetMainBundle();
    CFURLRef mainBundleURLRef = CFBundleCopyExecutableURL(mainBundle);
    char exePath[1024];
    if (! CFURLGetFileSystemRepresentation(mainBundleURLRef, true, (UInt8 *)exePath, sizeof(exePath))) {
        CFRelease(mainBundleURLRef);
        cerr << "Failed CFURLGetFileSystemRepresentation(mainBundleURLRef)\n";
        return false;
    }
    CFRelease(mainBundleURLRef);
    
    char *myArguments[] = { exePath, NULL };
    
    CFURLRef agentURLRef = CFBundleCopyAuxiliaryExecutableURL(mainBundle, CFSTR("MakeProcessTrustedAgent.app/Contents/MacOS/MakeProcessTrustedAgent"));
    char makeProcessTrustedAgentPath[1024];
    if (! CFURLGetFileSystemRepresentation(agentURLRef, true, (UInt8 *)makeProcessTrustedAgentPath, sizeof(makeProcessTrustedAgentPath))) {
        CFRelease(agentURLRef);
        cerr << "Failed CFURLGetFileSystemRepresentation(agentURLRef)\n";
        return false;
    }
    CFRelease(agentURLRef);
    
    myFlags = kAuthorizationFlagDefaults;
    //    cout << makeProcessTrustedAgentPath << endl;
    OSStatus launchStatus = AuthorizationExecuteWithPrivileges(myAuthorizationRef, makeProcessTrustedAgentPath, myFlags, myArguments, NULL);
    
    AuthorizationFree(myAuthorizationRef, kAuthorizationFlagDefaults);
    
    if (launchStatus != errAuthorizationSuccess) {
        cout << "Failed AuthorizationExecuteWithPrivileges: " << launchStatus << endl;
        needAXAccess();
        return false;
    }
    
    //due to a bug with AXMakeProcessTrusted(), we need to be relaunched before we will actually have access to UI Scripting
    exit(0);
}

}
