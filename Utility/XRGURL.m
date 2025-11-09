/* 
 * XRG (X Resource Graph):  A system resource grapher for Mac OS X.
 * Copyright (C) 2002-2022 Gaucho Software, LLC.
 * You can view the complete license in the LICENSE file in the root
 * of the source tree.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

//
//  XRGURL.m
//

#import "XRGURL.h"
#import <os/log.h>

NSString		*userAgent = nil;

@implementation XRGURL {
    NSURLSession *_urlSession;
    NSURLSessionDataTask *_dataTask;
}

- (instancetype) init {
	if (self = [super init]) {
		_urlSession = nil;
		_dataTask = nil;
		_url = nil;
		_urlString = nil;
		_urlData = nil;
		_isLoading = NO;
		_dataReady = NO;
		_errorOccurred = NO;
		_cacheMode = XRGURLCacheIgnore;
	}
    
    return self;
}

- (instancetype) initWithURLString:(NSString *)urlS {
	if (self = [self init]) {
		[self setURLString:urlS];
	}
	
	return self;
}

- (void) dealloc {
	[self setURL:nil];
	[self setData:nil];
	[self setURLString:nil];
	if (_dataTask) {
		[_dataTask cancel];
		_dataTask = nil;
	}
}

#pragma mark Getter/Setters
- (void) setURLString:(NSString *)newString {
	if (newString != _urlString) {
		_urlString = newString;
		
		// Need to reset our URL object now.
		if ([_urlString length] > 0) {
			[self setURL:[NSURL URLWithString:_urlString]];
		}
		else {
			[self setURL:nil];
		}
	}
}

+ (NSString *) userAgent {
	return userAgent;
}

+ (void) setUserAgent:(NSString *)newAgent {
	userAgent = newAgent;
}

- (void) setData:(NSData *)newData {
	NSMutableData *newMutableData = newData ? [NSMutableData dataWithData:newData] : nil;
	
	_urlData = newMutableData;
}

- (void) appendData:(NSData *)appendData {
	if (_urlData != nil) {
		[_urlData appendData:appendData];
	}
	else {
		[self setData:[NSMutableData dataWithLength:0]];
		[_urlData appendData:appendData];
	}
}

- (void) setUserAgentForRequest:(NSMutableURLRequest *)request {
	if ([XRGURL userAgent] != nil) {
		[request setValue:[XRGURL userAgent] forHTTPHeaderField:@"User-Agent"];
	}
}

#pragma mark Action Methods
- (void) loadURLInForeground {
	if (![self prepareForURLLoad]) {
		self.errorOccurred = YES;
		return;
	}
	
#ifdef XRG_DEBUG
    NSLog(@"[XRGURL loadURLInForeground] Loading URL: %@", _urlString);
#endif
    
    NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
    // Map cacheMode to requestCachePolicy
    NSURLRequestCachePolicy policy = NSURLRequestReloadIgnoringCacheData;
    switch (self.cacheMode) {
        case XRGURLCacheIgnore: policy = NSURLRequestReloadIgnoringCacheData; break;
        case XRGURLCacheUse: policy = NSURLRequestReturnCacheDataElseLoad; break;
        case XRGURLCacheOnly: policy = NSURLRequestReturnCacheDataDontLoad; break;
    }
    config.requestCachePolicy = policy;
    config.timeoutIntervalForRequest = 60;
    
    NSURLSession *session = [NSURLSession sessionWithConfiguration:config];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:self.url cachePolicy:policy timeoutInterval:60];
    [self setUserAgentForRequest:request];
    
    __block NSData *responseData = nil;
    __block NSError *taskError = nil;
    dispatch_semaphore_t sema = dispatch_semaphore_create(1);
    dispatch_semaphore_wait(sema, DISPATCH_TIME_NOW); // take the semaphore
    NSURLSessionDataTask *task = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (data) {
            responseData = data;
        }
        taskError = error;
        dispatch_semaphore_signal(sema);
    }];
    [task resume];
    
    // Ensure we are not blocking the main/UI thread; this method should be called off-main.
    dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
    [session finishTasksAndInvalidate];
    [self setData:responseData];
    if (_urlData == nil || taskError) self.errorOccurred = YES;
    
#ifdef XRG_DEBUG
    NSLog(@"[XRGURL loadURLInForeground] Finished loading URL: %@", _urlString);
#endif
    self.dataReady = YES;
}

- (void) loadURLInBackground {
	if (![self prepareForURLLoad]) {
		self.errorOccurred = YES;
		return;
	}
	
#ifdef XRG_DEBUG
    NSLog(@"[XRGURL] Started Background Loading %@", _urlString);
#endif
    self.isLoading = YES;
    
    NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSURLRequestCachePolicy policy = NSURLRequestReloadIgnoringCacheData;
    switch (self.cacheMode) {
        case XRGURLCacheIgnore: policy = NSURLRequestReloadIgnoringCacheData; break;
        case XRGURLCacheUse: policy = NSURLRequestReturnCacheDataElseLoad; break;
        case XRGURLCacheOnly: policy = NSURLRequestReturnCacheDataDontLoad; break;
    }
    config.requestCachePolicy = policy;
    config.timeoutIntervalForRequest = 60;
    
    NSOperationQueue *delegateQueue = [[NSOperationQueue alloc] init];
    delegateQueue.qualityOfService = NSQualityOfServiceUtility; // lower than user-interactive to avoid inversions
    NSURLSession *session = [NSURLSession sessionWithConfiguration:config delegate:nil delegateQueue:delegateQueue];
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:self.url cachePolicy:policy timeoutInterval:60];
    [self setUserAgentForRequest:request];
    
    __weak typeof(self) weakSelf = self;
    _dataTask = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        __strong typeof(self) strongSelf = weakSelf;
        if (!strongSelf) { return; }
        if (error) {
            strongSelf.errorOccurred = YES;
        } else if (data) {
            [strongSelf setData:data];
            strongSelf.dataReady = YES;
        }
        strongSelf.isLoading = NO;
        
#ifdef XRG_DEBUG
        if (error) {
            NSLog(@"[XRGURL] Failed Loading %@: %@", strongSelf->_urlString, error.localizedDescription);
        } else {
            NSLog(@"[XRGURL] Finished Loading %@", strongSelf->_urlString);
        }
#endif
        [session finishTasksAndInvalidate];
    }];
    [_dataTask resume];
}

// Returns whether or not the URL is ready to load.
- (BOOL) prepareForURLLoad {
	// Check to make sure we have a valid NSURL object.
	if (self.urlString == nil) {
#ifdef XRG_DEBUG
		NSLog(@"[XRGURL prepareForURLLoad] Error:  Attempted to load URL with empty URL String.");
#endif
		return NO;
	}
	
	if (self.url == nil) {
		[self setURL:[NSURL URLWithString:self.urlString]];
		
		if (self.url == nil) {
#ifdef XRG_DEBUG
			NSLog(@"[XRGURL prepareForURLLoad] Error:  Failed to initialize NSURL with urlString: %@.", _urlString);
#endif
			return NO;
		}
	}
	
	// Clear out the old data.
	self.dataReady = NO;
	self.errorOccurred = NO;
	[self setData:nil];
	
	return YES;
}

- (void) cancelLoading {
	if (_dataTask != nil) {
		[_dataTask cancel];
		_dataTask = nil;
	}
    
	[self setData:nil];
    
	self.errorOccurred = NO;
    self.dataReady = NO;
	self.isLoading = NO;
}

#pragma mark Status Methods
- (BOOL) haveGoodURL {
    return (self.url != nil);
}

#pragma mark Notifications
// Removed NSURLConnection delegate methods as requested

@end
