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
#import "XRGDataSet.h"

@implementation XRGAITokenView

- (void)awakeFromNib {
    [super awakeFromNib];

    tokenMiner = [[XRGAITokenMiner alloc] init];

    parentWindow = (XRGGraphWindow *)[self window];
    if (!parentWindow) {
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenView] Warning: parentWindow is nil during awakeFromNib");
#endif
        return;
    }

    [parentWindow setAiTokenView:self];
    [parentWindow initTimers];
    appSettings = [parentWindow appSettings];
    moduleManager = [parentWindow moduleManager];

    if (!appSettings || !moduleManager) {
#ifdef XRG_DEBUG
        NSLog(@"[XRGAITokenView] Warning: appSettings or moduleManager is nil");
#endif
        return;
    }

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

    [moduleManager addModule:m];
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
    if (!tokenMiner) return;  // Prevent crash if miner not initialized
    [tokenMiner getLatestTokenInfo];
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)rect {
    if ([self isHidden]) return;
    if (!appSettings) return;  // Prevent crash if not properly initialized
    if (!tokenMiner) return;   // Prevent crash if miner not initialized

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
    NSColor *geminiColor = [appSettings graphFG3Color];

    NSRect graphRect = NSMakeRect(0, 0, numSamples, graphSize.height - textRectHeight);
    NSInteger graphStyle = [appSettings graphStyle];

    // Draw pixel grid background (style 2)
    if (graphStyle == 2) {
        [self drawPixelGrid:graphRect withSpacing:4.0 color:[appSettings borderColor]];
    }

    // Create/update stacked graph data using cached datasets to avoid memory leaks
    XRGDataSet *claudeData = [tokenMiner claudeTokenData];
    XRGDataSet *codexData = [tokenMiner codexTokenData];
    XRGDataSet *geminiData = [tokenMiner geminiTokenData];

    // Defensive nil checks - skip graph drawing if data not ready
    if (!claudeData || claudeData.numValues == 0) {
        return;
    }

    // Lazily create or resize cached datasets
    if (!cachedTotalData || cachedTotalData.numValues != claudeData.numValues) {
        cachedTotalData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:claudeData];
        if (!cachedTotalData) return;  // Allocation failed
    } else {
        // Copy claude data into existing dataset
        if (claudeData.values && cachedTotalData.values && claudeData.numValues > 0) {
            memcpy(cachedTotalData.values, claudeData.values, claudeData.numValues * sizeof(CGFloat));
            cachedTotalData.min = claudeData.min;
            cachedTotalData.max = claudeData.max;
            cachedTotalData.sum = claudeData.sum;
            cachedTotalData.currentIndex = claudeData.currentIndex;
        }
    }
    if (codexData) [cachedTotalData addOtherDataSetValues:codexData];
    if (geminiData) [cachedTotalData addOtherDataSetValues:geminiData];

    CGFloat maxValue = [cachedTotalData max];
    if (maxValue == 0) maxValue = 1000;  // Default scale

    // Draw stacked area graphs (bottom to top: Claude, Codex, Gemini)
    [self drawGraphWithDataFromDataSet:cachedTotalData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:geminiColor];

    // Lazily create or resize cached claude+codex dataset
    if (!cachedClaudeCodexData || cachedClaudeCodexData.numValues != claudeData.numValues) {
        cachedClaudeCodexData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:claudeData];
        if (!cachedClaudeCodexData) return;  // Allocation failed
    } else {
        // Copy claude data into existing dataset
        if (claudeData.values && cachedClaudeCodexData.values && claudeData.numValues > 0) {
            memcpy(cachedClaudeCodexData.values, claudeData.values, claudeData.numValues * sizeof(CGFloat));
            cachedClaudeCodexData.min = claudeData.min;
            cachedClaudeCodexData.max = claudeData.max;
            cachedClaudeCodexData.sum = claudeData.sum;
            cachedClaudeCodexData.currentIndex = claudeData.currentIndex;
        }
    }
    if (codexData) [cachedClaudeCodexData addOtherDataSetValues:codexData];
    [self drawGraphWithDataFromDataSet:cachedClaudeCodexData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:codexColor];

    if (claudeData) [self drawGraphWithDataFromDataSet:claudeData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:claudeColor];

    // Apply selected graph style overlay
    switch (graphStyle) {
        case 1:  // Scanlines
            [self drawScanlines:graphRect];
            break;
        case 3:  // Retro Dots - data-aware version for AI Token view
            [self drawPixelDotsWithDataFromDataSet:cachedTotalData maxValue:maxValue inRect:graphRect color:geminiColor dotSize:2.0];
            [self drawPixelDotsWithDataFromDataSet:cachedClaudeCodexData maxValue:maxValue inRect:graphRect color:codexColor dotSize:2.0];
            [self drawPixelDotsWithDataFromDataSet:[tokenMiner claudeTokenData] maxValue:maxValue inRect:graphRect color:claudeColor dotSize:2.0];
            break;
        default:  // 0 = Normal, 2 = Pixel Grid (already drawn above)
            break;
    }

    // Draw current rate indicator on the right side
    NSRect indicatorRect = NSMakeRect(numSamples, 0, 2, graphSize.height - textRectHeight);
    [[appSettings borderColor] set];
    NSRectFill(indicatorRect);

    indicatorRect.origin.x += 2;
    indicatorRect.size.width = 8;

    // Draw Claude rate
    CGFloat indicatorTotalRate = [tokenMiner totalTokenRate];
    if (indicatorTotalRate > 0) {
        indicatorRect.size.height = (CGFloat)[tokenMiner claudeTokenRate] / indicatorTotalRate * (graphSize.height - textRectHeight);
        [claudeColor set];
        NSRectFill(indicatorRect);

        // Draw Codex rate
        indicatorRect.origin.y += indicatorRect.size.height;
        indicatorRect.size.height = (CGFloat)[tokenMiner codexTokenRate] / indicatorTotalRate * (graphSize.height - textRectHeight);
        [codexColor set];
        NSRectFill(indicatorRect);

        // Draw Gemini rate
        indicatorRect.origin.y += indicatorRect.size.height;
        indicatorRect.size.height = (CGFloat)[tokenMiner geminiTokenRate] / indicatorTotalRate * (graphSize.height - textRectHeight);
        [geminiColor set];
        NSRectFill(indicatorRect);
    }

    // Draw provider color legend at top-right of graph area
    NSDictionary *legendAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:9],
        NSForegroundColorAttributeName: [appSettings textColor]
    };

    CGFloat legendY = graphSize.height - textRectHeight - 12;
    CGFloat legendX = numSamples - 50;
    CGFloat legendBoxSize = 8;
    CGFloat legendSpacing = 11;

    // Only show legend if there's any data
    if ([tokenMiner totalClaudeTokens] > 0 || [tokenMiner totalCodexTokens] > 0 || [tokenMiner totalGeminiTokens] > 0) {
        // Claude legend entry (bottom, since stack is bottom-to-top)
        if ([tokenMiner totalClaudeTokens] > 0) {
            NSRect claudeBox = NSMakeRect(legendX, legendY, legendBoxSize, legendBoxSize);
            [claudeColor set];
            NSRectFill(claudeBox);
            [@"C" drawAtPoint:NSMakePoint(legendX + legendBoxSize + 2, legendY - 1) withAttributes:legendAttributes];
            legendY -= legendSpacing;
        }

        // Codex legend entry
        if ([tokenMiner totalCodexTokens] > 0) {
            NSRect codexBox = NSMakeRect(legendX, legendY, legendBoxSize, legendBoxSize);
            [codexColor set];
            NSRectFill(codexBox);
            [@"X" drawAtPoint:NSMakePoint(legendX + legendBoxSize + 2, legendY - 1) withAttributes:legendAttributes];
            legendY -= legendSpacing;
        }

        // Gemini legend entry (top)
        if ([tokenMiner totalGeminiTokens] > 0) {
            NSRect geminiBox = NSMakeRect(legendX, legendY, legendBoxSize, legendBoxSize);
            [geminiColor set];
            NSRectFill(geminiBox);
            [@"G" drawAtPoint:NSMakePoint(legendX + legendBoxSize + 2, legendY - 1) withAttributes:legendAttributes];
        }
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

    // Get per-provider rates
    UInt32 claudeRate = [tokenMiner claudeTokenRate];
    UInt32 codexRate = [tokenMiner codexTokenRate];
    UInt32 geminiRate = [tokenMiner geminiTokenRate];
    UInt32 totalRate = claudeRate + codexRate + geminiRate;

    // Get per-provider totals
    UInt64 claudeTotal = [tokenMiner totalClaudeTokens];
    UInt64 codexTotal = [tokenMiner totalCodexTokens];
    UInt64 geminiTotal = [tokenMiner totalGeminiTokens];

    // Always show per-provider totals for visibility
    if (claudeTotal > 0 || codexTotal > 0 || geminiTotal > 0) {
        [label appendString:@"\n─ Providers ─"];

        // Claude (FG1 color = typically green/cyan)
        if (claudeTotal > 0) {
            if (claudeTotal >= 1000000) {
                [label appendFormat:@"\n● Claude: %lluM", claudeTotal / 1000000];
            } else if (claudeTotal >= 1000) {
                [label appendFormat:@"\n● Claude: %lluK", claudeTotal / 1000];
            } else {
                [label appendFormat:@"\n● Claude: %llu", claudeTotal];
            }
        }

        // Codex (FG2 color = typically blue)
        if (codexTotal > 0) {
            if (codexTotal >= 1000000) {
                [label appendFormat:@"\n● Codex: %lluM", codexTotal / 1000000];
            } else if (codexTotal >= 1000) {
                [label appendFormat:@"\n● Codex: %lluK", codexTotal / 1000];
            } else {
                [label appendFormat:@"\n● Codex: %llu", codexTotal];
            }
        }

        // Gemini (FG3 color = typically orange/red)
        if (geminiTotal > 0) {
            if (geminiTotal >= 1000000) {
                [label appendFormat:@"\n● Gemini: %lluM", geminiTotal / 1000000];
            } else if (geminiTotal >= 1000) {
                [label appendFormat:@"\n● Gemini: %lluK", geminiTotal / 1000];
            } else {
                [label appendFormat:@"\n● Gemini: %llu", geminiTotal];
            }
        }
    }

    // === COST INTELLIGENCE SECTION ===
    double totalCost = [tokenMiner totalCostUSD];
    double burnRate = [tokenMiner costPerHour];

    if (totalCost > 0.001 || burnRate > 0.001) {
        [label appendString:@"\n─ Cost ─"];

        // Total cost
        if (totalCost >= 100.0) {
            [label appendFormat:@"\nTotal: $%.0f", totalCost];
        } else if (totalCost >= 1.0) {
            [label appendFormat:@"\nTotal: $%.2f", totalCost];
        } else {
            [label appendFormat:@"\nTotal: $%.4f", totalCost];
        }

        // Burn rate ($/hour) - only show if actively burning
        if (burnRate > 0.001) {
            if (burnRate >= 10.0) {
                [label appendFormat:@"\nBurn: $%.0f/hr", burnRate];
            } else if (burnRate >= 1.0) {
                [label appendFormat:@"\nBurn: $%.2f/hr", burnRate];
            } else {
                [label appendFormat:@"\nBurn: $%.4f/hr", burnRate];
            }

            // Projected daily cost
            double projectedDaily = [tokenMiner projectedDailyCost];
            if (projectedDaily >= 100.0) {
                [label appendFormat:@"\nProj 24hr: $%.0f", projectedDaily];
            } else if (projectedDaily >= 1.0) {
                [label appendFormat:@"\nProj 24hr: $%.2f", projectedDaily];
            } else if (projectedDaily > 0.001) {
                [label appendFormat:@"\nProj 24hr: $%.4f", projectedDaily];
            }
        }

        // Per-provider costs (only show non-zero)
        double claudeCost = [tokenMiner claudeCostUSD];
        double codexCost = [tokenMiner codexCostUSD];
        double geminiCost = [tokenMiner geminiCostUSD];

        if (claudeCost > 0.001 || codexCost > 0.001 || geminiCost > 0.001) {
            if (claudeCost > 0.001) {
                if (claudeCost >= 1.0) {
                    [label appendFormat:@"\n● C: $%.2f", claudeCost];
                } else {
                    [label appendFormat:@"\n● C: $%.4f", claudeCost];
                }
            }
            if (codexCost > 0.001) {
                if (codexCost >= 1.0) {
                    [label appendFormat:@"\n● X: $%.2f", codexCost];
                } else {
                    [label appendFormat:@"\n● X: $%.4f", codexCost];
                }
            }
            if (geminiCost > 0.001) {
                if (geminiCost >= 1.0) {
                    [label appendFormat:@"\n● G: $%.2f", geminiCost];
                } else {
                    [label appendFormat:@"\n● G: $%.4f", geminiCost];
                }
            }
        }
    }

    // Show current rate if enabled
    if (showRate && totalRate > 0) {
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

    // Reuse cached dataset to avoid memory allocations
    XRGDataSet *claudeData = [tokenMiner claudeTokenData];
    XRGDataSet *codexData = [tokenMiner codexTokenData];
    XRGDataSet *geminiData = [tokenMiner geminiTokenData];

    if (!cachedTotalData || cachedTotalData.numValues != claudeData.numValues) {
        cachedTotalData = [[XRGDataSet alloc] initWithContentsOfOtherDataSet:claudeData];
    } else {
        if (claudeData.values && cachedTotalData.values) {
            memcpy(cachedTotalData.values, claudeData.values, claudeData.numValues * sizeof(CGFloat));
            cachedTotalData.min = claudeData.min;
            cachedTotalData.max = claudeData.max;
            cachedTotalData.sum = claudeData.sum;
            cachedTotalData.currentIndex = claudeData.currentIndex;
        }
    }
    [cachedTotalData addOtherDataSetValues:codexData];
    [cachedTotalData addOtherDataSetValues:geminiData];

    CGFloat maxValue = [cachedTotalData max];
    if (maxValue == 0) maxValue = 1000;

    [self drawGraphWithDataFromDataSet:cachedTotalData maxValue:maxValue inRect:graphRect flipped:NO filled:YES color:[appSettings graphFG1Color]];
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

    // Cost Intelligence Section
    double totalCost = [tokenMiner totalCostUSD];
    if (totalCost > 0.001) {
        [myMenu addItem:[NSMenuItem separatorItem]];

        tMI = [[NSMenuItem alloc] initWithTitle:@"Cost Intelligence" action:@selector(emptyEvent:) keyEquivalent:@""];
        [tMI setEnabled:NO];
        [myMenu addItem:tMI];

        tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Total Cost: $%.4f", totalCost] action:@selector(emptyEvent:) keyEquivalent:@""];
        [myMenu addItem:tMI];

        double burnRate = [tokenMiner costPerHour];
        if (burnRate > 0.001) {
            tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Burn Rate: $%.4f/hr", burnRate] action:@selector(emptyEvent:) keyEquivalent:@""];
            [myMenu addItem:tMI];

            double projectedDaily = [tokenMiner projectedDailyCost];
            tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Projected 24hr: $%.4f", projectedDaily] action:@selector(emptyEvent:) keyEquivalent:@""];
            [myMenu addItem:tMI];
        }

        // Per-provider costs
        double claudeCost = [tokenMiner claudeCostUSD];
        double codexCost = [tokenMiner codexCostUSD];
        double geminiCost = [tokenMiner geminiCostUSD];

        if (claudeCost > 0.001) {
            tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Claude: $%.4f", claudeCost] action:@selector(emptyEvent:) keyEquivalent:@""];
            [myMenu addItem:tMI];
        }
        if (codexCost > 0.001) {
            tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Codex: $%.4f", codexCost] action:@selector(emptyEvent:) keyEquivalent:@""];
            [myMenu addItem:tMI];
        }
        if (geminiCost > 0.001) {
            tMI = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"  Gemini: $%.4f", geminiCost] action:@selector(emptyEvent:) keyEquivalent:@""];
            [myMenu addItem:tMI];
        }
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
