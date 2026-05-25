# Claude9000

Two independent projects for film/color processing and AI-powered market analysis.

---

## baselight_tool_01 — FilmTools OFX Plugin Suite

A pair of OFX plugins for film emulsion workflows, designed to work together:

- **FilmSplit** — Extracts a single RGB channel and outputs it as monochrome
- **FilmCombine** — Recombines three processed monochrome channels into RGB, with a cross-talk matrix to model dye-layer interlayer diffusion in film emulsion

### Build

Requires CMake 3.24+ and a C++17 compiler.

```bash
cd baselight_tool_01
git submodule update --init
cmake -B build
cmake --build build
```

Output: `build/FilmTools.ofx.bundle` (macOS universal arm64/x86_64)

### Install

Copy `FilmTools.ofx.bundle` to your OFX plugin directory:
- **macOS**: `/Library/OFX/Plugins/`
- **Linux**: `/usr/OFX/Plugins/`

---

## test_prj — Stock Analyzer

A Python script that fetches real-time market data and uses Claude AI with extended thinking to generate investment analysis.

### Setup

```bash
cd test_prj
pip install -r requirements.txt
export ANTHROPIC_API_KEY=your_key_here
python stock_analyzer.py
```

Covers 13 instruments: AAPL, MSFT, NVDA, GOOGL, META, AMZN, TSLA, JPM, BAC, GS, JNJ, UNH, SPY, QQQ, VTI.
