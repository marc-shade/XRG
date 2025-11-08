#import "XRGAITokensView.h"
#import "XRGGraphWindow.h"
#import "XRGAISettingsKeys.h"

@implementation XRGAITokensView

- (void)awakeFromNib {
    [super awakeFromNib];
    
    self.miner = [self parentWindow].miner;
    self.parentWindow = (XRGGraphWindow *)[self window];
    self.appSettings = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
    self.moduleManager = [self.parentWindow moduleManager];
    
    self.module = [[XRGModule alloc] initWithName:@"AI Tokens"];
    self.module.doesGraphUpdate = YES;
    self.module.displayOrder = 10;
    
    // Set module display from user defaults
    BOOL showAITokensGraph = [[NSUserDefaults standardUserDefaults] boolForKey:XRGDefaultsKeyShowAITokenGraph];
    [self.module setIsDisplayed:showAITokensGraph];
    
    [self.moduleManager addModule:self.module];
    
    [self setGraphSize:self.module.graphSize];
}

- (void)setGraphSize:(XRGGraphSize)size {
    [super setGraphSize:size];
    
    [self setWidth:size == XRGGraphSizeMini ? 140 : 320];
}

- (void)setWidth:(int)width {
    [super setWidth:width];
    
    if (self.miner) {
        [self.miner setResizeDataWidth:width];
    }
}

- (void)updateMinSize {
    [super updateMinSize];
    self.minSize.height = XRG_MINI_HEIGHT;
}

- (void)graphUpdate {
    [self.miner updateAITokens];
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    NSDictionary *settings = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
    
    BOOL showRate = [[settings objectForKey:@"aiTokensShowRate"] boolValue];
    BOOL showBreakdown = [[settings objectForKey:@"aiTokensShowBreakdown"] boolValue];
    
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    
    if (self.module.graphSize == XRGGraphSizeMini) {
        // Mini graph drawing: bar similar to GPU mini
        
        // Background bar
        NSRect barRect = NSMakeRect(10, 4, self.bounds.size.width - 20, self.bounds.size.height - 8);
        NSColor *bgColor = [NSColor colorWithCalibratedWhite:0.15 alpha:1.0];
        [bgColor setFill];
        NSRectFill(barRect);
        
        // Left label "AI"
        NSDictionary *leftAttrs = @{
            NSFontAttributeName: [NSFont boldSystemFontOfSize:11],
            NSForegroundColorAttributeName: [NSColor colorWithCalibratedWhite:0.75 alpha:1.0]
        };
        NSString *leftLabel = @"AI";
        NSSize leftSize = [leftLabel sizeWithAttributes:leftAttrs];
        NSPoint leftPoint = NSMakePoint(12, (self.bounds.size.height - leftSize.height) / 2);
        [leftLabel drawAtPoint:leftPoint withAttributes:leftAttrs];
        
        // Right label tokens/min rounded or "n/a" if disabled
        NSString *rightLabel = @"n/a";
        if (showRate && self.miner.isRunning) {
            double rate = self.miner.aiTokensPerMinute;
            rightLabel = [NSString stringWithFormat:@"%ld", (long)lround(rate)];
        }
        NSDictionary *rightAttrs = @{
            NSFontAttributeName: [NSFont boldSystemFontOfSize:11],
            NSForegroundColorAttributeName: [NSColor colorWithCalibratedWhite:0.75 alpha:1.0]
        };
        NSSize rightSize = [rightLabel sizeWithAttributes:rightAttrs];
        NSPoint rightPoint = NSMakePoint(self.bounds.size.width - rightSize.width - 12, (self.bounds.size.height - rightSize.height) / 2);
        [rightLabel drawAtPoint:rightPoint withAttributes:rightAttrs];
        
    } else {
        // Full graph drawing
        
        // Draw total rate filled (FG1)
        NSColor *fg1 = [NSColor colorWithCalibratedRed:0.0 green:0.7 blue:0.9 alpha:1.0];
        NSColor *fg2 = [NSColor colorWithCalibratedRed:0.4 green:0.85 blue:0.9 alpha:1.0];
        NSColor *fg3 = [NSColor colorWithCalibratedRed:0.7 green:0.95 blue:1.0 alpha:1.0];
        
        NSRect graphRect = NSInsetRect(self.bounds, 20, 20);
        
        double totalTokens = self.miner.aiTokensTotalRate;
        double maxTokens = MAX(1.0, self.miner.aiTokensMaxRate);
        CGFloat totalFrac = (CGFloat)(totalTokens / maxTokens);
        NSRect totalBar = NSMakeRect(NSMinX(graphRect), NSMinY(graphRect), graphRect.size.width * totalFrac, graphRect.size.height / 3);
        [fg1 setFill];
        NSRectFill(totalBar);
        
        if (showBreakdown) {
            // Prompt line (FG2)
            double promptTokens = self.miner.aiTokensPromptRate;
            CGFloat promptFrac = (CGFloat)(promptTokens / maxTokens);
            NSRect promptBar = NSMakeRect(NSMinX(graphRect), NSMinY(graphRect) + graphRect.size.height / 3, graphRect.size.width * promptFrac, graphRect.size.height / 3);
            [fg2 setFill];
            NSRectFill(promptBar);
            
            // Completion line (FG3)
            double completionTokens = self.miner.aiTokensCompletionRate;
            CGFloat completionFrac = (CGFloat)(completionTokens / maxTokens);
            NSRect completionBar = NSMakeRect(NSMinX(graphRect), NSMinY(graphRect) + 2 * graphRect.size.height / 3, graphRect.size.width * completionFrac, graphRect.size.height / 3);
            [fg3 setFill];
            NSRectFill(completionBar);
        }
        
        // Draw labels: "AI" and current tokens/min
        NSDictionary *labelAttrs = @{
            NSFontAttributeName: [NSFont boldSystemFontOfSize:14],
            NSForegroundColorAttributeName: [NSColor colorWithCalibratedWhite:0.85 alpha:1.0]
        };
        NSString *leftLabel = @"AI";
        NSSize leftSize = [leftLabel sizeWithAttributes:labelAttrs];
        NSPoint leftPoint = NSMakePoint(20, NSMaxY(self.bounds) - leftSize.height - 10);
        [leftLabel drawAtPoint:leftPoint withAttributes:labelAttrs];
        
        NSString *rightLabel = @"n/a";
        if (showRate && self.miner.isRunning) {
            rightLabel = [NSString stringWithFormat:@"%ld tokens/min", (long)lround(self.miner.aiTokensPerMinute)];
        }
        NSSize rightSize = [rightLabel sizeWithAttributes:labelAttrs];
        NSPoint rightPoint = NSMakePoint(NSMaxX(self.bounds) - rightSize.width - 20, NSMaxY(self.bounds) - rightSize.height - 10);
        [rightLabel drawAtPoint:rightPoint withAttributes:labelAttrs];
    }
}

@end
