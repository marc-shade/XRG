# LinkedIn Post: XRG AI Token Monitoring

---

**Building AI Observability with Familiar Tools: A Journey into XRG** 🚀

On our journey towards complete observability for our AI stack, I made an interesting choice: instead of adopting yet another new monitoring solution, I went back to a tool I've trusted for years.

**The Evolution**

XRG has been my go-to system monitor on macOS for as long as I can remember. Coming from a Linux background where I relied on tools like Gkrellm and other monitoring apps, I've always valued real-time visibility into what my systems are doing. A while back, I even rolled my own Swift-based menubar app for macOS monitoring.

But this time was different.

**The Challenge**

As we built out our AI infrastructure with Claude Code and other LLM tools, I realized we needed visibility into something new: **AI token consumption**. We're burning through millions of tokens, but had no real-time insight into usage patterns, costs, or spikes during development.

The solution? Fork XRG and add AI token monitoring.

**The Learning Experience**

Here's where it got interesting. XRG is a C-based Objective-C project in Xcode - a completely different beast from the Swift I was comfortable with. Diving into:
- Direct C API integration (SQLite3, IOKit)
- Grand Central Dispatch for background threading
- Objective-C memory management patterns
- JSONL file parsing at scale (2.8+ billion tokens tracked!)
- Real-time data visualization

...was simultaneously humbling and exhilarating.

**The Result**

Now I have a monitoring tool that shows:
✅ CPU, GPU, Memory, Network, Disk (the classics)
✅ **Real-time AI token consumption** - live updates within 1-2 seconds
✅ **Universal compatibility** - works on any Mac with Claude Code installed
✅ **Zero configuration** - automatic detection of data sources

The AI token module intelligently falls back between JSONL transcripts (default), SQLite databases (custom setups), and OpenTelemetry endpoints (advanced configurations).

**Why This Matters**

When building AI systems, observability isn't optional - it's essential. But it doesn't always mean adopting new tools. Sometimes the best solution is enhancing what you already know and trust.

The killer app I wanted was right there all along. I just had to build it.

**Open Source**

The fork is live on GitHub. If you're working with Claude Code or other AI tools and want real-time token visibility, check it out:

🔗 https://github.com/marc-shade/XRG

*Sometimes the best new tool is the old tool you know, reimagined.*

---

#AI #MachineLearning #Observability #OpenSource #DeveloperTools #AIEngineering #MacOS #LLM

---

**Optional shorter version:**

**Real-Time AI Token Monitoring with XRG** 🎯

Added AI token tracking to XRG, my trusted macOS system monitor. Now I can see Claude Code token consumption in real-time alongside CPU, memory, and network stats.

The journey from Swift comfort zone into C-based Objective-C was a fantastic learning experience. Threading, performance optimization, and handling 2.8+ billion tokens taught me new tricks.

Best part? Works out-of-the-box on any Mac with Claude Code. No configuration needed.

Fork available: https://github.com/marc-shade/XRG

Sometimes the best new tool is your old favorite, enhanced.

#AI #Observability #OpenSource #MacOS
