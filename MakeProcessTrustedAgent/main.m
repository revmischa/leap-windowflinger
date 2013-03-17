/*
 * Copyright (c) 2008, Adam Leonard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Leonard ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

int main(int argc, char **argv)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
	[NSApplication sharedApplication];
    
	if (argc != 2) {
		return -1; //not enough arguments
	}

	//the first and only custom argument is the path to the application
	NSString *executablePath = [NSString stringWithUTF8String:argv[1]];

	if (! executablePath) {
        NSLog(@"No executable path");
		return -2;
	}

	int error = AXMakeProcessTrusted((CFStringRef)executablePath);
	
	if(error != kAXErrorSuccess)
	{
		NSLog(@"*** Could not make process trusted! Path: %@ Error:%i ***",executablePath,error);
		
		//fallback: ask the user to enable access for assistive devices manually
		long result = NSRunAlertPanel(@"Enable Access for Assistive Devices." , @"To continue, please enable access for assistive devices in the Universal Access pane in System Preferences. Then, relaunch the application." , @"Open System Preferences", @"Quit", nil);
		
		if(result == NSAlertDefaultReturn)
		{
			[[NSWorkspace sharedWorkspace]openFile:@"/System/Library/PreferencePanes/UniversalAccessPref.prefPane"];
		}
		return -3; //could not make process trusted
	}
	
	[[NSWorkspace sharedWorkspace]launchApplication:executablePath];
	
	[pool release];
	
	return 0;
}

