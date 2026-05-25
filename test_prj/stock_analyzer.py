#!/usr/bin/env python3
"""
Stock Market Analyzer
Fetches real-time market data, computes technical indicators,
and uses Claude AI to recommend the best investments for the day.
"""

import anthropic
import yfinance as yf
import pandas as pd
import numpy as np
from datetime import datetime, date

# --- Watchlist: customize this list ---
WATCHLIST = [
    # Tech
    "AAPL", "MSFT", "NVDA", "GOOGL", "META", "AMZN", "TSLA",
    # Finance
    "JPM", "BAC", "GS",
    # Healthcare
    "JNJ", "UNH",
    # ETFs
    "SPY", "QQQ", "VTI",
]


def compute_rsi(series: pd.Series, period: int = 14) -> float:
    """Compute Relative Strength Index."""
    delta = series.diff()
    gain = delta.clip(lower=0).rolling(period).mean()
    loss = (-delta.clip(upper=0)).rolling(period).mean()
    rs = gain / loss.replace(0, np.nan)
    rsi = 100 - (100 / (1 + rs))
    return round(float(rsi.iloc[-1]), 2)


def compute_macd(series: pd.Series):
    """Compute MACD line and signal line."""
    ema12 = series.ewm(span=12, adjust=False).mean()
    ema26 = series.ewm(span=26, adjust=False).mean()
    macd = ema12 - ema26
    signal = macd.ewm(span=9, adjust=False).mean()
    histogram = macd - signal
    return round(float(macd.iloc[-1]), 4), round(float(signal.iloc[-1]), 4), round(float(histogram.iloc[-1]), 4)


def fetch_stock_data(ticker: str) -> dict | None:
    """Fetch and compute indicators for a single ticker."""
    try:
        stock = yf.Ticker(ticker)
        hist = stock.history(period="3mo", interval="1d")

        if hist.empty or len(hist) < 30:
            return None

        close = hist["Close"]
        volume = hist["Volume"]
        today = hist.iloc[-1]
        yesterday = hist.iloc[-2]

        price = round(float(today["Close"]), 2)
        prev_price = round(float(yesterday["Close"]), 2)
        pct_change = round((price - prev_price) / prev_price * 100, 2)

        ma20 = round(float(close.rolling(20).mean().iloc[-1]), 2)
        ma50 = round(float(close.rolling(50).mean().iloc[-1]), 2)

        rsi = compute_rsi(close)
        macd, macd_signal, macd_hist = compute_macd(close)

        avg_volume_20d = int(volume.rolling(20).mean().iloc[-1])
        today_volume = int(today["Volume"])
        volume_ratio = round(today_volume / avg_volume_20d, 2) if avg_volume_20d > 0 else 1.0

        # 52-week high/low
        hist_1y = stock.history(period="1y")
        week52_high = round(float(hist_1y["High"].max()), 2)
        week52_low = round(float(hist_1y["Low"].min()), 2)
        pct_from_high = round((price - week52_high) / week52_high * 100, 2)

        info = stock.info
        pe_ratio = info.get("trailingPE") or info.get("forwardPE")
        market_cap = info.get("marketCap")
        sector = info.get("sector", "N/A")
        company_name = info.get("shortName", ticker)

        return {
            "ticker": ticker,
            "company": company_name,
            "sector": sector,
            "price": price,
            "change_pct": pct_change,
            "ma20": ma20,
            "ma50": ma50,
            "rsi": rsi,
            "macd": macd,
            "macd_signal": macd_signal,
            "macd_histogram": macd_hist,
            "volume_ratio": volume_ratio,
            "week52_high": week52_high,
            "week52_low": week52_low,
            "pct_from_52w_high": pct_from_high,
            "pe_ratio": round(pe_ratio, 1) if pe_ratio else None,
            "market_cap_B": round(market_cap / 1e9, 1) if market_cap else None,
        }
    except Exception as e:
        print(f"  [!] {ticker}: {e}")
        return None


def format_market_snapshot(stocks: list[dict]) -> str:
    """Format stock data into a structured text for Claude."""
    lines = [f"Market Snapshot — {date.today().strftime('%B %d, %Y')}\n"]
    lines.append(f"{'Ticker':<8} {'Price':>8} {'Chg%':>7} {'RSI':>6} {'MACD Hist':>10} "
                 f"{'MA20':>8} {'MA50':>8} {'Vol Ratio':>10} {'52W Chg%':>9}")
    lines.append("-" * 80)
    for s in stocks:
        lines.append(
            f"{s['ticker']:<8} {s['price']:>8.2f} {s['change_pct']:>+7.2f}% "
            f"{s['rsi']:>6.1f} {s['macd_histogram']:>+10.4f} "
            f"{s['ma20']:>8.2f} {s['ma50']:>8.2f} "
            f"{s['volume_ratio']:>10.2f}x {s['pct_from_52w_high']:>+8.1f}%"
        )

    lines.append("\nAdditional Details:")
    for s in stocks:
        pe = f"P/E: {s['pe_ratio']}" if s['pe_ratio'] else "P/E: N/A"
        mcap = f"MCap: ${s['market_cap_B']}B" if s['market_cap_B'] else ""
        lines.append(f"  {s['ticker']} ({s['company']}) | Sector: {s['sector']} | {pe} | {mcap}")

    return "\n".join(lines)


def analyze_with_claude(market_data: str) -> str:
    """Send market data to Claude and get investment recommendations."""
    client = anthropic.Anthropic()

    system_prompt = """You are an expert stock market analyst and investment advisor.
You will receive real-time market data including price changes, technical indicators
(RSI, MACD, moving averages), and volume data for a watchlist of stocks.

Your job is to identify the best investment opportunities for TODAY based on:
- Technical signals (RSI oversold/overbought, MACD crossovers, price vs MA)
- Momentum and volume confirmation
- Risk/reward considerations

Output format:
1. **Market Overview** — Brief summary of today's market tone
2. **Top Picks (3-5 stocks)** — For each: ticker, signal strength (Strong/Moderate/Weak),
   key reason, suggested entry approach, and one-line risk note
3. **Stocks to Avoid Today** — Tickers with bearish signals and why
4. **Summary** — One-paragraph overall recommendation

Be concise, data-driven, and specific. Note: This is for educational purposes only,
not financial advice."""

    print("\nAsking Claude to analyze the data...")

    with client.messages.stream(
        model="claude-opus-4-6",
        max_tokens=2048,
        thinking={"type": "adaptive"},
        system=system_prompt,
        messages=[
            {
                "role": "user",
                "content": f"Please analyze this market data and give me your best investment picks for today:\n\n{market_data}",
            }
        ],
    ) as stream:
        result = []
        thinking_shown = False
        for event in stream:
            if event.type == "content_block_start":
                if event.content_block.type == "thinking" and not thinking_shown:
                    print("\n[Claude is thinking...]\n")
                    thinking_shown = True
            elif event.type == "content_block_delta":
                if event.delta.type == "text_delta":
                    print(event.delta.text, end="", flush=True)
                    result.append(event.delta.text)

    return "".join(result)


def main():
    print("=" * 60)
    print("  STOCK MARKET ANALYZER  —  Powered by Claude AI")
    print("=" * 60)
    print(f"\nFetching data for {len(WATCHLIST)} stocks...\n")

    stocks = []
    for ticker in WATCHLIST:
        print(f"  Fetching {ticker}...", end="\r")
        data = fetch_stock_data(ticker)
        if data:
            stocks.append(data)

    print(f"\nSuccessfully fetched data for {len(stocks)}/{len(WATCHLIST)} stocks.")

    if not stocks:
        print("No data available. Check your internet connection.")
        return

    # Sort by volume spike (most unusual volume first) for easier scanning
    stocks.sort(key=lambda x: x["volume_ratio"], reverse=True)

    market_data = format_market_snapshot(stocks)

    print("\n" + "=" * 60)
    print("  RAW MARKET DATA")
    print("=" * 60)
    print(market_data)

    print("\n" + "=" * 60)
    print("  AI ANALYSIS & RECOMMENDATIONS")
    print("=" * 60)

    analyze_with_claude(market_data)

    print("\n\n" + "=" * 60)
    print("  ⚠  DISCLAIMER: For educational purposes only.")
    print("  Always do your own research before investing.")
    print("=" * 60)


if __name__ == "__main__":
    main()
