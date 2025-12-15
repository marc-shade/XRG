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
//  PrefController.m
//

#import "XRGPrefController.h"
#import "XRGAppDelegate.h"
#import "XRGGraphWindow.h"
#import "definitions.h"

@interface XRGPrefController ()
@property (strong) NSButton *showAITokensGraph;
@end

@implementation XRGPrefController
- (void)awakeFromNib {
    // Create a toolbar
    toolbar = [[NSToolbar alloc] initWithIdentifier:@"preferenceToolbar"];
    
    // instantiate the dictionary that will hold the toolbar item list
    toolbarItems = [NSMutableDictionary dictionary];
   
    // add the General toolbar item
    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:@"General"];
    [item setLabel:@"General"];
    [item setPaletteLabel:@"General"];
    [item setToolTip:@"General Graph Options"];
    NSImage *generalImg = nil;
    if (@available(macOS 11.0, *)) {
        generalImg = [NSImage imageWithSystemSymbolName:@"gearshape" accessibilityDescription:@"General"];
    }
    if (!generalImg) { generalImg = [NSImage imageNamed:@"Preferences-General.tiff"]; }
    [generalImg setTemplate:YES];
    [item setImage:generalImg];

    [item setTarget:self];
    [item setAction:@selector(General:)];
    toolbarItems[@"General"] = item;

    // add the Appearance toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"Colors"];
    [item setLabel:@"Appearance"];
    [item setPaletteLabel:@"Appearance"];
    [item setToolTip:@"Graph Color, Opacity, and Font Options"];
    NSImage *appearanceImg = nil;
    if (@available(macOS 11.0, *)) {
        appearanceImg = [NSImage imageWithSystemSymbolName:@"paintbrush" accessibilityDescription:@"Appearance"];
        if (!appearanceImg) {
            appearanceImg = [NSImage imageWithSystemSymbolName:@"paintpalette" accessibilityDescription:@"Appearance"];
        }
    }
    if (!appearanceImg) { appearanceImg = [NSImage imageNamed:@"Preferences-Appearance.tiff"]; }
    [appearanceImg setTemplate:YES];
    [item setImage:appearanceImg];

    [item setTarget:self];
    [item setAction:@selector(Colors:)];
    toolbarItems[@"Appearance"] = item;
    
    // add the CPU toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"CPU"];
    [item setLabel:@"CPU"];
    [item setPaletteLabel:@"CPU"];
    [item setToolTip:@"CPU Graph Options"];
    NSImage *cpuImg = nil;
    if (@available(macOS 11.0, *)) {
        cpuImg = [NSImage imageWithSystemSymbolName:@"cpu" accessibilityDescription:@"CPU"];
        if (!cpuImg) {
            cpuImg = [NSImage imageWithSystemSymbolName:@"gauge" accessibilityDescription:@"CPU"];
        }
    }
    if (!cpuImg) { cpuImg = [NSImage imageNamed:@"Preferences-CPU.tiff"]; }
    [cpuImg setTemplate:YES];
    [item setImage:cpuImg];

    [item setTarget:self];
    [item setAction:@selector(CPU:)];
    toolbarItems[@"CPU"] = item;
    
    // add the Memory toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"RAM"];
    [item setLabel:@"Memory"];
    [item setPaletteLabel:@"Memory"];
    [item setToolTip:@"Memory Graph Options"];
    NSImage *memoryImg = nil;
    if (@available(macOS 11.0, *)) {
        memoryImg = [NSImage imageWithSystemSymbolName:@"memorychip" accessibilityDescription:@"Memory"];
    }
    if (!memoryImg) { memoryImg = [NSImage imageNamed:@"Preferences-Memory.tiff"]; }
    [memoryImg setTemplate:YES];
    [item setImage:memoryImg];

    [item setTarget:self];
    [item setAction:@selector(RAM:)];
    toolbarItems[@"RAM"] = item;

    // add the Temperature toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"Temperature"];
    [item setLabel:@"Temperature"];
    [item setPaletteLabel:@"Temperature"];
    [item setToolTip:@"Temperature Graph Options"];
    NSImage *temperatureImg = nil;
    if (@available(macOS 11.0, *)) {
        temperatureImg = [NSImage imageWithSystemSymbolName:@"thermometer" accessibilityDescription:@"Temperature"];
    }
    if (!temperatureImg) { temperatureImg = [NSImage imageNamed:@"Preferences-Temperature.tiff"]; }
    [temperatureImg setTemplate:YES];
    [item setImage:temperatureImg];

    [item setTarget:self];
    [item setAction:@selector(Temperature:)];
    toolbarItems[@"Temperature"] = item;
    
    // add the Network toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"Network"];
    [item setLabel:@"Network"];
    [item setPaletteLabel:@"Network"];
    [item setToolTip:@"Network Graph Options"];
    NSImage *networkImg = nil;
    if (@available(macOS 11.0, *)) {
        networkImg = [NSImage imageWithSystemSymbolName:@"network" accessibilityDescription:@"Network"];
        if (!networkImg) {
            networkImg = [NSImage imageWithSystemSymbolName:@"dot.radiowaves.left.and.right" accessibilityDescription:@"Network"];
        }
    }
    if (!networkImg) { networkImg = [NSImage imageNamed:@"Preferences-Network.tiff"]; }
    [networkImg setTemplate:YES];
    [item setImage:networkImg];

    [item setTarget:self];
    [item setAction:@selector(Network:)];
    toolbarItems[@"Network"] = item;

    // add the Disk toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"Disk"];
    [item setLabel:@"Disk"];
    [item setPaletteLabel:@"Disk"];
    [item setToolTip:@"Disk Graph Options"];
    NSImage *diskImg = nil;
    if (@available(macOS 11.0, *)) {
        diskImg = [NSImage imageWithSystemSymbolName:@"externaldrive" accessibilityDescription:@"Disk"];
    }
    if (!diskImg) { diskImg = [NSImage imageNamed:@"Preferences-Disk.tiff"]; }
    [diskImg setTemplate:YES];
    [item setImage:diskImg];

    [item setTarget:self];
    [item setAction:@selector(Disk:)];
    toolbarItems[@"Disk"] = item;
     
    // add the Weather toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"Weather"];
    [item setLabel:@"Weather"];
    [item setPaletteLabel:@"Weather"];
    [item setToolTip:@"Weather Graph Options"];
    NSImage *weatherImg = nil;
    if (@available(macOS 11.0, *)) {
        weatherImg = [NSImage imageWithSystemSymbolName:@"cloud.sun" accessibilityDescription:@"Weather"];
    }
    if (!weatherImg) { weatherImg = [NSImage imageNamed:@"Preferences-Weather.tiff"]; }
    [weatherImg setTemplate:YES];
    [item setImage:weatherImg];

    [item setTarget:self];
    [item setAction:@selector(Weather:)];
    toolbarItems[@"Weather"] = item;

    // add the Stocks toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"Stocks"];
    [item setLabel:@"Stocks"];
    [item setPaletteLabel:@"Stocks"];
    [item setToolTip:@"Stock Graph Options"];
    NSImage *stocksImg = nil;
    if (@available(macOS 11.0, *)) {
        stocksImg = [NSImage imageWithSystemSymbolName:@"chart.line.uptrend.xyaxis" accessibilityDescription:@"Stocks"];
    }
    if (!stocksImg) { stocksImg = [NSImage imageNamed:@"Preferences-Stocks.tiff"]; }
    [stocksImg setTemplate:YES];
    [item setImage:stocksImg];

    [item setTarget:self];
    [item setAction:@selector(Stocks:)];
    toolbarItems[@"Stocks"] = item;
    
    // add the AI toolbar item
    item = [[NSToolbarItem alloc] initWithItemIdentifier:@"AI"];
    [item setLabel:@"AI"];
    [item setPaletteLabel:@"AI"];
    [item setToolTip:@"AI Token Preferences"];
    NSImage *aiImage = nil;
    if (@available(macOS 11.0, *)) {
        aiImage = [NSImage imageWithSystemSymbolName:@"robot" accessibilityDescription:@"AI"];
        if (!aiImage) {
            aiImage = [NSImage imageWithSystemSymbolName:@"brain" accessibilityDescription:@"AI"];
        }
    }
    if (!aiImage) {
        aiImage = [NSImage imageNamed:@"Preferences-AI.tiff"]; // optional bundled asset
    }
    if (!aiImage) {
        aiImage = [NSImage imageNamed:@"Preferences-Stocks.tiff"]; // final fallback to avoid nil
    }
    [aiImage setTemplate:YES];
    [item setImage:aiImage];

    [item setTarget:self];
    [item setAction:@selector(AI:)];
    toolbarItems[@"AI"] = item;

    // we want to handle the actions for the toolbar
    [toolbar setDelegate:self];
	[toolbar setSelectedItemIdentifier:@"General"];
	[window setTitle:@"General Preferences"];

    // set the GeneralPrefView as the default
    NSRect standardSize = [GeneralPrefView frame];
    [window setContentView:GeneralPrefView];
    [window setContentSize:standardSize.size];
    currentView = GeneralPrefView;
    
    // turn off the customization palette.  
    [toolbar setAllowsUserCustomization:NO];

    // tell the toolbar that it should not save any configuration changes to user defaults.  
    [toolbar setAutosavesConfiguration: NO]; 
    
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];  
    [window setToolbar:toolbar];                             // install the toolbar.


    // Now do other initializations of the preference controls.
    [[NSColorPanel sharedColorPanel] setContinuous:YES];
    
    self.xrgGraphWindow = [(XRGAppDelegate *)[NSApp delegate] xrgGraphWindow];
    
    // Initialize the panel outlets
    [self setUpGeneralPanel];
    [self setUpColorPanel];
    [self setUpCPUPanel];
    [self setUpMemoryPanel];
    [self setUpTemperaturePanel];
    [self setUpNetworkPanel];
    [self setUpDiskPanel];
    [self setUpWeatherPanel];
    [self setUpStockPanel];
    [self setUpAIPanel];
}

- (IBAction)save:(id)sender {
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
        
    [defs setFloat: [backgroundTransparency floatValue] forKey:XRG_backgroundTransparency];
    [defs setFloat: [graphBGTransparency floatValue]    forKey:XRG_graphBGTransparency];
    [defs setFloat: [graphFG1Transparency floatValue]   forKey:XRG_graphFG1Transparency];
    [defs setFloat: [graphFG2Transparency floatValue]   forKey:XRG_graphFG2Transparency];
    [defs setFloat: [graphFG3Transparency floatValue]   forKey:XRG_graphFG3Transparency];
    [defs setFloat: [borderTransparency floatValue]     forKey:XRG_borderTransparency];
    [defs setFloat: [textTransparency floatValue]       forKey:XRG_textTransparency];
    
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject: [backgroundColorWell color] requiringSecureCoding:NO error:nil]
        forKey: XRG_backgroundColor
    ];
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject:[graphBGColorWell color] requiringSecureCoding:NO error:nil]
        forKey: XRG_graphBGColor
    ];
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject:[graphFG1ColorWell color] requiringSecureCoding:NO error:nil]
        forKey:XRG_graphFG1Color
    ];
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject: [graphFG2ColorWell color] requiringSecureCoding:NO error:nil]
        forKey:XRG_graphFG2Color
    ];
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject: [graphFG3ColorWell color] requiringSecureCoding:NO error:nil]
        forKey:XRG_graphFG3Color
    ];
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject: [borderColorWell color] requiringSecureCoding:NO error:nil]
        forKey:XRG_borderColor
    ];
    [defs setObject:
        [NSKeyedArchiver archivedDataWithRootObject: [textColorWell color] requiringSecureCoding:NO error:nil]
        forKey:XRG_textColor
    ];
    
    if ([graphOrientation indexOfSelectedItem] == 0)
        [defs setObject: @"YES" forKey:XRG_graphOrientationVertical];
    else 
        [defs setObject: @"NO"  forKey:XRG_graphOrientationVertical];
    
    [self.xrgGraphWindow setICAO:ICAOCode];
    if ([ICAOCode stringValue]) 
        [defs setObject: [ICAOCode stringValue] forKey:XRG_ICAO];
    else 
        [defs setObject: @"" forKey:XRG_ICAO];
    
        
    [defs setInteger: [secondaryWeatherGraph indexOfSelectedItem] forKey:XRG_secondaryWeatherGraph];
    [defs setInteger: [borderWidthSlider intValue] forKey:XRG_borderWidth];
    [defs setInteger: [temperatureUnits indexOfSelectedItem] forKey:XRG_temperatureUnits];
    [defs setInteger: [distanceUnits indexOfSelectedItem] forKey:XRG_distanceUnits];
    [defs setInteger: [pressureUnits indexOfSelectedItem] forKey:XRG_pressureUnits];
    [defs setFloat:   [graphRefreshValue floatValue] forKey:XRG_graphRefresh]; 
    [defs setObject: ([generalAutoExpandGraph state] == NSControlStateValueOn ? @"YES" : @"NO")  forKey:XRG_autoExpandGraph];
    [defs setObject: ([generalForegroundWhenExpanding state] == NSControlStateValueOn ? @"YES" : @"NO") forKey:XRG_foregroundWhenExpanding];
    [defs setObject: ([generalShowSummary state] == NSControlStateValueOn ? @"YES" : @"NO")      forKey:XRG_showSummary];
    [defs setInteger: [generalMinimizeUpDown indexOfSelectedItem]                    forKey:XRG_minimizeUpDown];
    
    [defs setObject: ([showCPUGraph state] == NSControlStateValueOn ? @"YES" : @"NO")            forKey:XRG_showCPUGraph];    
	[defs setObject: ([showGPUGraph state] == NSControlStateValueOn ? @"YES" : @"NO")            forKey:XRG_showGPUGraph];
    [defs setObject: ([showMemoryGraph state] == NSControlStateValueOn ? @"YES" : @"NO")         forKey:XRG_showMemoryGraph];
    [defs setObject: ([showBatteryGraph state] == NSControlStateValueOn ? @"YES" : @"NO")        forKey:XRG_showBatteryGraph];
    [defs setObject: ([showTemperatureGraph state] == NSControlStateValueOn ? @"YES" : @"NO")    forKey:XRG_showTemperatureGraph];
    [defs setObject: ([showNetGraph state] == NSControlStateValueOn ? @"YES" : @"NO")            forKey:XRG_showNetworkGraph];    
    [defs setObject: ([showDiskGraph state] == NSControlStateValueOn ? @"YES" : @"NO")           forKey:XRG_showDiskGraph];    
    [defs setObject: ([showWeatherGraph state] == NSControlStateValueOn ? @"YES" : @"NO")        forKey:XRG_showWeatherGraph];    
    [defs setObject: ([showStockGraph state] == NSControlStateValueOn ? @"YES" : @"NO")          forKey:XRG_showStockGraph];   

    if (self.showAITokensGraph) {
        [defs setObject:([self.showAITokensGraph state] == NSControlStateValueOn ? @"YES" : @"NO") forKey:XRG_showAITokenGraph];
    }

    // Save AI Token preferences
    if (AIPrefView) {
        NSButton *trackingEnabled = (NSButton *)[AIPrefView viewWithTag:1000];
        NSButton *dailyAutoReset = (NSButton *)[AIPrefView viewWithTag:1001];
        NSTextField *budgetField = (NSTextField *)[AIPrefView viewWithTag:1002];
        NSTextField *notifyField = (NSTextField *)[AIPrefView viewWithTag:1003];
        NSButton *aggregateByModel = (NSButton *)[AIPrefView viewWithTag:1004];
        NSButton *aggregateByProvider = (NSButton *)[AIPrefView viewWithTag:1005];
        NSButton *showRate = (NSButton *)[AIPrefView viewWithTag:1006];
        NSButton *showBreakdown = (NSButton *)[AIPrefView viewWithTag:1007];

        if (trackingEnabled) {
            [defs setBool:([trackingEnabled state] == NSControlStateValueOn) forKey:@"aiTokensTrackingEnabled"];
        }
        if (dailyAutoReset) {
            [defs setBool:([dailyAutoReset state] == NSControlStateValueOn) forKey:@"aiTokensDailyAutoReset"];
        }
        if (budgetField) {
            [defs setInteger:[budgetField integerValue] forKey:@"aiTokensDailyBudget"];
        }
        if (notifyField) {
            [defs setInteger:[notifyField integerValue] forKey:@"aiTokensBudgetNotifyPercent"];
        }
        if (aggregateByModel) {
            [defs setBool:([aggregateByModel state] == NSControlStateValueOn) forKey:@"aiTokensAggregateByModel"];
        }
        if (aggregateByProvider) {
            [defs setBool:([aggregateByProvider state] == NSControlStateValueOn) forKey:@"aiTokensAggregateByProvider"];
        }
        if (showRate) {
            [defs setBool:([showRate state] == NSControlStateValueOn) forKey:@"aiTokensShowRate"];
        }
        if (showBreakdown) {
            [defs setBool:([showBreakdown state] == NSControlStateValueOn) forKey:@"aiTokensShowBreakdown"];
        }

        // Save cyberpunk visual effects settings
        NSButton *showScanlines = (NSButton *)[AIPrefView viewWithTag:1010];
        NSButton *showPixelGrid = (NSButton *)[AIPrefView viewWithTag:1011];
        NSButton *showPixelDots = (NSButton *)[AIPrefView viewWithTag:1012];

        if (showScanlines) {
            [defs setBool:([showScanlines state] == NSControlStateValueOn) forKey:@"showScanlines"];
        }
        if (showPixelGrid) {
            [defs setBool:([showPixelGrid state] == NSControlStateValueOn) forKey:@"showPixelGrid"];
        }
        if (showPixelDots) {
            [defs setBool:([showPixelDots state] == NSControlStateValueOn) forKey:@"showPixelDots"];
        }
    }

    // CPU graph checkboxes
    [defs setObject: ([fastCPUUsageCheckbox state] == NSControlStateValueOn ? @"YES" : @"NO")    forKey:XRG_showCPUBars];
    [defs setObject: ([enableAntiAliasing state] == NSControlStateValueOn ? @"YES" : @"NO")      forKey:XRG_antiAliasing];
    [defs setObject: ([separateCPUColor state] == NSControlStateValueOn ? @"YES" : @"NO")        forKey:XRG_separateCPUColor];
    [defs setObject: ([showLoadAverage state] == NSControlStateValueOn ? @"YES" : @"NO")         forKey:XRG_showLoadAverage];
    [defs setObject: ([showCPUTemperature state] == NSControlStateValueOn ? @"YES" : @"NO")      forKey:XRG_showCPUTemperature];
    [defs setInteger: [cpuTemperatureUnits indexOfSelectedItem] forKey:XRG_cpuTemperatureUnits];
    [defs setObject: ([cpuShowAverageUsage state] == NSControlStateValueOn ? @"YES" : @"NO")     forKey:XRG_cpuShowAverageUsage];
    [defs setObject: ([cpuShowUptime state] == NSControlStateValueOn ? @"YES" : @"NO")           forKey:XRG_cpuShowUptime];

    // Memory graph checkboxes
    [defs setObject: ([memoryShowPagingGraph state] == NSControlStateValueOn ? @"YES" : @"NO")   forKey:XRG_showMemoryPagingGraph];    
    [defs setObject: ([memoryShowWired state] == NSControlStateValueOn ? @"YES" : @"NO")         forKey:XRG_memoryShowWired];    
    [defs setObject: ([memoryShowActive state] == NSControlStateValueOn ? @"YES" : @"NO")        forKey:XRG_memoryShowActive];    
    [defs setObject: ([memoryShowInactive state] == NSControlStateValueOn ? @"YES" : @"NO")      forKey:XRG_memoryShowInactive];    
    [defs setObject: ([memoryShowFree state] == NSControlStateValueOn ? @"YES" : @"NO")          forKey:XRG_memoryShowFree];    
    [defs setObject: ([memoryShowCache state] == NSControlStateValueOn ? @"YES" : @"NO")         forKey:XRG_memoryShowCache];    
    [defs setObject: ([memoryShowPage state] == NSControlStateValueOn ? @"YES" : @"NO")          forKey:XRG_memoryShowPage];
    
    // Temperature graph options
    [defs setInteger: [tempUnits indexOfSelectedItem]                                forKey:XRG_tempUnits];
    if (self.temperatureSensors.count > 0) {
        NSInteger tempFG1SelectedIndex = [tempFG1Location indexOfSelectedItem];
        [defs setObject:tempFG1SelectedIndex > 0 ? self.temperatureSensors[tempFG1SelectedIndex - 1].key : nil forKey:XRG_tempFG1Location];

        NSInteger tempFG2SelectedIndex = [tempFG2Location indexOfSelectedItem];
        [defs setObject:tempFG2SelectedIndex > 0 ? self.temperatureSensors[tempFG2SelectedIndex - 1].key : nil forKey:XRG_tempFG2Location];

        NSInteger tempFG3SelectedIndex = [tempFG3Location indexOfSelectedItem];
        [defs setObject:tempFG3SelectedIndex > 0 ? self.temperatureSensors[tempFG3SelectedIndex - 1].key : nil forKey:XRG_tempFG3Location];
    }

    [defs setObject: ([stockShowChange state] == NSControlStateValueOn ? @"YES" : @"NO")         forKey:XRG_stockShowChange];    
    [defs setObject: ([showDJIA state] == NSControlStateValueOn ? @"YES" : @"NO")                forKey:XRG_showDJIA];
    [defs setObject: ([stickyWindow state] == NSControlStateValueOn ? @"YES" : @"NO")            forKey:XRG_stickyWindow];
    [defs setObject: ([checkForUpdates state] == NSControlStateValueOn ? @"YES" : @"NO")         forKey:XRG_checkForUpdates];
    [defs setObject: ([dropShadow state] == NSControlStateValueOn ? @"YES" : @"NO")              forKey:XRG_dropShadow];
    [defs setObject: ([showTotalBandwidthSinceBoot state] == NSControlStateValueOn ? @"YES" : @"NO") forKey:XRG_showTotalBandwidthSinceBoot];
    [defs setObject: ([showTotalBandwidthSinceLoad state] == NSControlStateValueOn ? @"YES" : @"NO") forKey:XRG_showTotalBandwidthSinceLoad];

    [defs setInteger:[netGraphMode selectedRow] forKey:XRG_netGraphMode];
    [defs setInteger:[diskGraphMode selectedRow] forKey:XRG_diskGraphMode];
    
    // save the minNetGraphScale
    int sInt = [[netMinGraphScaleValue stringValue] intValue];
    if (sInt == INT_MAX || sInt == INT_MIN) {
        sInt = 0;
    }
    if (sInt == 0) {
        if (![[netMinGraphScaleValue stringValue] isEqualToString:@"0"]) {
            sInt = 0;
        }
    }

    if ([netMinGraphScaleUnits indexOfSelectedItem] == 0)
        [defs setInteger:sInt forKey:XRG_netMinGraphScale];
    else if ([netMinGraphScaleUnits indexOfSelectedItem] == 1) 
        [defs setInteger:sInt * 1024 forKey:XRG_netMinGraphScale];
    else
        [defs setInteger:sInt * 1048576 forKey:XRG_netMinGraphScale];
    // done saving the minNetGraphScale
    
    NSInteger selectedRow = [networkInterface indexOfSelectedItem];
    if (selectedRow == 0) {
        [defs setObject:@"All" forKey:XRG_networkInterface];
    }
    else {
        NSArray *interfaces = [self.xrgGraphWindow.netView.miner networkInterfaces];
        if (selectedRow - 1 < [interfaces count])
            [defs setObject:interfaces[(selectedRow - 1)] forKey:XRG_networkInterface];
        else
            [defs setObject:@"All" forKey:XRG_networkInterface];
    }

    [self.xrgGraphWindow setStockSymbols:stockSymbols];
    if ([stockSymbols stringValue])
        [defs setObject:[stockSymbols stringValue] forKey:XRG_stockSymbols];
    else 
        [defs setObject:@"" forKey:XRG_stockSymbols];

    [self.xrgGraphWindow setWindowTitle:windowTitle];
    if ([windowTitle stringValue])
        [defs setObject:[windowTitle stringValue] forKey:XRG_windowTitle];
    else
        [defs setObject:@"" forKey:XRG_windowTitle];
        
    [defs setInteger:[stockGraphTimeFrame indexOfSelectedItem] forKey:XRG_stockGraphTimeFrame];
        
    [defs setInteger:[windowLevel indexOfSelectedItem] - 1 forKey:XRG_windowLevel];
    
    [defs setObject:[NSKeyedArchiver archivedDataWithRootObject:self.xrgGraphWindow.appSettings.graphFont requiringSecureCoding:NO error:nil] forKey:XRG_graphFont];
    
    [defs synchronize];
    [window orderOut:nil];
}

- (IBAction)revert:(id)sender {
}

- (void)setUpGeneralPanel {
    // Setup the window title
    [windowTitle setTarget:self.xrgGraphWindow];
    [windowTitle setAction:@selector(setWindowTitle:)];
    if (self.xrgGraphWindow.appSettings.windowTitle != nil)
        [windowTitle setStringValue:self.xrgGraphWindow.appSettings.windowTitle];
    else
        [windowTitle setStringValue:@""];
    
    // Setup the border width
    [borderWidthSlider setTarget:self.xrgGraphWindow];
    [borderWidthSlider setAction:@selector(setBorderWidthAction:)];
    [borderWidthSlider setIntValue:self.xrgGraphWindow.borderWidth];
    
    // Setup the graph orientation
    [graphOrientation setTarget:self.xrgGraphWindow];
    [graphOrientation setAction:@selector(setGraphOrientation:)];
    if ([self.xrgGraphWindow.moduleManager graphOrientationVertical])
        [graphOrientation selectItemAtIndex:0];
    else
        [graphOrientation selectItemAtIndex:1];
    
    // Setup anti-aliasing
    [enableAntiAliasing setTarget:self.xrgGraphWindow];
    [enableAntiAliasing setAction:@selector(setAntiAliasing:)];
	[enableAntiAliasing setState:self.xrgGraphWindow.appSettings.antiAliasing ? NSControlStateValueOn : NSControlStateValueOff];

    // Setup show CPU graph
    [showCPUGraph setTarget:self.xrgGraphWindow];
    [showCPUGraph setAction:@selector(setShowCPUGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"CPU"] isDisplayed])
        [showCPUGraph setState:NSControlStateValueOn];
    else
        [showCPUGraph setState:NSControlStateValueOff];

	// Setup show GPU graph
	[showGPUGraph setTarget:self.xrgGraphWindow];
	[showGPUGraph setAction:@selector(setShowGPUGraph:)];
	if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"GPU"] isDisplayed])
		[showGPUGraph setState:NSControlStateValueOn];
	else
		[showGPUGraph setState:NSControlStateValueOff];

    // Setup show memory graph
    [showMemoryGraph setTarget:self.xrgGraphWindow];
    [showMemoryGraph setAction:@selector(setShowMemoryGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Memory"] isDisplayed])
        [showMemoryGraph setState:NSControlStateValueOn];
    else
        [showMemoryGraph setState:NSControlStateValueOff];

    // Setup show battery graph
    [showBatteryGraph setTarget:self.xrgGraphWindow];
    [showBatteryGraph setAction:@selector(setShowBatteryGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Battery"] isDisplayed])
        [showBatteryGraph setState:NSControlStateValueOn];
    else
        [showBatteryGraph setState:NSControlStateValueOff];

    // Setup show temperature graph
    [showTemperatureGraph setTarget:self.xrgGraphWindow];
    [showTemperatureGraph setAction:@selector(setShowTemperatureGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Temperature"] isDisplayed])
        [showTemperatureGraph setState:NSControlStateValueOn];
    else
        [showTemperatureGraph setState:NSControlStateValueOff];

    // Setup show network graph
    [showNetGraph setTarget:self.xrgGraphWindow];
    [showNetGraph setAction:@selector(setShowNetGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Network"] isDisplayed])
        [showNetGraph setState:NSControlStateValueOn];
    else
        [showNetGraph setState:NSControlStateValueOff];
    
    // Setup show disk graph
    [showDiskGraph setTarget:self.xrgGraphWindow];
    [showDiskGraph setAction:@selector(setShowDiskGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Disk"] isDisplayed])
        [showDiskGraph setState:NSControlStateValueOn];
    else
        [showDiskGraph setState:NSControlStateValueOff];

    // Setup show weather graph
    [showWeatherGraph setTarget:self.xrgGraphWindow];
    [showWeatherGraph setAction:@selector(setShowWeatherGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Weather"] isDisplayed])
        [showWeatherGraph setState:NSControlStateValueOn];
    else
        [showWeatherGraph setState:NSControlStateValueOff];

    // Setup show stock graph
    [showStockGraph setTarget:self.xrgGraphWindow];
    [showStockGraph setAction:@selector(setShowStockGraph:)];
    if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"Stock"] isDisplayed])
        [showStockGraph setState:NSControlStateValueOn];
    else
        [showStockGraph setState:NSControlStateValueOff];

    // Setup show AI Tokens graph (added programmatically)
    if (!self.showAITokensGraph) {
        NSButton *aiBtn = [NSButton checkboxWithTitle:@"AI Tokens" target:self.xrgGraphWindow action:@selector(setShowAITokenGraph:)];
        // Position the new checkbox just below the Stock checkbox
        NSView *container = [showStockGraph superview];
        NSRect stockFrame = [showStockGraph frame];
        NSRect aiFrame = stockFrame;
        aiFrame.origin.y = NSMinY(stockFrame) - (NSHeight(stockFrame) + 8.0);
        aiBtn.frame = aiFrame;
        
        // Initial state based on module visibility
        if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"AI Tokens"] isDisplayed])
            [aiBtn setState:NSControlStateValueOn];
        else
            [aiBtn setState:NSControlStateValueOff];
        
        [container addSubview:aiBtn];
        self.showAITokensGraph = aiBtn;
    } else {
        // Update state if it already exists
        if ([[self.xrgGraphWindow.moduleManager getModuleByName:@"AI Tokens"] isDisplayed])
            [self.showAITokensGraph setState:NSControlStateValueOn];
        else
            [self.showAITokensGraph setState:NSControlStateValueOff];
    }

    // Setup graph refresh
    [graphRefreshValue setTarget:self];
    [graphRefreshValue setAction:@selector(setGraphRefreshAction:)];
    [graphRefreshValue setFloatValue:self.xrgGraphWindow.appSettings.graphRefresh];
    
    float ref = self.xrgGraphWindow.appSettings.graphRefresh;
    NSString *s;
    if (roundf(ref * 10.) == 10)
        s = @"Graph updates every second";
    else
        s = [[NSString alloc] initWithFormat: @"Graph updates every %2.1f seconds", ref];
        
    [graphRefreshText setStringValue:s];
    
    // Setup window level
    [windowLevel setTarget:self.xrgGraphWindow];
    [windowLevel setAction:@selector(setWindowLevel:)];
    [windowLevel removeAllItems];
    [windowLevel addItemWithTitle:@"Background"];
    [windowLevel addItemWithTitle:@"Normal"];
    [windowLevel addItemWithTitle:@"Foreground"];
    NSInteger selection = self.xrgGraphWindow.appSettings.windowLevel;
    if (selection < -1 || selection > 1)
        [windowLevel selectItemAtIndex:1];
    else
        [windowLevel selectItemAtIndex:selection + 1];

    // Setup sticky window
    [stickyWindow setTarget:self.xrgGraphWindow];
    [stickyWindow setAction:@selector(setStickyWindow:)];
	[stickyWindow setState:self.xrgGraphWindow.appSettings.stickyWindow ? NSControlStateValueOn : NSControlStateValueOff];

    // Setup check for updates
    [checkForUpdates setTarget:self.xrgGraphWindow];
    [checkForUpdates setAction:@selector(setCheckForUpdates:)];
	[checkForUpdates setState:self.xrgGraphWindow.appSettings.checkForUpdates ? NSControlStateValueOn : NSControlStateValueOff];

    // Setup drop shadow
    [dropShadow setTarget:self.xrgGraphWindow];
    [dropShadow setAction:@selector(setDropShadow:)];
	[dropShadow setState:self.xrgGraphWindow.appSettings.dropShadow ? NSControlStateValueOn : NSControlStateValueOff];
	
    // Setup auto-expand graph
    [generalAutoExpandGraph setTarget:self.xrgGraphWindow];
    [generalAutoExpandGraph setAction:@selector(setAutoExpandGraph:)];
	[generalAutoExpandGraph setState:self.xrgGraphWindow.appSettings.autoExpandGraph ? NSControlStateValueOn : NSControlStateValueOff];
	
    // Setup foreground when expanding
    [generalForegroundWhenExpanding setTarget:self.xrgGraphWindow];
    [generalForegroundWhenExpanding setAction:@selector(setForegroundWhenExpanding:)];
	[generalForegroundWhenExpanding setState:self.xrgGraphWindow.appSettings.foregroundWhenExpanding ? NSControlStateValueOn : NSControlStateValueOff];
	
    // Setup show summary
    [generalShowSummary setTarget:self.xrgGraphWindow];
    [generalShowSummary setAction:@selector(setShowSummary:)];
	[generalShowSummary setState:self.xrgGraphWindow.appSettings.showSummary ? NSControlStateValueOn : NSControlStateValueOff];
	
    // Setup minimize up/down
    [generalMinimizeUpDown setTarget:self.xrgGraphWindow];
    [generalMinimizeUpDown setAction:@selector(setMinimizeUpDown:)];
    [generalMinimizeUpDown removeAllItems];
    [generalMinimizeUpDown addItemWithTitle:@"Up/Left"];
    [generalMinimizeUpDown addItemWithTitle:@"Down/Right"];
    selection = self.xrgGraphWindow.appSettings.minimizeUpDown;
    if (selection < 0 || selection > 1)
        [generalMinimizeUpDown selectItemAtIndex:0];
    else
        [generalMinimizeUpDown selectItemAtIndex:selection];    
        
    return;
}

- (void)setUpColorPanel {
    [self setUpWell:backgroundColorWell withTransparency:backgroundTransparency];
    [self setUpWell:graphBGColorWell    withTransparency:graphBGTransparency];
    [self setUpWell:graphFG1ColorWell   withTransparency:graphFG1Transparency];
    [self setUpWell:graphFG2ColorWell   withTransparency:graphFG2Transparency];
    [self setUpWell:graphFG3ColorWell   withTransparency:graphFG3Transparency];
    [self setUpWell:borderColorWell     withTransparency:borderTransparency];
    [self setUpWell:textColorWell       withTransparency:textTransparency];
    
    [font setTarget:self];
    [font setAction:@selector(setFont:)];
}

- (void)setUpCPUPanel {
    // Setup fast CPU usage checkbox
    [fastCPUUsageCheckbox setTarget:self.xrgGraphWindow];
    [fastCPUUsageCheckbox setAction:@selector(setFastCPUUsageCheckbox:)];
	[fastCPUUsageCheckbox setState:self.xrgGraphWindow.appSettings.fastCPUUsage ? NSControlStateValueOn : NSControlStateValueOff];

    // Setup separate CPU color
    [separateCPUColor setTarget:self.xrgGraphWindow];
    [separateCPUColor setAction:@selector(setSeparateCPUColor:)];
	[separateCPUColor setState:self.xrgGraphWindow.appSettings.separateCPUColor ? NSControlStateValueOn : NSControlStateValueOff];

    // Setup show CPU temperature
    [showCPUTemperature setTarget:self.xrgGraphWindow];
    [showCPUTemperature setAction:@selector(setShowCPUTemperature:)];
	[showCPUTemperature setState:self.xrgGraphWindow.appSettings.showCPUTemperature ? NSControlStateValueOn : NSControlStateValueOff];
	
    [cpuTemperatureUnits setTarget:self.xrgGraphWindow];
    [cpuTemperatureUnits setAction:@selector(setCPUTemperatureUnits:)];
    [cpuTemperatureUnits selectItemAtIndex:self.xrgGraphWindow.appSettings.cpuTemperatureUnits];

    // Setup show load average
    [showLoadAverage setTarget:self.xrgGraphWindow];
    [showLoadAverage setAction:@selector(setShowLoadAverage:)];
	[showLoadAverage setState:self.xrgGraphWindow.appSettings.showLoadAverage ? NSControlStateValueOn : NSControlStateValueOff];
	
    // Setup show average cpu usage
    [cpuShowAverageUsage setTarget:self.xrgGraphWindow];
    [cpuShowAverageUsage setAction:@selector(setCPUShowAverageUsage:)];
	[cpuShowAverageUsage setState:self.xrgGraphWindow.appSettings.cpuShowAverageUsage ? NSControlStateValueOn : NSControlStateValueOff];

    // Setup show uptime
    [cpuShowUptime setTarget:self.xrgGraphWindow];
    [cpuShowUptime setAction:@selector(setCPUShowUptime:)];
	[cpuShowUptime setState:self.xrgGraphWindow.appSettings.cpuShowUptime ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)setUpMemoryPanel {
    [memoryShowWired       setTarget:self.xrgGraphWindow];
    [memoryShowActive      setTarget:self.xrgGraphWindow];
    [memoryShowInactive    setTarget:self.xrgGraphWindow];
    [memoryShowFree        setTarget:self.xrgGraphWindow];
    [memoryShowCache       setTarget:self.xrgGraphWindow];
    [memoryShowPage        setTarget:self.xrgGraphWindow];
    [memoryShowPagingGraph setTarget:self.xrgGraphWindow];
    
    [memoryShowWired       setAction:@selector(setMemoryCheckbox:)];
    [memoryShowActive      setAction:@selector(setMemoryCheckbox:)];
    [memoryShowInactive    setAction:@selector(setMemoryCheckbox:)];
    [memoryShowFree        setAction:@selector(setMemoryCheckbox:)];
    [memoryShowCache       setAction:@selector(setMemoryCheckbox:)];
    [memoryShowPage        setAction:@selector(setMemoryCheckbox:)];
    [memoryShowPagingGraph setAction:@selector(setMemoryCheckbox:)];
	
	[memoryShowWired setState:self.xrgGraphWindow.appSettings.memoryShowWired ? NSControlStateValueOn : NSControlStateValueOff];
	[memoryShowActive setState:self.xrgGraphWindow.appSettings.memoryShowActive ? NSControlStateValueOn : NSControlStateValueOff];
	[memoryShowInactive setState:self.xrgGraphWindow.appSettings.memoryShowInactive ? NSControlStateValueOn : NSControlStateValueOff];
	[memoryShowFree setState:self.xrgGraphWindow.appSettings.memoryShowFree ? NSControlStateValueOn : NSControlStateValueOff];
	[memoryShowCache setState:self.xrgGraphWindow.appSettings.memoryShowCache ? NSControlStateValueOn : NSControlStateValueOff];
	[memoryShowPage setState:self.xrgGraphWindow.appSettings.memoryShowPage ? NSControlStateValueOn : NSControlStateValueOff];
	[memoryShowPagingGraph setState:self.xrgGraphWindow.appSettings.showMemoryPagingGraph ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)setUpTemperaturePanel {
    [tempUnits setTarget:self.xrgGraphWindow];
    [tempFG1Location setTarget:self.xrgGraphWindow];
    [tempFG2Location setTarget:self.xrgGraphWindow];
    [tempFG3Location setTarget:self.xrgGraphWindow];
    
    [tempUnits setAction:@selector(setTempUnits:)];
    [tempFG1Location setAction:@selector(setTempFG1Location:)];
    [tempFG2Location setAction:@selector(setTempFG2Location:)];
    [tempFG3Location setAction:@selector(setTempFG3Location:)];
    
    [tempUnits removeAllItems];
    [tempUnits addItemWithTitle:@"Fahrenheit"];
    [tempUnits addItemWithTitle:@"Celsius"];
    [tempUnits selectItemAtIndex:self.xrgGraphWindow.appSettings.tempUnits];
	
    [tempFG1Location removeAllItems];
    [tempFG2Location removeAllItems];
    [tempFG3Location removeAllItems];

    XRGTemperatureMiner *temperatureMiner = [XRGTemperatureMiner shared];
    if (temperatureMiner) {
        NSArray *locations = [temperatureMiner locationKeysIncludingUnknown:[XRGTemperatureView showUnknownSensors]];
        NSMutableArray *sensors = [NSMutableArray array];
        NSMutableArray<NSString *> *locationTitles = [NSMutableArray array];
        for (NSString *location in locations) {
            XRGSensorData *sensor = [temperatureMiner sensorForLocation:location];
            [sensors addObject:sensor];
            [locationTitles addObject:sensor.label];
        }
        self.temperatureSensors = sensors;
        NSInteger numLocations = [sensors count];

        if (numLocations > 0) {
            [tempFG1Location addItemWithTitle:@"None"];
            [tempFG2Location addItemWithTitle:@"None"];
            [tempFG3Location addItemWithTitle:@"None"];

            for (NSInteger i = 0; i < sensors.count; i++) {
                XRGSensorData *sensor = sensors[i];

                NSMenuItem *item1 = [[NSMenuItem alloc] initWithTitle:sensor.label action:@selector(setTempFG1Location:) keyEquivalent:@""];
                item1.target = self.xrgGraphWindow;
                item1.representedObject = sensor.key;
                [[tempFG1Location menu] addItem:item1];

                NSMenuItem *item2 = [[NSMenuItem alloc] initWithTitle:sensor.label action:@selector(setTempFG2Location:) keyEquivalent:@""];
                item2.target = self.xrgGraphWindow;
                item2.representedObject = sensor.key;
                [[tempFG2Location menu] addItem:item2];

                NSMenuItem *item3 = [[NSMenuItem alloc] initWithTitle:sensor.label action:@selector(setTempFG3Location:) keyEquivalent:@""];
                item3.target = self.xrgGraphWindow;
                item3.representedObject = sensor.key;
                [[tempFG3Location menu] addItem:item3];
            }

			NSString *temp1Key = self.xrgGraphWindow.appSettings.tempFG1Location;
			NSString *temp2Key = self.xrgGraphWindow.appSettings.tempFG2Location;
			NSString *temp3Key = self.xrgGraphWindow.appSettings.tempFG3Location;
            NSInteger temp1Index = [locations indexOfObject:temp1Key];
            NSInteger temp2Index = [locations indexOfObject:temp2Key];
            NSInteger temp3Index = [locations indexOfObject:temp3Key];
			if (temp1Index < 0 | temp1Index >= numLocations) temp1Index = -1;
			if (temp2Index < 0 | temp2Index >= numLocations) temp2Index = -1;
			if (temp3Index < 0 | temp3Index >= numLocations) temp3Index = -1;
			
            [tempFG1Location selectItemAtIndex:temp1Index+1];
            [tempFG2Location selectItemAtIndex:temp2Index+1];
            [tempFG3Location selectItemAtIndex:temp3Index+1];
        }
        else {
            [tempFG1Location addItemWithTitle:@"No Sensors Found"];
            [tempFG2Location addItemWithTitle:@"No Sensors Found"];
            [tempFG3Location addItemWithTitle:@"No Sensors Found"];
        }
    }
    else {
        [tempFG1Location addItemWithTitle:@"No Sensors Found"];
        [tempFG2Location addItemWithTitle:@"No Sensors Found"];
        [tempFG3Location addItemWithTitle:@"No Sensors Found"];
    }
}

- (void)setUpNetworkPanel {
    // Setup net min graph
    [netMinGraphScaleUnits setTarget:self];
    [netMinGraphScaleValue setTarget:self];
    [netMinGraphScaleUnits setAction:@selector(setNetMinGraphUnitsAction:)];
    [netMinGraphScaleValue setAction:@selector(setNetMinGraphValueAction:)];
    
    NSString *s;
    NSInteger minByteScale = self.xrgGraphWindow.appSettings.netMinGraphScale;
    if (minByteScale < 1024) {
        [netMinGraphScaleUnits selectItemAtIndex:0];
        s = [[NSString alloc] initWithFormat: @"%ld", (long)minByteScale];
        [netMinGraphScaleValue setStringValue:s];
    }
    else if (minByteScale < 1048576) {
        [netMinGraphScaleUnits selectItemAtIndex:1];
        s = [[NSString alloc] initWithFormat: @"%ld", (long)(minByteScale / 1024)];
        [netMinGraphScaleValue setStringValue:s];
    }
    else {
        [netMinGraphScaleUnits selectItemAtIndex:2];
        s = [[NSString alloc] initWithFormat: @"%ld", (long)(minByteScale / 1048576)];
        [netMinGraphScaleValue setStringValue:s];
    }
    
    // Setup net graph mode
    [netGraphMode setTarget:self.xrgGraphWindow];
    [netGraphMode setAction:@selector(setNetGraphMode:)];
    [netGraphMode selectCellAtRow:self.xrgGraphWindow.appSettings.netGraphMode column:0];
    
    // Setup show total bandwidth
    [showTotalBandwidthSinceBoot setTarget:self.xrgGraphWindow];
    [showTotalBandwidthSinceBoot setAction:@selector(setShowTotalBandwidthSinceBoot:)];
	[showTotalBandwidthSinceBoot setState:self.xrgGraphWindow.appSettings.showTotalBandwidthSinceBoot ? NSControlStateValueOn : NSControlStateValueOff];
	
    [showTotalBandwidthSinceLoad setTarget:self.xrgGraphWindow];
    [showTotalBandwidthSinceLoad setAction:@selector(setShowTotalBandwidthSinceLoad:)];
	[showTotalBandwidthSinceLoad setState:self.xrgGraphWindow.appSettings.showTotalBandwidthSinceLoad ? NSControlStateValueOn : NSControlStateValueOff];
	
    // Setup network interface to monitor
    [networkInterface setTarget:self.xrgGraphWindow];
    [networkInterface setAction:@selector(setNetworkInterface:)];
    [networkInterface removeAllItems];
    [networkInterface addItemWithTitle:@"All Active"];
    NSArray *interfaces = [self.xrgGraphWindow.netView.miner networkInterfaces];
    NSString *selectedInterface = self.xrgGraphWindow.appSettings.networkInterface;
    int i;
    for (i = 0; i < [interfaces count]; i++) {
        [networkInterface addItemWithTitle:interfaces[i]];
        
        if ([selectedInterface isEqualToString:interfaces[i]])
            [networkInterface selectItemAtIndex:(i + 1)];
    }
}

- (void)setUpDiskPanel {
    // Setup disk graph mode
    [diskGraphMode setTarget:self.xrgGraphWindow];
    [diskGraphMode setAction:@selector(setDiskGraphMode:)];
    [diskGraphMode selectCellAtRow:self.xrgGraphWindow.appSettings.diskGraphMode column:0];
}

- (void)setUpWeatherPanel {
    NSInteger selection;
    
    // Setup ICAO
    [ICAOCode setTarget:self.xrgGraphWindow];
    [ICAOCode setAction:@selector(setICAO:)];
    if ([self.xrgGraphWindow.appSettings ICAO] != nil)
        [ICAOCode setStringValue:[self.xrgGraphWindow.appSettings ICAO]];
    else
        [ICAOCode setStringValue:@""];
        
    // Setup station list link
    NSString *htmlString = @"<a href=\"https://www.aviationweather.gov/docs/metar/stations.txt\" style=\"font-family: -apple-system\">Station Listing</a>";
	const char *cString = [htmlString cStringUsingEncoding:NSASCIIStringEncoding];
	[weatherStationListLink setAttributedTitle:[[NSAttributedString alloc] initWithHTML:[NSData dataWithBytes:cString length:strlen(cString)] documentAttributes:nil]];
    [weatherStationListLink setTarget:self];
    [weatherStationListLink setAction:@selector(openWeatherStationList:)];

    
    // Setup secondary weather graph
    [secondaryWeatherGraph setTarget:self.xrgGraphWindow];
    [secondaryWeatherGraph setAction:@selector(setSecondaryWeatherGraph:)];
    
    NSArray *items = [self.xrgGraphWindow.weatherView getSecondaryGraphList];
    [secondaryWeatherGraph removeAllItems];
    int i;
    for (i = 0; i < [items count]; i++) {
        [secondaryWeatherGraph addItemWithTitle: items[i]];
    }
    selection = self.xrgGraphWindow.appSettings.secondaryWeatherGraph;
    if (selection < 0 || selection >= [secondaryWeatherGraph numberOfItems])
        [secondaryWeatherGraph selectItemAtIndex:0];
    else
        [secondaryWeatherGraph selectItemAtIndex:selection];

    // Setup temperature units
    [temperatureUnits setTarget:self.xrgGraphWindow];
    [temperatureUnits setAction:@selector(setTemperatureUnits:)];
    
    [temperatureUnits removeAllItems];
    [temperatureUnits addItemWithTitle:@"Fahrenheit"];
    [temperatureUnits addItemWithTitle:@"Celsius"];
    
    selection = self.xrgGraphWindow.appSettings.temperatureUnits;
    if (selection < 0 || selection > 1)
        [temperatureUnits selectItemAtIndex:0];
    else
        [temperatureUnits selectItemAtIndex:selection];

    // Setup distance Units
    [distanceUnits setTarget:self.xrgGraphWindow];
    [distanceUnits setAction:@selector(setDistanceUnits:)];
    
    [distanceUnits removeAllItems];
    [distanceUnits addItemWithTitle:@"Miles"];
    [distanceUnits addItemWithTitle:@"Kilometers"];
    
    selection = self.xrgGraphWindow.appSettings.distanceUnits;
    if (selection < 0 || selection > 1) 
        [distanceUnits selectItemAtIndex:0];
    else
        [distanceUnits selectItemAtIndex:selection];
    
    // Setup pressure units
    [pressureUnits setTarget:self.xrgGraphWindow];
    [pressureUnits setAction:@selector(setPressureUnits:)];
    
    [pressureUnits removeAllItems];
    [pressureUnits addItemWithTitle:@"Inches"];
    [pressureUnits addItemWithTitle:@"Hectopascals"];
    
    selection = self.xrgGraphWindow.appSettings.pressureUnits;
    if (selection < 0 || selection > 1) 
        [pressureUnits selectItemAtIndex:0];
    else
        [pressureUnits selectItemAtIndex:selection];
}

- (void)setUpStockPanel {
    [stockSymbols setTarget:self.xrgGraphWindow];
    [stockSymbols setAction:@selector(setStockSymbols:)];
    if (self.xrgGraphWindow.appSettings.stockSymbols != nil)
        [stockSymbols setStringValue:self.xrgGraphWindow.appSettings.stockSymbols];
    else
        [stockSymbols setStringValue:@""];
        
    [stockGraphTimeFrame setTarget:self.xrgGraphWindow];
    [stockGraphTimeFrame setAction:@selector(setStockGraphTimeFrame:)];
    
    [stockGraphTimeFrame removeAllItems];
    [stockGraphTimeFrame addItemWithTitle:@"1 Month"];
    [stockGraphTimeFrame addItemWithTitle:@"3 Months"];
    [stockGraphTimeFrame addItemWithTitle:@"6 Months"];
    [stockGraphTimeFrame addItemWithTitle:@"12 Months"];
    
    NSInteger selection = self.xrgGraphWindow.appSettings.stockGraphTimeFrame;
    if (selection < 0 || selection > 3)
        [stockGraphTimeFrame selectItemAtIndex:0];
    else
        [stockGraphTimeFrame selectItemAtIndex:selection];
    
    [stockShowChange setTarget:self.xrgGraphWindow];
    [stockShowChange setAction:@selector(setStockShowChange:)];
	[stockShowChange setState:self.xrgGraphWindow.appSettings.stockShowChange ? NSControlStateValueOn : NSControlStateValueOff];
        
    [showDJIA setTarget:self.xrgGraphWindow];
    [showDJIA setAction:@selector(setShowDJIA:)];
	[showDJIA setState:self.xrgGraphWindow.appSettings.showDJIA ? NSControlStateValueOn : NSControlStateValueOff];
}

- (void)setUpAIPanel {
    // Register default values for AI Token settings
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSDictionary *defaults = @{
            @"aiTokensTrackingEnabled": @NO,
            @"showAITokenGraph": @NO,
            @"aiTokensDailyAutoReset": @YES,
            @"aiTokensDailyBudget": @0,
            @"aiTokensBudgetNotifyPercent": @80,
            @"aiTokensAggregateByModel": @YES,
            @"aiTokensAggregateByProvider": @NO,
            @"aiTokensShowRate": @YES,
            @"aiTokensShowBreakdown": @YES,
            @"showScanlines": @YES,
            @"showPixelGrid": @YES,
            @"showPixelDots": @YES
        };
        [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
    });

    // Create the AI preferences panel programmatically if it doesn't exist
    if (!AIPrefView) {
        AIPrefView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 500, 400)];

        CGFloat yPos = 350;
        CGFloat leftMargin = 20;
        CGFloat labelWidth = 200;
        CGFloat controlWidth = 250;

        // Title label
        NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 460, 24)];
        [titleLabel setStringValue:@"AI Token Tracking Settings"];
        [titleLabel setBezeled:NO];
        [titleLabel setDrawsBackground:NO];
        [titleLabel setEditable:NO];
        [titleLabel setSelectable:NO];
        [titleLabel setFont:[NSFont boldSystemFontOfSize:14]];
        [AIPrefView addSubview:titleLabel];

        yPos -= 40;

        // Enable tracking checkbox
        NSButton *trackingEnabled = [NSButton checkboxWithTitle:@"Enable AI Token Tracking" target:nil action:nil];
        [trackingEnabled setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [trackingEnabled setState:self.xrgGraphWindow.appSettings.aiTokensTrackingEnabled ? NSControlStateValueOn : NSControlStateValueOff];
        [trackingEnabled setTag:1000];
        [AIPrefView addSubview:trackingEnabled];

        yPos -= 30;

        // Daily auto-reset checkbox
        NSButton *dailyAutoReset = [NSButton checkboxWithTitle:@"Auto-reset daily counters at midnight" target:nil action:nil];
        [dailyAutoReset setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [dailyAutoReset setState:self.xrgGraphWindow.appSettings.aiTokensDailyAutoReset ? NSControlStateValueOn : NSControlStateValueOff];
        [dailyAutoReset setTag:1001];
        [AIPrefView addSubview:dailyAutoReset];

        yPos -= 40;

        // Daily budget
        NSTextField *budgetLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, labelWidth, 20)];
        [budgetLabel setStringValue:@"Daily token budget (0 = no limit):"];
        [budgetLabel setBezeled:NO];
        [budgetLabel setDrawsBackground:NO];
        [budgetLabel setEditable:NO];
        [budgetLabel setSelectable:NO];
        [AIPrefView addSubview:budgetLabel];

        NSTextField *budgetField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + labelWidth + 10, yPos, 80, 22)];
        [budgetField setIntegerValue:self.xrgGraphWindow.appSettings.aiTokensDailyBudget];
        [budgetField setTag:1002];
        [AIPrefView addSubview:budgetField];

        yPos -= 40;

        // Budget notify percent
        NSTextField *notifyLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, labelWidth, 20)];
        [notifyLabel setStringValue:@"Budget notification threshold (%):"];
        [notifyLabel setBezeled:NO];
        [notifyLabel setDrawsBackground:NO];
        [notifyLabel setEditable:NO];
        [notifyLabel setSelectable:NO];
        [AIPrefView addSubview:notifyLabel];

        NSTextField *notifyField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + labelWidth + 10, yPos, 80, 22)];
        [notifyField setIntegerValue:self.xrgGraphWindow.appSettings.aiTokensBudgetNotifyPercent];
        [notifyField setTag:1003];
        [AIPrefView addSubview:notifyField];

        yPos -= 40;

        // Aggregate by model checkbox
        NSButton *aggregateByModel = [NSButton checkboxWithTitle:@"Track usage by AI model" target:nil action:nil];
        [aggregateByModel setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [aggregateByModel setState:self.xrgGraphWindow.appSettings.aiTokensAggregateByModel ? NSControlStateValueOn : NSControlStateValueOff];
        [aggregateByModel setTag:1004];
        [AIPrefView addSubview:aggregateByModel];

        yPos -= 30;

        // Aggregate by provider checkbox
        NSButton *aggregateByProvider = [NSButton checkboxWithTitle:@"Track usage by AI provider" target:nil action:nil];
        [aggregateByProvider setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [aggregateByProvider setState:self.xrgGraphWindow.appSettings.aiTokensAggregateByProvider ? NSControlStateValueOn : NSControlStateValueOff];
        [aggregateByProvider setTag:1005];
        [AIPrefView addSubview:aggregateByProvider];

        yPos -= 30;

        // Show rate checkbox
        NSButton *showRate = [NSButton checkboxWithTitle:@"Show token rate (tokens/second)" target:nil action:nil];
        [showRate setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [showRate setState:self.xrgGraphWindow.appSettings.aiTokensShowRate ? NSControlStateValueOn : NSControlStateValueOff];
        [showRate setTag:1006];
        [AIPrefView addSubview:showRate];

        yPos -= 30;

        // Show breakdown checkbox
        NSButton *showBreakdown = [NSButton checkboxWithTitle:@"Show detailed breakdown" target:nil action:nil];
        [showBreakdown setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [showBreakdown setState:self.xrgGraphWindow.appSettings.aiTokensShowBreakdown ? NSControlStateValueOn : NSControlStateValueOff];
        [showBreakdown setTag:1007];
        [AIPrefView addSubview:showBreakdown];

        yPos -= 40;

        // Cyberpunk visual effects section
        NSTextField *effectsLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 460, 24)];
        [effectsLabel setStringValue:@"Cyberpunk Visual Effects"];
        [effectsLabel setBezeled:NO];
        [effectsLabel setDrawsBackground:NO];
        [effectsLabel setEditable:NO];
        [effectsLabel setSelectable:NO];
        [effectsLabel setFont:[NSFont boldSystemFontOfSize:14]];
        [AIPrefView addSubview:effectsLabel];

        yPos -= 30;

        // Scanlines checkbox
        NSButton *showScanlines = [NSButton checkboxWithTitle:@"Show CRT scanlines" target:nil action:nil];
        [showScanlines setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [showScanlines setState:self.xrgGraphWindow.appSettings.showScanlines ? NSControlStateValueOn : NSControlStateValueOff];
        [showScanlines setTag:1010];
        [AIPrefView addSubview:showScanlines];

        yPos -= 30;

        // Pixel grid checkbox
        NSButton *showPixelGrid = [NSButton checkboxWithTitle:@"Show pixel grid background" target:nil action:nil];
        [showPixelGrid setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [showPixelGrid setState:self.xrgGraphWindow.appSettings.showPixelGrid ? NSControlStateValueOn : NSControlStateValueOff];
        [showPixelGrid setTag:1011];
        [AIPrefView addSubview:showPixelGrid];

        yPos -= 30;

        // Pixel dots checkbox
        NSButton *showPixelDots = [NSButton checkboxWithTitle:@"Show pixel dots on graphs" target:nil action:nil];
        [showPixelDots setFrame:NSMakeRect(leftMargin, yPos, controlWidth, 20)];
        [showPixelDots setState:self.xrgGraphWindow.appSettings.showPixelDots ? NSControlStateValueOn : NSControlStateValueOff];
        [showPixelDots setTag:1012];
        [AIPrefView addSubview:showPixelDots];

        yPos -= 50;

        // Reset buttons
        NSButton *resetSessionBtn = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 150, 28)];
        [resetSessionBtn setTitle:@"Reset Session"];
        [resetSessionBtn setBezelStyle:NSBezelStyleRounded];
        [resetSessionBtn setTarget:self];
        [resetSessionBtn setAction:@selector(resetSessionCounters:)];
        [AIPrefView addSubview:resetSessionBtn];

        NSButton *resetDailyBtn = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin + 160, yPos, 150, 28)];
        [resetDailyBtn setTitle:@"Reset Daily"];
        [resetDailyBtn setBezelStyle:NSBezelStyleRounded];
        [resetDailyBtn setTarget:self];
        [resetDailyBtn setAction:@selector(resetDailyCounters:)];
        [AIPrefView addSubview:resetDailyBtn];
    }
}

- (IBAction)resetSessionCounters:(id)sender {
    [[NSNotificationCenter defaultCenter] postNotificationName:@"XRGAITokensDidResetSession" object:self];
}

- (IBAction)resetDailyCounters:(id)sender {
    [[NSNotificationCenter defaultCenter] postNotificationName:@"XRGAITokensDidResetDaily" object:self];
}

- (IBAction)loadTheme:(id)sender {                   
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];

    [oPanel setAllowsMultipleSelection:NO];
    [oPanel setCanChooseFiles:YES];
    [oPanel setAllowedFileTypes:@[@"xtf"]];
    [oPanel setDirectoryURL:[NSURL fileURLWithPath:NSHomeDirectory()]];
    
    [oPanel beginSheetModalForWindow:window completionHandler:^(NSInteger returnCode) {
        if (returnCode == NSModalResponseOK) {
            NSArray *filenames = [oPanel URLs];
            NSURL *path = filenames[0];

            NSData *themeData = [NSData dataWithContentsOfURL:path];
            
            if ([themeData length] == 0) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert setMessageText:@"Error"];
                [alert setInformativeText:@"The theme file specified is not a valid theme file."];
                [alert addButtonWithTitle:@"Okay"];
                [alert runModal];
                return;
            }

            NSError *error = nil;
            NSPropertyListFormat format;
            NSDictionary *themeDictionary = [NSPropertyListSerialization propertyListWithData:themeData
                                                                                       options:NSPropertyListImmutable
                                                                                        format:&format
                                                                                         error:&error];
                                                               
            if (!themeDictionary) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert setMessageText:@"Error"];
                [alert setInformativeText:@"The theme file specified is not a valid theme file."];
                [alert addButtonWithTitle:@"Okay"];
                [alert runModal];
                NSLog(@"%@", [error localizedDescription]);
            }
            else {
                @try {
                    NSData *d = themeDictionary[XRG_backgroundColor];
                    [self->backgroundColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    d = themeDictionary[XRG_graphBGColor];
                    [self->graphBGColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    d = themeDictionary[XRG_graphFG1Color];
                    [self->graphFG1ColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    d = themeDictionary[XRG_graphFG2Color];
                    [self->graphFG2ColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    d = themeDictionary[XRG_graphFG3Color];
                    [self->graphFG3ColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    d = themeDictionary[XRG_borderColor];
                    [self->borderColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    d = themeDictionary[XRG_textColor];
                    [self->textColorWell setColor:[NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class] fromData:d error:nil]];
                    
                    NSNumber *n = (NSNumber *)themeDictionary[XRG_backgroundTransparency];
                    [self->backgroundTransparency setFloatValue: [n floatValue]];
                    
                    n = (NSNumber *)themeDictionary[XRG_graphBGTransparency];
                    [self->graphBGTransparency setFloatValue:    [n floatValue]];
                    
                    n = (NSNumber *)themeDictionary[XRG_graphFG1Transparency];
                    [self->graphFG1Transparency setFloatValue:   [n floatValue]];
                    
                    n = (NSNumber *)themeDictionary[XRG_graphFG2Transparency];
                    [self->graphFG2Transparency setFloatValue:   [n floatValue]];
                    
                    n = (NSNumber *)themeDictionary[XRG_graphFG3Transparency];
                    [self->graphFG3Transparency setFloatValue:   [n floatValue]];
                    
                    n = (NSNumber *)themeDictionary[XRG_borderTransparency];
                    [self->borderTransparency setFloatValue:     [n floatValue]];
                    
                    n = (NSNumber *)themeDictionary[XRG_textTransparency];
                    [self->textTransparency setFloatValue:       [n floatValue]];
                    
                    [self.xrgGraphWindow setObjectsToColor:self->backgroundColorWell];
                    [self.xrgGraphWindow setObjectsToColor:self->graphBGColorWell];
                    [self.xrgGraphWindow setObjectsToColor:self->graphFG1ColorWell];
                    [self.xrgGraphWindow setObjectsToColor:self->graphFG2ColorWell];
                    [self.xrgGraphWindow setObjectsToColor:self->graphFG3ColorWell];
                    [self.xrgGraphWindow setObjectsToColor:self->borderColorWell];
                    [self.xrgGraphWindow setObjectsToColor:self->textColorWell];
        
                    [self.xrgGraphWindow setObjectsToTransparency:self->backgroundTransparency];
                    [self.xrgGraphWindow setObjectsToTransparency:self->graphBGTransparency];
                    [self.xrgGraphWindow setObjectsToTransparency:self->graphFG1Transparency];
                    [self.xrgGraphWindow setObjectsToTransparency:self->graphFG2Transparency];
                    [self.xrgGraphWindow setObjectsToTransparency:self->graphFG3Transparency];
                    [self.xrgGraphWindow setObjectsToTransparency:self->borderTransparency];
                    [self.xrgGraphWindow setObjectsToTransparency:self->textTransparency];
                } @catch (NSException *e) {
                    NSAlert *alert = [[NSAlert alloc] init];
                    [alert setMessageText:@"Error"];
                    [alert setInformativeText:@"The theme file specified is not a valid theme file."];
                    [alert addButtonWithTitle:@"Okay"];
                    [alert runModal];
                }
            }
        }
    }];
}

- (IBAction)saveTheme:(id)sender {
    NSSavePanel *sp = [NSSavePanel savePanel];
    
    [sp setAllowedFileTypes:@[@"xtf"]];
    [sp setDirectoryURL:[NSURL fileURLWithPath:NSHomeDirectory()]];
    [sp setNameFieldStringValue:@"My Theme.xtf"];
    
    [sp beginSheetModalForWindow:window completionHandler:^(NSInteger returnCode) {
        if (returnCode == NSModalResponseOK) {
            NSURL *path = [sp URL];
            
            // Create the property dictionary
            NSMutableDictionary *colorPrefs = [NSMutableDictionary dictionary];

            colorPrefs[XRG_backgroundTransparency] = @([self->backgroundTransparency floatValue]);
            colorPrefs[XRG_graphBGTransparency] = @([self->graphBGTransparency floatValue]);
            colorPrefs[XRG_graphFG1Transparency] = @([self->graphFG1Transparency floatValue]);
            colorPrefs[XRG_graphFG2Transparency] = @([self->graphFG2Transparency floatValue]);
            colorPrefs[XRG_graphFG3Transparency] = @([self->graphFG3Transparency floatValue]);
            colorPrefs[XRG_borderTransparency] = @([self->borderTransparency floatValue]);
            colorPrefs[XRG_textTransparency] = @([self->textTransparency floatValue]);

            colorPrefs[XRG_backgroundColor] = [NSKeyedArchiver archivedDataWithRootObject:[self->backgroundColorWell color] requiringSecureCoding:NO error:nil];
            colorPrefs[XRG_graphBGColor] = [NSKeyedArchiver archivedDataWithRootObject:[self->graphBGColorWell color] requiringSecureCoding:NO error:nil];
            colorPrefs[XRG_graphFG1Color] = [NSKeyedArchiver archivedDataWithRootObject:[self->graphFG1ColorWell color] requiringSecureCoding:NO error:nil];
            colorPrefs[XRG_graphFG2Color] = [NSKeyedArchiver archivedDataWithRootObject:[self->graphFG2ColorWell color] requiringSecureCoding:NO error:nil];
            colorPrefs[XRG_graphFG3Color] = [NSKeyedArchiver archivedDataWithRootObject:[self->graphFG3ColorWell color] requiringSecureCoding:NO error:nil];
            colorPrefs[XRG_borderColor] = [NSKeyedArchiver archivedDataWithRootObject:[self->borderColorWell color] requiringSecureCoding:NO error:nil];
            colorPrefs[XRG_textColor] = [NSKeyedArchiver archivedDataWithRootObject:[self->textColorWell color] requiringSecureCoding:NO error:nil];
            
            NSError *error = nil;
            NSData *xmlData = [NSPropertyListSerialization dataWithPropertyList:colorPrefs
                                                                         format:NSPropertyListXMLFormat_v1_0
                                                                        options:0
                                                                          error:&error];
            if (xmlData) {
                if (![xmlData writeToURL:path atomically:YES]) {
                    NSAlert *alert = [[NSAlert alloc] init];
                    [alert setMessageText:@"Error"];
                    [alert setInformativeText:@"Could not save the theme to that location."];
                    [alert addButtonWithTitle:@"Okay"];
                    [alert runModal];
                }
            }
            else {
                NSLog(@"%@", [error localizedDescription]);
            }
        }
    }];
}

- (NSWindow *)window {
    return window;
}

- (void)setUpWell:(NSColorWell *)well withTransparency:(NSSlider *)tSlider {
    [well setTarget:self.xrgGraphWindow];
    [well setAction:@selector(setObjectsToColor:)];
    [well setColor:[self.xrgGraphWindow colorForTag:[well tag]]];
    
    [tSlider setTarget:self.xrgGraphWindow];
    [tSlider setAction:@selector(setObjectsToTransparency:)];
    [tSlider setFloatValue:[self.xrgGraphWindow transparencyForTag:[tSlider tag]]];
}

- (IBAction)setGraphRefreshAction:(id)sender {
    float ref = [sender floatValue];
	ref = roundf(ref * 5.) * 0.2;
	
    NSString *s;
    if (roundf(ref * 10.) == 10) 
        s = @"Graph updates every second";
    else
        s = [[NSString alloc] initWithFormat: @"Graph updates every %2.1f seconds", ref];
    
    [graphRefreshText setStringValue:s];

    [self.xrgGraphWindow setGraphRefreshActionPart2:sender];
}

- (void)setUpModuleSelection {
}

- (IBAction)setNetMinGraphUnitsAction:(id)sender {
    int sInt = [[netMinGraphScaleValue stringValue] intValue];
    if (sInt == INT_MAX || sInt == INT_MIN) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Error"];
        [alert setInformativeText:@"The minimum network scale must be a number."];
        [alert addButtonWithTitle:@"OK"];
        [alert beginSheetModalForWindow:window completionHandler:nil];
        [netMinGraphScaleValue setStringValue:@"0"];
        return;
    }
    if (sInt == 0) {
        if (![[netMinGraphScaleValue stringValue] isEqualToString:@"0"]) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Error"];
            [alert setInformativeText:@"The minimum network scale must be a number."];
            [alert addButtonWithTitle:@"OK"];
            [alert beginSheetModalForWindow:window completionHandler:nil];
            [netMinGraphScaleValue setStringValue:@"0"];
            return;
        }
    }

    if ([sender indexOfSelectedItem] == 0)
        [self.xrgGraphWindow.appSettings setNetMinGraphScale:sInt];
    else if ([sender indexOfSelectedItem] == 1) 
        [self.xrgGraphWindow.appSettings setNetMinGraphScale:sInt * 1024];
    else
        [self.xrgGraphWindow.appSettings setNetMinGraphScale:sInt * 1048576];
}

- (IBAction)setNetMinGraphValueAction:(id)sender {
    NSString *s = [sender stringValue];
    int sInt = [s intValue];
    if (sInt == INT_MAX || sInt == INT_MIN) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Error"];
        [alert setInformativeText:@"The minimum network scale must be a number."];
        [alert addButtonWithTitle:@"OK"];
        [alert beginSheetModalForWindow:window completionHandler:nil];
        [netMinGraphScaleValue setStringValue:@"0"];
        return;
    }
    if (sInt == 0) {
        if (![s isEqualToString:@"0"]) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Error"];
            [alert setInformativeText:@"The minimum network scale must be a number."];
            [alert addButtonWithTitle:@"OK"];
            [alert beginSheetModalForWindow:window completionHandler:nil];
            [netMinGraphScaleValue setStringValue:@"0"];
            return;
        }
    }
    
    if ([netMinGraphScaleUnits indexOfSelectedItem] == 0)
        [self.xrgGraphWindow.appSettings setNetMinGraphScale:sInt];
    else if ([netMinGraphScaleUnits indexOfSelectedItem] == 1) 
        [self.xrgGraphWindow.appSettings setNetMinGraphScale:sInt * 1024];
    else
        [self.xrgGraphWindow.appSettings setNetMinGraphScale:sInt * 1048576];
}

- (NSColorWell *)colorWellForTag:(int)aTag {
    return [[window contentView] viewWithTag:aTag];
}

- (IBAction)setFont:(id)sender {
    [[NSFontPanel sharedFontPanel] makeKeyAndOrderFront:self];
    [[NSFontManager sharedFontManager] setSelectedFont:self.xrgGraphWindow.appSettings.graphFont isMultiple:NO];
}

// Here are the action methods for the toolbar buttons.

// This method will change the window size from one view to another and display the new view.
-(void) switchWindowFromView:oldView toView:newView {
    NSRect newWindowSize = [newView frame];
    newWindowSize.origin.x = [window frame].origin.x;
    newWindowSize.origin.y = [window frame].origin.y + [oldView frame].size.height - newWindowSize.size.height;
    newWindowSize.size.width = MAX(newWindowSize.size.width, 500);
    newWindowSize.size.height = [window frame].size.height - [oldView frame].size.height + newWindowSize.size.height;

    NSRect oldViewSize = [oldView frame];
    
    [oldView removeFromSuperview];
    [window setFrame:newWindowSize display:YES animate:YES];
    [window setContentView:newView];
    [oldView setFrameSize:oldViewSize.size];
}

-(IBAction) General:(id)sender {
    if (currentView != GeneralPrefView) {
        [self switchWindowFromView:currentView toView:GeneralPrefView];
        currentView = GeneralPrefView;
		[window setTitle:@"General Preferences"];
		[toolbar setSelectedItemIdentifier:@"General"];
    }
}

-(IBAction) Colors:(id)sender {
    if (currentView != ColorPrefView) {
        [self switchWindowFromView:currentView toView:ColorPrefView];
        currentView = ColorPrefView;
		[window setTitle:@"Appearance Preferences"];
		[toolbar setSelectedItemIdentifier:@"Appearance"];
    }
}

-(IBAction) CPU:(id)sender {
    if (currentView != CPUPrefView) {
        [self switchWindowFromView:currentView toView:CPUPrefView];
        currentView = CPUPrefView;
		[window setTitle:@"CPU Preferences"];
		[toolbar setSelectedItemIdentifier:@"CPU"];
    }
}

-(IBAction) RAM:(id)sender {
    if (currentView != MemoryPrefView) {
        [self switchWindowFromView:currentView toView:MemoryPrefView];
        currentView = MemoryPrefView;
 		[window setTitle:@"Memory Preferences"];
		[toolbar setSelectedItemIdentifier:@"RAM"];
   }
}

-(IBAction) Temperature:(id)sender {
    if (currentView != TemperaturePrefView) {
        [self switchWindowFromView:currentView toView:TemperaturePrefView];
        currentView = TemperaturePrefView;
		[window setTitle:@"Temperature Preferences"];
		[toolbar setSelectedItemIdentifier:@"Temperature"];
    }
}

-(IBAction) Network:(id)sender {
    if (currentView != NetworkPrefView) {
        [self switchWindowFromView:currentView toView:NetworkPrefView];
        currentView = NetworkPrefView;
		[window setTitle:@"Network Preferences"];
		[toolbar setSelectedItemIdentifier:@"Network"];
    }
}

-(IBAction) Disk:(id)sender {
    if (currentView != DiskPrefView) {
        [self switchWindowFromView:currentView toView:DiskPrefView];
        currentView = DiskPrefView;
		[window setTitle:@"Disk Preferences"];
		[toolbar setSelectedItemIdentifier:@"Disk"];
    }
}

-(IBAction) Weather:(id)sender {
    if (currentView != WeatherPrefView) {
        [self switchWindowFromView:currentView toView:WeatherPrefView];
        currentView = WeatherPrefView;
		[window setTitle:@"Weather Preferences"];
		[toolbar setSelectedItemIdentifier:@"Weather"];
    }
}

-(IBAction) Stocks:(id)sender {
    if (currentView != StockPrefView) {
        [self switchWindowFromView:currentView toView:StockPrefView];
        currentView = StockPrefView;
		[window setTitle:@"Stock Preferences"];
 		[toolbar setSelectedItemIdentifier:@"Stock"];
	}
}

-(IBAction) AI:(id)sender {
    if (currentView != AIPrefView) {
        [self switchWindowFromView:currentView toView:AIPrefView];
        currentView = AIPrefView;
        [window setTitle:@"AI Token Preferences"];
        [toolbar setSelectedItemIdentifier:@"AI"];
    }
}

-(IBAction) openWeatherStationList:(id)sender {
    [NSTask 
        launchedTaskWithLaunchPath:@"/usr/bin/open"
        arguments:@[@"https://download.gauchosoft.com/xrg/stations.txt"]
    ];
}

// These last methods are required for the toolbar

// This method is required of NSToolbar delegates.  It takes an identifier, and returns the matching NSToolbarItem.
// It also takes a parameter telling whether this toolbar item is going into an actual toolbar, or whether it's
// going to be displayed in a customization palette.
- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
    // We create and autorelease a new NSToolbarItem, and then go through the process of setting up its
    // attributes from the master toolbar item matching that identifier in our dictionary of items.
    NSToolbarItem *newItem = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
    NSToolbarItem *item=toolbarItems[itemIdentifier];
    
    [newItem setLabel:[item label]];
    [newItem setPaletteLabel:[item paletteLabel]];
    if ([item view] != NULL) {
	[newItem setView:[item view]];
    }
    else {
	[newItem setImage:[item image]];
    }
    [newItem setToolTip:[item toolTip]];
    [newItem setTarget:[item target]];
    [newItem setAction:[item action]];
    [newItem setMenuFormRepresentation:[item menuFormRepresentation]];
    // If we have a custom view, we *have* to set the min/max size - otherwise, it'll default to 0,0 and the custom
    // view won't show up at all!  This doesn't affect toolbar items with images, however.
    if ([newItem view] != NULL) {
	[newItem setMinSize:[[item view] bounds].size];
	[newItem setMaxSize:[[item view] bounds].size];
    }

    return newItem;
}

// This method is required of NSToolbar delegates.  It returns an array holding identifiers for the default
// set of toolbar items.  It can also be called by the customization palette to display the default toolbar.    
- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar*)toolbar {
    return @[@"General", @"Appearance", @"CPU", @"RAM", @"Temperature", @"Network", @"Disk", @"Weather", @"Stocks", @"AI"];
}

// This method is required of NSToolbar delegates.  It returns an array holding identifiers for all allowed
// toolbar items in this toolbar.  Any not listed here will not be available in the customization palette.
- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar*)toolbar {
    return @[@"General", @"Appearance", @"CPU", @"RAM", @"Temperature", @"Network", @"Disk", @"Weather", @"Stocks", @"AI"];
}

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar*)toolbar {
    return @[@"General", @"Appearance", @"CPU", @"RAM", @"Temperature", @"Network", @"Disk", @"Weather", @"Stocks", @"AI"];
}

- (void) windowWillClose:(NSNotification *)aNotification {
	[self save:self];
}

@end



