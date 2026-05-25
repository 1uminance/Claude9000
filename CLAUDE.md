# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This repository contains two independent projects:

1. **`baselight_tool_01/`** — A C++17 OFX plugin suite for film/color processing
2. **`test_prj/`** — A Python stock market analyzer using Claude AI

---

## baselight_tool_01 — OFX Plugin Suite

### Building

Requires CMake 3.24+ and a C++17 compiler. The OFX SDK is fetched automatically from GitHub at configure time.

```bash
cd baselight_tool_01
cmake -B build
cmake --build build
cmake --install build
```

Output is a platform-specific OFX bundle (`FilmTools.ofx`):
- **macOS**: Universal binary (arm64 + x86_64)
- **Linux**: x86-64

### Architecture

Two OFX plugins compiled into a single shared library (`FilmTools.ofx`):

- **FilmSplit** (`src/FilmSplit/`) — Extracts one RGB channel (R/G/B selectable) and outputs it as monochrome. Plugin ID: `com.filmtools.FilmSplit`
- **FilmCombine** (`src/FilmCombine/`) — Recombines three monochrome channel inputs into RGB with cross-talk matrix modeling. Plugin ID: `com.filmtools.FilmCombine`
- **Common** (`src/common/`) — `ImageProcessor` abstract base class for single-pass CPU pixel processors; subclasses implement `processRow()`.

**FilmCombine cross-talk matrix**: Models dye-layer interlayer diffusion in film emulsion. Six off-diagonal entries named `XY` meaning "X channel bleeds into Y output" (e.g., `RG` = R bleeds into G). Typical values 0.0–0.10, can be negative.

**`src/PluginMain.cpp`** is the entry point: it implements `OFX::Plugin::getPluginIDs()`, registering static instances of both factories with the OFX host. This is the standard OFX support library registration hook.

Symbol visibility is controlled via `cmake/ofx_exports.sym` (macOS) and `cmake/ofx_exports.lds` (Linux). macOS bundle metadata is generated from `cmake/Info.plist.in` (bundle ID: `com.filmtools.FilmTools`).

> **Note**: OFX headers are fetched by CMake at configure time — LSP errors about missing headers are expected until `cmake -B build` has been run.

---

## test_prj — Stock Analyzer

### Setup & Running

```bash
cd test_prj
pip install -r requirements.txt   # anthropic, yfinance, pandas, numpy
python stock_analyzer.py
```

Requires the `ANTHROPIC_API_KEY` environment variable to be set.

### Architecture

`stock_analyzer.py` is a single-file script that:
1. Fetches real-time market data for a hardcoded watchlist of 13 stocks/ETFs via `yfinance`
2. Computes technical indicators (RSI, MACD, 20/50-day MAs, volume ratio, 52-week high/low)
3. Sends the market snapshot to **Claude Opus 4.6** with extended thinking enabled
4. Streams the AI response (including thinking blocks) and prints investment recommendations

The watchlist covers: Tech (AAPL, MSFT, NVDA, GOOGL, META, AMZN, TSLA), Finance (JPM, BAC, GS), Healthcare (JNJ, UNH), ETFs (SPY, QQQ, VTI).
