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
//  XRGAITokenView.m
//

#import "XRGAITokenView.h"
#import "XRGGraphWindow.h"
#import "XRGCommon.h"

@implementation XRGAITokenView

- (void)awakeFromNib {
    [super awakeFromNib];

    tokenMiner = [[XRGAITokenMiner alloc] init];

    parentWindow = (XRGGraphWindow *)[self window];
    [parentWindow setAiTokenView:self];
    [parentWindow initTimers];
    appSettings = [parentWindow appSettings];
    moduleManager = [parentWindow moduleManager];

    textRectHeight = [appSettings textRectHeight];

    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    m = [[XRGModule alloc] initWithName:@"AI Tokens" andReference:self];
    m.doesFastUpdate = NO;
    m.doesGraphUpdate = YES;
    m.doesMin5Update = NO;
    m.doesMin30Update = NO;
    m.displayOrder = 9;  // Display after Stock (8)
    [self updateMinSize];
    [m setIsDisplayed:(bool)[defs boolForKey:XRG_showAITokenGraph]];

    [[parentWindow moduleManager] addModule:m];
    [self setGraphSize:[m currentSize]];
}

- (void)setGraphSize:(NSSize)newSize {
    NSSize tmpSize;
    tmpSize.width = newSize.width;
    tmpSize.height = newSize.height;
    if (tmpSize.width < 23) tmpSize.width = 23;
    if (tmpSize.width > 20000) tmpSize.width = 20000;
    [self setWidth:tmpSize.width];
    graphSize = tmpSize;
}

- (void)setWidth:(int)newWidth {
    int newNumSamples = newWidth - 19;
    if (newNumSamples < 0) return;

    numSamples = newNumSamples;
    [tokenMiner setDataSize:newNumSamples];
}

- (void)updateMinSize {
    [m setMinWidth:[@"Claude: 99999K tok/s" sizeWithAttributes:[appSettings alignRightAttributes]].width + 19 + 6];
    [m setMinHeight:[appSettings textRectHeight]];
}

- (void)graphUpdate:(NSTimer *)aTimer {
    [tokenMiner getLatestTokenInfo];
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)rect {
    if ([self isHidden]) return;

    NSGraphicsContext *gc = [NSGraphicsContext currentContext];

    #ifdef XRG_DEBUG
        NSLog(@"In AI Token DrawRect.");
    #endif

    textRectHeight = [appSettings textRectHeight];

    [[appSettings graphBGColor] set];
    NSRect bounds = [self bounds];
    CGContextFillRect(gc.CGContext, bounds);

    if ([self shouldDrawMiniGraph]) {
        [self drawMiniGraph:self.bounds];
        return;
    }

    [gc setShouldAntialias:[appSettings antiAliasing]];

    // Draw the token usage graphs
    NSColor *claudeColor = [appSettings graphFG1Color];
    NSColor *codexColor = [appSettings graphFG2Color];
    NSColor *otherColor = [appSettings graphFG3Color];

    NSRect graphRect = NSMakeRect(0, 0, numSamples, graphSize.height - textRectHeight);

    // Create stacked graph data
    XRGDataSet *totalData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:[tokenMiner claudeTokenData]];
    [totalData addOtherDataSetValues:[tokenMiner codexTokenData]];
    [totalData addOtherDataSetValues:[tokenMiner otherTokenData]];

    CGFloat maxValue = [totalData max];
    if (maxValue == 0) maxValue = 1000;  // Default scale

    // Draw stacked area graphs (bottom to top: Claude, Codex, Other)
    [self drawGraphWithDataFromDataSet:totalData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:otherColor];

    XRGDataSet *claudeCodexData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:[tokenMiner claudeTokenData]];
    [claudeCodexData addOtherDataSetValues:[tokenMiner codexTokenData]];
    [self drawGraphWithDataFromDataSet:claudeCodexData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:codexColor];

    [self drawGraphWithDataFromDataSet:[tokenMiner claudeTokenData] maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:claudeColor];

    // Draw current rate indicator on the right side
    NSRect indicatorRect = NSMakeRect(numSamples, 0, 2, graphSize.height - textRectHeight);
    [[appSettings borderColor] set];
    NSRectFill(indicatorRect);

    indicatorRect.origin.x += 2;
    indicatorRect.size.width = 8;

    // Draw Claude rate
    CGFloat totalRate = [tokenMiner totalTokenRate];
    if (totalRate > 0) {
        indicatorRect.size.height = (CGFloat)[tokenMiner claudeTokenRate] / totalRate * (graphSize.height - textRectHeight);
        [claudeColor set];
        NSRectFill(indicatorRect);

        // Draw Codex rate
        indicatorRect.origin.y += indicatorRect.size.height;
        indicatorRect.size.height = (CGFloat)[tokenMiner codexTokenRate] / totalRate * (graphSize.height - textRectHeight);
        [codexColor set];
        NSRectFill(indicatorRect);

        // Draw Other rate
        indicatorRect.origin.y += indicatorRect.size.height;
        indicatorRect.size.height = (CGFloat)[tokenMiner otherTokenRate] / totalRate * (graphSize.height - textRectHeight);
        [otherColor set];
        NSRectFill(indicatorRect);
    }

    // Draw text labels
    NSMutableString *label = [[NSMutableString alloc] init];
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];

    // Get user preferences
    BOOL showRates = [defs boolForKey:XRG_aiTokenShowRates];
    BOOL showTotals = [defs boolForKey:XRG_aiTokenShowTotals];
    BOOL showCost = [defs boolForKey:XRG_aiTokenShowCost];

    // Format token data
    UInt32 claudeRate = [tokenMiner claudeTokenRate];
    UInt32 codexRate = [tokenMiner codexTokenRate];
    UInt32 totalTokens = (UInt32)([tokenMiner totalClaudeTokens] + [tokenMiner totalCodexTokens] + [tokenMiner totalOtherTokens]);
    double totalCost = [tokenMiner totalCostUSD];

    [label appendString:@"AI Tokens"];

    // Show rates if enabled (default: YES)
    if (showRates) {
        if (claudeRate >= 1000) {
            [label appendFormat:@"\nClaude: %uK/s", claudeRate / 1000];
        } else {
            [label appendFormat:@"\nClaude: %u/s", claudeRate];
        }

        if (codexRate >= 1000) {
            [label appendFormat:@"\nCodex: %uK/s", codexRate / 1000];
        } else {
            [label appendFormat:@"\nCodex: %u/s", codexRate];
        }
    }

    // Show totals if enabled (default: YES)
    if (showTotals) {
        if (totalTokens >= 1000000) {
            [label appendFormat:@"\nTotal: %uM", totalTokens / 1000000];
        } else if (totalTokens >= 1000) {
            [label appendFormat:@"\nTotal: %uK", totalTokens / 1000];
        } else {
            [label appendFormat:@"\nTotal: %u", totalTokens];
        }
    }

    // Show cost if enabled and available (default: YES)
    if (showCost && totalCost > 0.0) {
        [label appendFormat:@"\nCost: $%.2f", totalCost];
    }

    [self drawLeftText:label centerText:nil rightText:nil inRect:[self paddedTextRect]];
    [gc setShouldAntialias:YES];
}

- (void)drawMiniGraph:(NSRect)rect {
    // Mini graph for when the view is very small
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc setShouldAntialias:[appSettings antiAliasing]];

    NSRect graphRect = NSMakeRect(0, 0, rect.size.width, rect.size.height);
    XRGDataSet *totalData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:[tokenMiner claudeTokenData]];
    [totalData addOtherDataSetValues:[tokenMiner codexTokenData]];
    [totalData addOtherDataSetValues:[tokenMiner otherTokenData]];

    CGFloat maxValue = [totalData max];
    if (maxValue == 0) maxValue = 1000;

    [self drawGraphWithDataFromDataSet:totalData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:[appSettings graphFG1Color]];
}

@end
