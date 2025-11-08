# AI Token Traffic Implementation for XRG

## Overview
Added a new AI Token Traffic monitoring module to XRG that tracks token usage from Claude Code, OpenAI Codex, and other AI services.

## Files Created

### 1. Data Miner (Token Collection)
- **Location**: `Data Miners/XRGAITokenMiner.h`
- **Location**: `Data Miners/XRGAITokenMiner.m`
- **Purpose**: Collects token usage data from AI service log files
- **Features**:
  - Monitors Claude Code logs at `~/.claude/logs/claude-code.log`
  - Monitors Codex logs at `~/.config/codex/usage.log`
  - Tracks cumulative token counts
  - Calculates tokens per second rates
  - Stores data in XRGDataSet for graphing

### 2. Graph View (Token Visualization)
- **Location**: `Graph Views/XRGAITokenView.h`
- **Location**: `Graph Views/XRGAITokenView.m`
- **Purpose**: Displays token usage graphs and statistics
- **Features**:
  - Stacked area graphs for Claude, Codex, and Other services
  - Real-time rate indicator on right side
  - Text labels showing current rates and totals
  - Supports mini-graph mode for small views
  - Color-coded: FG1 (Claude), FG2 (Codex), FG3 (Other)

### 3. Configuration Updates
- **Location**: `Other Sources/definitions.h`
- **Changes**:
  - Added `XRG_showAITokenGraph` preference key
  - Added `XRG_aiTokenClaudeLogPath` for custom Claude log path
  - Added `XRG_aiTokenCodexLogPath` for custom Codex log path

### 4. Window Controller Integration
- **Location**: `Controllers/XRGGraphWindow.h`
- **Changes**:
  - Added `#import "XRGAITokenView.h"`
  - Added `@property XRGAITokenView *aiTokenView`
  - Added `- (IBAction)setShowAITokenGraph:(id)sender`

- **Location**: `Controllers/XRGGraphWindow.m`
- **Changes**:
  - Added `setShowAITokenGraph:` implementation

## Next Steps

### 1. Add Files to Xcode Project ⚠️ REQUIRED

You need to add the new files to the Xcode project:

1. Open `XRG.xcodeproj` in Xcode
2. Right-click on "Data Miners" group → Add Files to "XRG"
   - Select `XRGAITokenMiner.h` and `XRGAITokenMiner.m`
   - Ensure "Copy items if needed" is unchecked (files are already in place)
   - Ensure target "XRG" is checked
3. Right-click on "Graph Views" group → Add Files to "XRG"
   - Select `XRGAITokenView.h` and `XRGAITokenView.m`
   - Same settings as above
4. Build the project to verify no errors

### 2. Update NIB Files (Interface Builder)

#### MainMenu.nib
Add the AI Token view to the main window:

1. Open `Resources/MainMenu.nib` in Interface Builder
2. Find the main XRG window
3. Add a new Custom View
4. Set the Custom Class to `XRGAITokenView`
5. Connect the outlet:
   - Control-drag from "File's Owner" (XRGGraphWindow) to the new view
   - Select `aiTokenView` outlet

#### Preferences.nib
Add preference controls:

1. Open `Resources/Preferences.nib` in Interface Builder
2. Find the "Graphs" preference panel
3. Add a new checkbox: "Show AI Tokens"
4. Connect the checkbox:
   - Action: Control-drag to "File's Owner" → select `setShowAITokenGraph:`
   - Binding: Bind "value" to NSUserDefaultsController → values.showAITokenGraph

### 3. Optional: Add Preference Controls for Log Paths

To allow users to customize log file locations, add to `XRGPrefController`:

```objc
// In XRGPrefController.h
@property IBOutlet NSTextField *claudeLogPathField;
@property IBOutlet NSTextField *codexLogPathField;

// In XRGPrefController.m
- (IBAction)setClaudeLogPath:(id)sender {
    NSString *path = [sender stringValue];
    [[NSUserDefaults standardUserDefaults] setObject:path forKey:XRG_aiTokenClaudeLogPath];
}

- (IBAction)setCodexLogPath:(id)sender {
    NSString *path = [sender stringValue];
    [[NSUserDefaults standardUserDefaults] setObject:path forKey:XRG_aiTokenCodexLogPath];
}
```

Add text fields in Preferences.nib and connect them.

## How It Works

### Data Flow
1. **Timer fires** (every second via `graphTimer`)
2. **XRGModuleManager** calls `[aiTokenView graphUpdate:]`
3. **View** calls `[tokenMiner getLatestTokenInfo]`
4. **Miner** reads new lines from log files
5. **Miner** parses token counts using regex patterns
6. **Miner** calculates rate (tokens since last update)
7. **Miner** stores rate in XRGDataSet
8. **View** calls `setNeedsDisplay:YES`
9. **View's drawRect** renders the graph

### Log Parsing

The miner uses regex patterns to extract token counts:

**Claude Code logs**: Looks for patterns like:
- `"input=1234, output=567"`
- `"prompt: 1234, completion: 567"`

**Codex logs**: Looks for patterns like:
- `"tokens: 1234"`
- `"token count: 5678"`

### Graph Display

The view creates a **stacked area graph**:
- Bottom layer (blue/FG1): Claude Code tokens
- Middle layer (green/FG2): Codex tokens
- Top layer (red/FG3): Other AI tokens

Plus a **real-time indicator** bar on the right showing current proportions.

## Testing

### 1. Build and Run
```bash
cd /Volumes/FILES/code/XRG
xcodebuild -project XRG.xcodeproj -scheme XRG build
```

### 2. Enable the Module
1. Launch XRG
2. Open Preferences → Graphs
3. Check "Show AI Tokens"
4. The graph should appear in the main window

### 3. Generate Token Traffic
To test with real data:
```bash
# Use Claude Code to generate token traffic
claude-code "write a hello world program"

# Check if logs are being created
ls -la ~/.claude/logs/
tail -f ~/.claude/logs/claude-code.log
```

### 4. Verify Display
- The graph should show token rates over time
- Text labels should show current rates (e.g., "Claude: 1234 tok/s")
- Total tokens should accumulate
- Colors should match other XRG graphs (customizable in prefs)

## Default Log Locations

The module expects logs at:
- **Claude Code**: `~/.claude/logs/claude-code.log`
- **Codex**: `~/.config/codex/usage.log`

If your logs are elsewhere, you'll need to:
1. Add the preference UI (see step 3 above)
2. Update `XRGAITokenMiner.m` init method with your paths

## Customization

### Adjusting Display Order
In `XRGAITokenView.m`, change `m.displayOrder`:
```objc
m.displayOrder = 11;  // Display after other graphs
```

Lower numbers appear first in the window.

### Changing Colors
The module uses standard XRG color preferences:
- FG1 Color (Blue) → Claude Code
- FG2 Color (Green) → Codex
- FG3 Color (Red) → Other services

Users can customize these in Preferences → Appearance.

### Graph Scale
The graph auto-scales based on max token rate.
Default minimum scale: 1000 tokens/sec

## Architecture Notes

This module follows XRG's standard three-layer pattern:

1. **XRGAITokenMiner** (Data Layer)
   - Inherits from NSObject
   - Manages XRGDataSet ring buffers
   - Polls log files periodically
   - Calculates rates

2. **XRGAITokenView** (Presentation Layer)
   - Inherits from XRGGenericView
   - Owns the miner instance
   - Handles drawing and layout
   - Responds to timer events

3. **XRGModule** (Registration)
   - Created in view's `awakeFromNib`
   - Registered with XRGModuleManager
   - Manages visibility, size, and ordering

## Known Limitations

1. **Log Format Dependency**: Assumes Claude Code and Codex write token counts to log files. If they don't, no data will appear.

2. **No Real-time API**: This implementation monitors logs, not live API responses. There's ~1 second lag.

3. **Single Log File**: Each service monitors one log file. If multiple log files exist, only one is tracked.

4. **No Historical Data**: Token counts start at zero when XRG launches. No persistent storage.

## Future Enhancements

1. **API Integration**: Query Claude/OpenAI APIs directly for usage stats
2. **Cost Tracking**: Convert tokens to estimated costs
3. **Multiple Services**: Add support for GPT-4, Gemini, etc.
4. **Alerts**: Notify when token usage exceeds thresholds
5. **Export**: Save token usage history to CSV
6. **Rate Limiting**: Visual indicator when approaching rate limits

## Troubleshooting

### Graph shows no data
- Check that log files exist at expected locations
- Verify log files contain token usage information
- Check console for NSLog output (if XRG_DEBUG is defined)
- Ensure the checkbox is enabled in preferences

### Build errors
- Verify all files are added to Xcode project target
- Check that imports are correct
- Clean build folder (Product → Clean Build Folder)

### Crashes
- Check that aiTokenView outlet is connected in NIB
- Verify tokenMiner is properly initialized
- Look for null pointer dereferences in console

## Contact

For issues or questions about this implementation:
- Check XRG documentation at https://gauchosoft.com/xrg
- Review XRG source code for similar modules (e.g., XRGMemoryView)
