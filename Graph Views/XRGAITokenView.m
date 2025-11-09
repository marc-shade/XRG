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
#import "XRGAITokensObserver.h"
#import "XRGSettings.h"

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
    NSRectFill([self bounds]);

    if ([self shouldDrawMiniGraph]) {
        [self drawMiniGraph:self.bounds];
        return;
    }

    gc.shouldAntialias = [appSettings antiAliasing];

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
    XRGSettings *settings = [XRGSettings sharedSettings];
    XRGAITokensObserver *observer = [XRGAITokensObserver shared];

    // Get user preferences
    BOOL showRate = settings.aiTokensShowRate;
    BOOL showBreakdown = settings.aiTokensShowBreakdown;
    BOOL aggregateByModel = settings.aiTokensAggregateByModel;
    BOOL aggregateByProvider = settings.aiTokensAggregateByProvider;
    NSInteger dailyBudget = settings.aiTokensDailyBudget;

    [label appendString:@"AI Tokens"];

    // Get Observer data
    NSUInteger sessionTotal = observer.sessionTotalTokens;
    NSUInteger dailyTotal = observer.dailyTotalTokens;

    // Show current rate if enabled
    if (showRate) {
        UInt32 claudeRate = [tokenMiner claudeTokenRate];
        UInt32 codexRate = [tokenMiner codexTokenRate];
        UInt32 otherRate = [tokenMiner otherTokenRate];
        UInt32 totalRate = claudeRate + codexRate + otherRate;

        if (totalRate >= 1000) {
            [label appendFormat:@"\nRate: %uK/s", totalRate / 1000];
        } else {
            [label appendFormat:@"\nRate: %u/s", totalRate];
        }
    }

    // Show session totals
    if (sessionTotal >= 1000000) {
        [label appendFormat:@"\nSession: %luM", (unsigned long)(sessionTotal / 1000000)];
    } else if (sessionTotal >= 1000) {
        [label appendFormat:@"\nSession: %luK", (unsigned long)(sessionTotal / 1000)];
    } else {
        [label appendFormat:@"\nSession: %lu", (unsigned long)sessionTotal];
    }

    // Show daily totals with budget if configured
    if (dailyBudget > 0) {
        CGFloat progress = (CGFloat)dailyTotal / (CGFloat)dailyBudget * 100.0;
        if (dailyTotal >= 1000000) {
            [label appendFormat:@"\nDaily: %luM/", (unsigned long)(dailyTotal / 1000000)];
        } else if (dailyTotal >= 1000) {
            [label appendFormat:@"\nDaily: %luK/", (unsigned long)(dailyTotal / 1000)];
        } else {
            [label appendFormat:@"\nDaily: %lu/", (unsigned long)dailyTotal];
        }

        if (dailyBudget >= 1000000) {
            [label appendFormat:@"%ldM (%.0f%%)", (long)(dailyBudget / 1000000), progress];
        } else if (dailyBudget >= 1000) {
            [label appendFormat:@"%ldK (%.0f%%)", (long)(dailyBudget / 1000), progress];
        } else {
            [label appendFormat:@"%ld (%.0f%%)", (long)dailyBudget, progress];
        }
    } else {
        if (dailyTotal >= 1000000) {
            [label appendFormat:@"\nDaily: %luM", (unsigned long)(dailyTotal / 1000000)];
        } else if (dailyTotal >= 1000) {
            [label appendFormat:@"\nDaily: %luK", (unsigned long)(dailyTotal / 1000)];
        } else {
            [label appendFormat:@"\nDaily: %lu", (unsigned long)dailyTotal];
        }
    }

    // Show breakdown if enabled
    if (showBreakdown) {
        if (aggregateByModel) {
            NSDictionary *modelTotals = observer.dailyByModel;

            if (modelTotals.count > 0) {
                [label appendString:@"\n─ By Model ─"];
                for (NSString *model in [modelTotals.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
                    NSUInteger total = [modelTotals[model] unsignedIntegerValue];

                    if (total >= 1000000) {
                        [label appendFormat:@"\n%@: %luM", model, (unsigned long)(total / 1000000)];
                    } else if (total >= 1000) {
                        [label appendFormat:@"\n%@: %luK", model, (unsigned long)(total / 1000)];
                    } else {
                        [label appendFormat:@"\n%@: %lu", model, (unsigned long)total];
                    }
                }
            }
        }

        if (aggregateByProvider) {
            NSDictionary *providerTotals = observer.dailyByProvider;

            if (providerTotals.count > 0) {
                [label appendString:@"\n─ By Provider ─"];
                for (NSString *provider in [providerTotals.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
                    NSUInteger total = [providerTotals[provider] unsignedIntegerValue];

                    if (total >= 1000000) {
                        [label appendFormat:@"\n%@: %luM", provider, (unsigned long)(total / 1000000)];
                    } else if (total >= 1000) {
                        [label appendFormat:@"\n%@: %luK", provider, (unsigned long)(total / 1000)];
                    } else {
                        [label appendFormat:@"\n%@: %lu", provider, (unsigned long)total];
                    }
                }
            }
        }
    }

    [self drawLeftText:label centerText:nil rightText:nil inRect:[self paddedTextRect]];
    gc.shouldAntialias = YES;
}

- (void)drawMiniGraph:(NSRect)rect {
    // Mini graph for when the view is very small
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    gc.shouldAntialias = [appSettings antiAliasing];

    NSRect graphRect = NSMakeRect(0, 0, rect.size.width, rect.size.height);
    XRGDataSet *totalData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:[tokenMiner claudeTokenData]];
    [totalData addOtherDataSetValues:[tokenMiner codexTokenData]];
    [totalData addOtherDataSetValues:[tokenMiner otherTokenData]];

    CGFloat maxValue = [totalData max];
    if (maxValue == 0) maxValue = 1000;

    [self drawGraphWithDataFromDataSet:totalData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:[appSettings graphFG1Color]];
}

#pragma mark - Context Menu

- (NSMenu *)menuForEvent:(NSEvent *)theEvent {
    NSMenu *myMenu = [[NSMenu alloc] initWithTitle:@"AI Token View"];
    NSMenuItem *tMI;

    XRGSettings *settings = [XRGSettings sharedSettings];
    XRGAITokensObserver *observer = [XRGAITokensObserver shared];

    // Session Statistics Section
    tMI = [[NSMenuItem alloc] initWithTitle:@"Session Statistics" action:@selector(emptyEvent:) keyEquivalent:@""];
    [tMI setEnabled:NO];
    [myMenu addItem:tMI];

    NSUInteger sessionPrompt = observer.sessionPromptTokens;
    NSUInteger sessionCompletion = observer.sessionCompletionTokens;
    NSUInteger sessionTotal = observer.sessionTotalTokens;

    tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Prompt: %@ tokens", [self formatNumber:sessionPrompt]] action:@selector(emptyEvent:) keyEquivalent:@""];
    [myMenu addItem:tMI];

    tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Completion: %@ tokens", [self formatNumber:sessionCompletion]] action:@selector(emptyEvent:) keyEquivalent:@""];
    [myMenu addItem:tMI];

    tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Total: %@ tokens", [self formatNumber:sessionTotal]] action:@selector(emptyEvent:) keyEquivalent:@""];
    [myMenu addItem:tMI];

    [myMenu addItem:[NSMenuItem separatorItem]];

    // Daily Statistics Section
    tMI = [[NSMenuItem alloc] initWithTitle:@"Daily Statistics" action:@selector(emptyEvent:) keyEquivalent:@""];
    [tMI setEnabled:NO];
    [myMenu addItem:tMI];

    NSUInteger dailyTotal = observer.dailyTotalTokens;
    tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Total: %@ tokens", [self formatNumber:dailyTotal]] action:@selector(emptyEvent:) keyEquivalent:@""];
    [myMenu addItem:tMI];

    NSInteger dailyBudget = settings.aiTokensDailyBudget;
    if (dailyBudget > 0) {
        CGFloat progress = (CGFloat)dailyTotal / (CGFloat)dailyBudget * 100.0;
        tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Budget: %@ / %@ (%1.1f%%)", [self formatNumber:dailyTotal], [self formatNumber:dailyBudget], progress] action:@selector(emptyEvent:) keyEquivalent:@""];
        [myMenu addItem:tMI];
    }

    // Model Breakdown (if enabled)
    if (settings.aiTokensAggregateByModel) {
        NSDictionary *modelTotals = observer.dailyByModel;
        if (modelTotals.count > 0) {
            [myMenu addItem:[NSMenuItem separatorItem]];

            tMI = [[NSMenuItem alloc] initWithTitle:@"By Model" action:@selector(emptyEvent:) keyEquivalent:@""];
            [tMI setEnabled:NO];
            [myMenu addItem:tMI];

            for (NSString *model in [modelTotals.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
                NSUInteger total = [modelTotals[model] unsignedIntegerValue];
                tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  %@: %@ tokens", model, [self formatNumber:total]] action:@selector(emptyEvent:) keyEquivalent:@""];
                [myMenu addItem:tMI];
            }
        }
    }

    // Provider Breakdown (if enabled)
    if (settings.aiTokensAggregateByProvider) {
        NSDictionary *providerTotals = observer.dailyByProvider;
        if (providerTotals.count > 0) {
            [myMenu addItem:[NSMenuItem separatorItem]];

            tMI = [[NSMenuItem alloc] initWithTitle:@"By Provider" action:@selector(emptyEvent:) keyEquivalent:@""];
            [tMI setEnabled:NO];
            [myMenu addItem:tMI];

            for (NSString *provider in [providerTotals.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
                NSUInteger total = [providerTotals[provider] unsignedIntegerValue];
                tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  %@: %@ tokens", provider, [self formatNumber:total]] action:@selector(emptyEvent:) keyEquivalent:@""];
                [myMenu addItem:tMI];
            }
        }
    }

    // Actions Section
    [myMenu addItem:[NSMenuItem separatorItem]];

    tMI = [[NSMenuItem alloc] initWithTitle:@"Reset Daily Counters" action:@selector(resetDailyCounters:) keyEquivalent:@""];
    [myMenu addItem:tMI];

    tMI = [[NSMenuItem alloc] initWithTitle:@"Open Database Location..." action:@selector(openDatabaseLocation:) keyEquivalent:@""];
    [myMenu addItem:tMI];

    return myMenu;
}

- (void)emptyEvent:(NSEvent *)theEvent {
    // Informational menu items - no action needed
}

- (void)resetDailyCounters:(NSEvent *)theEvent {
    XRGAITokensObserver *observer = [XRGAITokensObserver shared];
    [observer resetDaily];
}

- (void)openDatabaseLocation:(NSEvent *)theEvent {
    NSString *dbPath = [@"~/.claude/monitoring" stringByExpandingTildeInPath];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:dbPath]];
}

- (NSString *)formatNumber:(NSUInteger)number {
    if (number >= 1000000) {
        return [NSString stringWithFormat:@"%1.1fM", (float)number / 1000000.0f];
    } else if (number >= 1000) {
        return [NSString stringWithFormat:@"%1.1fK", (float)number / 1000.0f];
    } else {
        return [NSString stringWithFormat:@"%lu", (unsigned long)number];
    }
}

@end
