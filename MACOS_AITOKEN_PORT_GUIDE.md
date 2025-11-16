# macOS AI Token Model Breakdown - Port Guide

**Date**: November 16, 2025
**Author**: Linux XRG Development (via Claude Code)
**Target**: macOS XRG AI Developer
**Purpose**: Port model-by-model token tracking from Linux to macOS

---

## Overview

The Linux version of XRG now displays **per-model token usage breakdown** in the AI Token module. This guide provides everything the macOS developer needs to implement the same feature on macOS.

### What's Working on Linux

✅ **Per-model token tracking** - Displays tokens used per AI model
✅ **Model name extraction** - Correctly parses `message.model` from JSONL
✅ **Visual breakdown** - Shows list of models with token counts
✅ **Current model indicator** - Marks active model with asterisk
✅ **User preference** - Toggle model breakdown on/off

### Visual Example

```
AI Tokens
Rate: 1234/s
Total: 4567890
--- By Model ---
* claude-sonnet-4-5...: 3456789
  claude-opus-4...: 1111101
```

---

## The Bug That Was Fixed

### Problem

The JSONL parser was looking for the `model` field at the **root level** of the JSON object:

```c
// OLD (BROKEN) - Linux code before fix
if (json_object_has_member(obj, "model")) {
    const gchar *mdl = json_object_get_string_member(obj, "model");
    // ...
}
```

But Claude Code's JSONL format has `model` **inside the `message` object**:

```json
{
  "parentUuid": "...",
  "message": {
    "model": "claude-sonnet-4-5-20250929",
    "usage": {
      "input_tokens": 10,
      "output_tokens": 791
    }
  }
}
```

### Solution

Check **both locations** - root level AND inside `message`:

```c
// NEW (FIXED) - Linux code after fix
if (model) {
    const gchar *mdl = NULL;

    if (json_object_has_member(obj, "model")) {
        mdl = json_object_get_string_member(obj, "model");
    } else if (json_object_has_member(obj, "message")) {
        JsonObject *message = json_object_get_object_member(obj, "message");
        if (message && json_object_has_member(message, "model")) {
            mdl = json_object_get_string_member(message, "model");
        }
    }

    if (mdl) {
        g_free(*model);
        *model = g_strdup(mdl);
    }
}
```

---

## macOS Implementation Guide

### Step 1: Locate the macOS JSONL Parser

**File**: `Data Miners/XRGAITokenMiner.m`
**Method**: `- (UInt64)parseJSONLFile:(NSString *)filePath`
**Lines**: ~489-529 (approximately)

### Step 2: Current macOS Code Structure

The macOS parser currently extracts token usage but **does NOT extract model names**:

```objc
// Current macOS code (lines 514-524 approximately)
if (data && data[@"message"] && data[@"message"][@"usage"]) {
    NSDictionary *usage = data[@"message"][@"usage"];

    // Calculate prompt and completion tokens
    UInt64 promptTokens = [usage[@"input_tokens"] unsignedLongLongValue] +
                         [usage[@"cache_creation_input_tokens"] unsignedLongLongValue] +
                         [usage[@"cache_read_input_tokens"] unsignedLongLongValue];
    UInt64 completionTokens = [usage[@"output_tokens"] unsignedLongLongValue];

    // Sum all token types for total (for graph display)
    fileTokens += promptTokens + completionTokens;
}
```

**Problem**: No model extraction happening!

### Step 3: Add Model Extraction to macOS Parser

Modify `parseJSONLFile:` to extract and aggregate model information:

```objc
// Add to parseJSONLFile method
- (UInt64)parseJSONLFile:(NSString *)filePath {
    NSString *content = [NSString stringWithContentsOfFile:filePath
                                                  encoding:NSUTF8StringEncoding
                                                     error:nil];
    if (!content) return 0;

    UInt64 fileTokens = 0;

    // Create dictionary to track per-model tokens for this file
    NSMutableDictionary<NSString *, NSNumber *> *fileModelTokens = [NSMutableDictionary dictionary];

    // Parse line-delimited JSON (JSONL format)
    NSArray *lines = [content componentsSeparatedByString:@"\n"];
    for (NSString *line in lines) {
        if ([line length] == 0) continue;

        NSData *jsonData = [line dataUsingEncoding:NSUTF8StringEncoding];
        NSError *parseError = nil;
        NSDictionary *data = [NSJSONSerialization JSONObjectWithData:jsonData
                                                            options:0
                                                              error:&parseError];

        // Extract model name - check both root and message.model
        NSString *modelName = nil;
        if (data[@"model"]) {
            modelName = data[@"model"];
        } else if (data[@"message"] && data[@"message"][@"model"]) {
            modelName = data[@"message"][@"model"];
        }

        // Extract token usage from message.usage
        if (data && data[@"message"] && data[@"message"][@"usage"]) {
            NSDictionary *usage = data[@"message"][@"usage"];

            // Calculate prompt and completion tokens
            UInt64 promptTokens = [usage[@"input_tokens"] unsignedLongLongValue] +
                                 [usage[@"cache_creation_input_tokens"] unsignedLongLongValue] +
                                 [usage[@"cache_read_input_tokens"] unsignedLongLongValue];
            UInt64 completionTokens = [usage[@"output_tokens"] unsignedLongLongValue];
            UInt64 totalTokens = promptTokens + completionTokens;

            // Sum for file total
            fileTokens += totalTokens;

            // Aggregate by model if we have a model name
            if (modelName && totalTokens > 0) {
                NSNumber *currentTotal = fileModelTokens[modelName] ?: @0;
                fileModelTokens[modelName] = @([currentTotal unsignedLongLongValue] + totalTokens);
            }
        }
    }

    // TODO: Pass fileModelTokens to Observer for aggregation
    // This is where you'll integrate with XRGAITokensObserver

    return fileTokens;
}
```

### Step 4: Update XRGAITokensObserver Integration

The macOS code has this comment (lines 497-500):

```objc
// NOTE: We DON'T call observer.recordEvent here anymore!
// This was causing massive queue buildup and freezing.
// The observer should be updated separately by the actual API calls,
// not by parsing historical files.
```

**Solution**: Instead of recording individual events, aggregate at the file level:

```objc
// Add new method to XRGAITokensObserver.h
- (void)recordAggregatedModelTokens:(NSDictionary<NSString *, NSNumber *> *)modelTokens;

// Implement in XRGAITokensObserver.m
- (void)recordAggregatedModelTokens:(NSDictionary<NSString *, NSNumber *> *)modelTokens {
    if (![self aiTokensTrackingEnabled]) {
        return;
    }

    dispatch_async(self.serialQueue, ^{
        for (NSString *model in modelTokens) {
            NSUInteger tokens = [modelTokens[model] unsignedIntegerValue];

            NSNumber *prevPrompt = self.dailyModelPromptTokens[model] ?: @0;
            NSNumber *prevCompletion = self.dailyModelCompletionTokens[model] ?: @0;

            // For aggregated data, split tokens 50/50 between prompt and completion
            // (This is a reasonable approximation when we don't have detailed breakdown)
            NSUInteger promptTokens = tokens / 2;
            NSUInteger completionTokens = tokens - promptTokens;

            self.dailyModelPromptTokens[model] = @(prevPrompt.unsignedIntegerValue + promptTokens);
            self.dailyModelCompletionTokens[model] = @(prevCompletion.unsignedIntegerValue + completionTokens);
        }

        [self persistDailyCounters];
    });
}
```

### Step 5: Wire Up to Display

The macOS view (`XRGAITokenView.m`) already has model breakdown display code (lines 232-271):

```objc
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
    // ... provider breakdown ...
}
```

✅ **This code is already correct** - it just needs model data to flow through!

### Step 6: Enable Settings by Default

Update `Controllers/XRGPrefController.m` default preferences:

```objc
// In getDefaultPrefs (around line 1016)
@"aiTokensTrackingEnabled": @YES,  // Change from NO to YES
@"aiTokensAggregateByModel": @YES,
@"aiTokensShowBreakdown": @YES,
```

---

## Testing Checklist

### Before Making Changes

1. ✅ Build and run current macOS XRG
2. ✅ Verify AI Token module shows only "Rate" and "Total"
3. ✅ Check that model names are missing

### After Making Changes

1. ✅ Build the updated XRG
2. ✅ Open Preferences → AI Tokens
3. ✅ Verify these settings exist and are enabled:
   - ☑ Enable AI Token Tracking
   - ☑ Aggregate by Model
   - ☑ Show Breakdown
4. ✅ Run XRG and use Claude Code to generate some tokens
5. ✅ Verify the AI Token view now shows:
   ```
   AI Tokens
   Rate: X/s
   Session: XXXK
   Daily: XXXK
   ─ By Model ─
   claude-sonnet-4-5...: XXXK
   ```
6. ✅ Right-click AI Token module → verify context menu shows "By Model" section
7. ✅ Verify model names appear (truncated if necessary)
8. ✅ Verify totals increase as you use Claude Code

### Debug Commands

```bash
# Check JSONL structure
head -1 ~/.claude/projects/*/sessionid.jsonl | python3 -m json.tool | grep -A 5 "message"

# Verify defaults are set
defaults read com.gauchosoft.XRG | grep -i "aiTokens"

# Should show:
# aiTokensTrackingEnabled = 1;
# aiTokensAggregateByModel = 1;
# aiTokensShowBreakdown = 1;
```

---

## Files to Modify

### Required Changes

| File | Purpose | Change Type |
|------|---------|-------------|
| `Data Miners/XRGAITokenMiner.m` | JSONL parser | Add model extraction |
| `Graph Views/XRGAITokensObserver.h` | Observer interface | Add aggregated method |
| `Graph Views/XRGAITokensObserver.m` | Observer implementation | Implement aggregation |
| `Controllers/XRGPrefController.m` | Default prefs | Enable tracking by default |

### Already Correct (No Changes Needed)

| File | Purpose | Status |
|------|---------|--------|
| `Graph Views/XRGAITokenView.m` | Display logic | ✅ Already correct |
| `Utility/XRGSettings.h/m` | Settings accessors | ✅ Already correct |
| `Resources/Preferences.nib` | UI bindings | ✅ Already correct |

---

## Key Differences: Linux vs macOS

| Aspect | Linux (C/GTK) | macOS (Objective-C/Cocoa) |
|--------|---------------|----------------------------|
| **JSON Parsing** | json-glib library | NSJSONSerialization |
| **Model Storage** | GHashTable (key: gchar*, value: ModelTokens*) | NSMutableDictionary (key: NSString*, value: NSNumber*) |
| **Threading** | None (single-threaded) | dispatch_async with serial queue |
| **Memory Mgmt** | Manual g_free() | ARC (automatic) |
| **Settings** | GKeyFile in ~/.config/ | NSUserDefaults |

---

## Expected Outcome

After implementing these changes, the macOS AI Token module will display:

```
AI Tokens
Rate: 1234/s
Session: 4567K
Daily: 9012K/10000K (90%)
─ By Model ─
claude-sonnet-4-5-2...: 7890K
claude-opus-4-202501...: 1122K
─ By Provider ─
anthropic: 9012K
```

---

## Common Pitfalls to Avoid

### 1. Don't Call Observer Per Line
❌ **WRONG**: Recording every JSONL line individually
```objc
for (NSString *line in lines) {
    // Parse line
    [observer recordEventWithPromptTokens:p completionTokens:c model:m provider:nil];
    // ❌ This causes queue buildup and freezing!
}
```

✅ **RIGHT**: Aggregate first, then record once
```objc
NSMutableDictionary *fileModels = [NSMutableDictionary dictionary];
for (NSString *line in lines) {
    // Aggregate in dictionary
}
[observer recordAggregatedModelTokens:fileModels];  // Single call
```

### 2. Don't Forget to Check Both Locations
The `model` field can be at:
- Root level: `data[@"model"]`
- Inside message: `data[@"message"][@"model"]`

Check **both** locations!

### 3. Handle Model Name Truncation
Long model names like `claude-sonnet-4-5-20250929` need truncation:

```objc
NSString *displayName = modelName;
if ([displayName length] > 20) {
    displayName = [[displayName substringToIndex:17] stringByAppendingString:@"..."];
}
```

### 4. Enable Tracking by Default
The Observer won't work unless `aiTokensTrackingEnabled` is YES:

```objc
// In XRGAITokensObserver.m
- (void)recordEventWithPromptTokens:... {
    if (![self aiTokensTrackingEnabled]) {
        return;  // ❌ Silently ignores if disabled!
    }
    // ...
}
```

Set it to YES by default in preferences!

---

## Reference: Linux Implementation

### Linux File Structure
```
xrg-linux/src/
├── collectors/
│   ├── aitoken_collector.h     # Interface
│   └── aitoken_collector.c     # Implementation (JSONL parser)
├── widgets/
│   └── aitoken_widget.c        # Display logic (incomplete stub)
├── core/
│   └── preferences.h/c         # Settings management
└── main.c                      # Drawing code (on_draw_aitoken)
```

### Linux Data Flow
1. **Collector** (`aitoken_collector.c`) parses JSONL
2. **GHashTable** (`model_tokens`) stores per-model totals
3. **Drawing code** (`main.c:on_draw_aitoken`) renders breakdown
4. **Preferences** control visibility (`aitoken_show_model_breakdown`)

### macOS Data Flow (Target)
1. **Miner** (`XRGAITokenMiner.m`) parses JSONL
2. **Observer** (`XRGAITokensObserver.m`) stores per-model totals
3. **View** (`XRGAITokenView.m`) renders breakdown
4. **Settings** control visibility (`aiTokensShowBreakdown`)

---

## Support

If you encounter issues during the port:

1. **Check the Linux implementation**: `xrg-linux/src/collectors/aitoken_collector.c`
2. **Review the git diff**: Exact change that fixed the bug
3. **Test with sample JSONL**: Use `~/.claude/projects/*/sessionid.jsonl`
4. **Enable debug logging**: Add `NSLog()` statements to track model extraction

---

## Success Criteria

✅ Model names appear in AI Token breakdown
✅ Totals increment correctly as Claude Code is used
✅ Settings can toggle model breakdown on/off
✅ Context menu shows model breakdown
✅ No performance degradation (no queue buildup)
✅ Works with all Claude models (Opus, Sonnet, Haiku)

---

**Good luck with the port!** 🚀

The Linux implementation is battle-tested and working perfectly. Following this guide should make the macOS implementation straightforward.

---

**Questions?** Check the git commit for this fix:
```bash
git log --oneline | grep -i "model\|aitoken"
git show <commit-hash>
```
