#import <Cocoa/Cocoa.h>
#import "XRGGenericView.h"
#import "XRGAITokensMiner.h"

@interface XRGAITokensView : XRGGenericView
{
    NSSize graphSize;
    XRGModule *m;
    NSInteger numSamples;
}

- (void)setGraphSize:(NSSize)newSize;
- (void)setWidth:(NSInteger)newWidth;
- (void)updateMinSize;
- (void)graphUpdate:(NSTimer *)aTimer;
- (void)drawRect:(NSRect)rect;

@end
