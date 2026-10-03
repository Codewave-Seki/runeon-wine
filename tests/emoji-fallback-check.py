#!/usr/bin/env python3
"""Check the dwrite system fallback table and unmapped-run length in the patched tree.

Parses system_fallback_config in dlls/dwrite/analyzer.c and resolves sample
characters the way find_fallback_mapping() does for the neutral locale (first
entry whose ranges contain the character). Emoji must resolve to an entry that
lists Segoe UI Emoji, an emoji range may only overlap a later entry, without an emoji
font every covered character must keep its previous families, entries keep
within the parser's 16-range limit, text-presentation symbols stay as they were, and
existing non-emoji entries must keep their first family. It then checks that
fallback_map_characters() never overwrites the mapped run length while trying
families, and reports that run (not the whole locale span) when no family
matches. It does not compile or run dwrite; it reads source text only.

usage: emoji-fallback-check.py SOURCE_ROOT
"""
from pathlib import Path
import re
import sys

EMOJI_SAMPLES = {
    0x231A: "watch",
    0x2614: "umbrella with rain drops",
    0x26A1: "high voltage",
    0x2705: "white heavy check mark (Dingbats)",
    0x2728: "sparkles (Dingbats)",
    0x2B50: "white medium star",
    0x2B55: "heavy large circle",
    0x1F004: "mahjong red dragon",
    0x1F1E6: "regional indicator A",
    0x1F43B: "bear face",
    0x1F600: "grinning face",
    0x1F680: "rocket",
    0x1F7F0: "heavy equals sign",
    0x1F929: "star-struck",
    0x1FAE0: "melting face",
}

# Characters whose first family must not change. Text-presentation symbols
# (check mark, heart suit, arrow) must not be pulled into the emoji font.
UNCHANGED = {
    0x0041: "Tahoma",
    0x2714: "Noto Sans Symbols2",
    0x2764: "Noto Sans Symbols2",
    0x1F800: "Noto Sans Symbols2",
}
UNMAPPED = {
    0x2192: "rightwards arrow",
    0x2665: "black heart suit",
}


def fail(message):
    print(f"FAIL: {message}")
    sys.exit(1)


def parse_table(text):
    start = text.find("system_fallback_config[] =")
    if start < 0:
        fail("system_fallback_config not found")
    end = text.find("\n};", start)
    body = text[start:end]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    entries = []
    for match in re.finditer(r"\{\s*((?:\"[^\"]*\"\s*)+),\s*L\"([^\"]*)\"(?:\s*,\s*L\"([^\"]*)\")?\s*\}", body):
        ranges_text = "".join(re.findall(r"\"([^\"]*)\"", match.group(1)))
        ranges = []
        for part in ranges_text.split(","):
            bounds = part.strip().split("-")
            ranges.append((int(bounds[0], 16), int(bounds[-1], 16)))
        families = [f.strip() for f in match.group(2).split(",")]
        entries.append((ranges, families, match.group(3) or ""))
    if not entries:
        fail("no fallback entries parsed")
    return entries


def neutral_lookup(entries, ch):
    for ranges, families, locale in entries:
        if locale:
            continue
        if any(low <= ch <= high for low, high in ranges):
            return families
    return None


def main():
    if len(sys.argv) != 2:
        fail("usage: emoji-fallback-check.py SOURCE_ROOT")
    path = Path(sys.argv[1]) / "dlls/dwrite/analyzer.c"
    text = path.read_text()
    entries = parse_table(text)
    checks = 0

    # Lookup takes the first matching entry, so an emoji range may overlap a
    # broader entry only when it comes first.
    emoji_entries = [i for i, (_, families, _) in enumerate(entries) if families[0] == "Segoe UI Emoji"]
    if not emoji_entries:
        fail("no Segoe UI Emoji entry")
    for i in emoji_entries:
        ranges = entries[i][0]
        if len(ranges) > 16:
            fail(f"entry {i} has {len(ranges)} ranges; the parser keeps only 16")
        for other, _, _ in (entries[j] for j in range(i) if j not in emoji_entries):
            for low, high in ranges:
                for o_low, o_high in other:
                    if low <= o_high and o_low <= high:
                        fail(f"emoji range {low:04X}-{high:04X} is shadowed by earlier {o_low:04X}-{o_high:04X}")
    checks += 1

    # Without a Segoe UI Emoji font, every character an emoji entry covers must
    # resolve to the families it had before the emoji entries were added.
    without = [e for j, e in enumerate(entries) if j not in emoji_entries]
    for i in emoji_entries:
        for low, high in entries[i][0]:
            for ch in range(low, high + 1):
                before = neutral_lookup(without, ch) or []
                after = [f for f in neutral_lookup(entries, ch) if f != "Segoe UI Emoji"]
                if after != before:
                    fail(f"U+{ch:04X} without Segoe UI Emoji resolves to {after}, previously {before}")
    checks += 1

    for ch, name in EMOJI_SAMPLES.items():
        families = neutral_lookup(entries, ch)
        if not families or "Segoe UI Emoji" not in families:
            fail(f"U+{ch:04X} {name} does not fall back to Segoe UI Emoji: {families}")
        checks += 1

    for ch, family in UNCHANGED.items():
        families = neutral_lookup(entries, ch)
        if not families or families[0] != family:
            fail(f"U+{ch:04X} first family changed: {families}")
        checks += 1

    for ch, name in UNMAPPED.items():
        if neutral_lookup(entries, ch) is not None:
            fail(f"U+{ch:04X} {name} gained a fallback mapping")
        checks += 1

    start = text.find("static HRESULT fallback_map_characters(")
    if start < 0:
        fail("fallback_map_characters not found")
    body = text[start:text.find("\n}\n", start)]
    loop = body[body.find("for (i = 0; i < mapping->families_count; ++i)"):]
    if re.search(r"\bmapped\s*=", loop):
        fail("the family loop overwrites the mapped run length")
    checks += 1
    tail = body[body.rfind("*ret_font = NULL;"):]
    if "*ret_length = mapped;" not in tail or "*ret_length = length;" in tail:
        fail("unsupported mapping does not report the mapped run")
    checks += 1

    print(f"emoji fallback checks passed: {checks}")


if __name__ == "__main__":
    main()
