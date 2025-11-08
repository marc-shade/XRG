# Quick Start: AI Token Monitoring in XRG

## What This Does

XRG now monitors AI API token usage from:
- **Claude Code** - Your AI coding assistant
- **OpenAI Codex** - OpenAI's code model
- **Other AI Services** - Extensible for future services

## How to Use

### 1. Build XRG

```bash
cd /Volumes/FILES/code/XRG
xcodebuild -project XRG.xcodeproj -scheme XRG build
```

Or open in Xcode and click Build (⌘B).

### 2. Launch XRG

```bash
open build/Debug/XRG.app
```

Or launch from Xcode with ⌘R.

### 3. Enable AI Token Monitoring

1. Click the **XRG** menu → **Preferences**
2. Go to the **Graphs** tab
3. Check the box: **☑ Show AI Tokens**
4. Close Preferences

### 4. Position the Graph

The AI Token graph will appear in your XRG window. You can:
- **Drag** to reorder it among other graphs
- **Resize** the window to see more history
- **Click** to expand/collapse

## Understanding the Display

### Graph Colors
- **Blue (FG1)**: Claude Code tokens
- **Green (FG2)**: OpenAI Codex tokens
- **Red (FG3)**: Other AI services

### Real-time Indicator
The vertical bar on the right shows current proportions.

### Text Labels
```
Claude: 1234 tok/s    ← Current rate
Codex: 567 tok/s      ← Current rate
Total: 123K           ← Cumulative total
```

## Generating Token Traffic

### Method 1: Use Claude Code
```bash
# In terminal
claude-code "write a hello world program in Python"
```

Watch the XRG graph update in real-time!

### Method 2: Use Codex (if installed)
```bash
codex "explain this code"
```

### Method 3: Natural Usage
Just use your AI tools normally. XRG monitors in the background.

## Configuration

### Default Log Locations

XRG looks for token data here:
- Claude Code: `~/.claude/logs/claude-code.log`
- Codex: `~/.config/codex/usage.log`

### Custom Log Paths

If your logs are elsewhere:

1. Open `~/Library/Preferences/com.gauchosoft.XRG.plist`
2. Add these keys:
   ```xml
   <key>aiTokenClaudeLogPath</key>
   <string>/your/custom/path/claude.log</string>

   <key>aiTokenCodexLogPath</key>
   <string>/your/custom/path/codex.log</string>
   ```
3. Restart XRG

## Customization

### Change Colors

1. Preferences → **Appearance**
2. Modify:
   - **FG1 Color** (Claude - default blue)
   - **FG2 Color** (Codex - default green)
   - **FG3 Color** (Other - default red)

### Adjust Update Frequency

Edit `XRGGraphWindow.m`:
```objc
// Default: 1 second
- (void)initTimers {
    self.graphTimer = [NSTimer scheduledTimerWithTimeInterval:1.0  // ← Change this
```

### Change Display Order

Edit `XRGAITokenView.m`:
```objc
- (void)awakeFromNib {
    // ...
    m.displayOrder = 11;  // Lower = appears first
```

## Troubleshooting

### Graph Shows Zero
**Problem**: No token data appears

**Solutions**:
1. Verify log files exist:
   ```bash
   ls -la ~/.claude/logs/
   ls -la ~/.config/codex/
   ```

2. Check log format:
   ```bash
   tail -20 ~/.claude/logs/claude-code.log
   ```

   Should contain patterns like:
   - `input=1234, output=567`
   - `tokens: 1234`

3. Generate some traffic:
   ```bash
   claude-code "test"
   ```

### Build Errors

**Problem**: Xcode says "file not found"

**Solution**: Clean build folder
```bash
cd /Volumes/FILES/code/XRG
xcodebuild clean
xcodebuild build
```

### Signing Certificate Error

**Problem**: "No signing certificate found"

**Solution**: Build without signing
```bash
xcodebuild -project XRG.xcodeproj -scheme XRG \
    CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO build
```

Or in Xcode:
1. Select project in Navigator
2. Build Settings → Signing
3. Set **Code Signing Identity** to "Don't Code Sign"

## Advanced: Log Format

### Claude Code Expected Format

XRG parses logs with regex: `(?:input|prompt).*?(\d+).*?(?:output|completion).*?(\d+)`

Examples that work:
```
2025-01-01 12:00:00 - Token usage: input=1234, output=567
2025-01-01 12:00:01 - Prompt tokens: 1234, Completion tokens: 567
```

### Codex Expected Format

XRG parses logs with regex: `tokens?[:\s]+(\d+)`

Examples that work:
```
2025-01-01 12:00:00 - Total tokens: 1234
2025-01-01 12:00:01 - Token count: 5678
```

### Custom Log Format

To support different formats, edit `XRGAITokenMiner.m`:

```objc
- (void)parseTokensFromClaudeLog:(NSString *)logContent {
    NSRegularExpression *regex = [NSRegularExpression
        regularExpressionWithPattern:@"YOUR_PATTERN_HERE"
        options:NSRegularExpressionCaseInsensitive
        error:&error];
    // ...
}
```

## Performance

### CPU Usage
- **Idle**: ~0.1% (polls every second)
- **Active parsing**: ~0.5% (when logs update)

### Memory
- **Base**: ~2 MB
- **Per sample**: ~8 bytes × 3 datasets
- **Total** (1 hour history): ~2.5 MB

### Disk I/O
- Reads only new log lines (incremental)
- No writes to disk
- Minimal I/O impact

## Tips

### Maximize History
Drag the XRG window wider to see more historical data.

### Compare Services
Watch the stacked graph to see which AI service you use most.

### Track Costs
Note your token rates and multiply by your provider's pricing:
- Claude Code: ~$15/1M tokens (input)
- Codex: Varies by model

### Export Data
Currently no built-in export. Future enhancement planned.

## Next Steps

1. **Use it!** Monitor your AI usage in real-time
2. **Customize** colors and layout to your preference
3. **Share feedback** - What other metrics would be useful?

## Need Help?

- **Documentation**: See `AI_TOKEN_IMPLEMENTATION.md` for technical details
- **Issues**: Check the XRG GitHub repository
- **Questions**: Review the implementation source code

---

**Enjoy monitoring your AI token usage with XRG!** 🚀
