"""Release information for the GitHub workflows (Python 3, standard library only).

    python .github/scripts/release_info.py version
        Prints the version from project(... VERSION x.y.z) in CMakeLists.txt.

    python .github/scripts/release_info.py notes 0.2.0 [--output release-notes.md]
        Writes the release notes for 0.2.0: the "## 0.2.0" section of
        CHANGELOG.md and how to install the download. Prints a GitHub warning
        annotation if CHANGELOG.md has no section for that version.
"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]

# CMake tokens: quoted strings (which may contain parentheses), comments,
# brackets and bare words.
TOKEN = re.compile(r'"(?:[^"\\]|\\.)*"|#[^\n]*|[()]|[^\s()"#]+')


def read_version():
    text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    start = re.search(r"^\s*project\s*\(", text, re.IGNORECASE | re.MULTILINE)
    if not start:
        sys.exit("CMakeLists.txt: no project(...) call found")

    args = []
    depth = 1
    for match in TOKEN.finditer(text, start.end()):
        token = match.group(0)
        if token.startswith("#"):
            continue
        if token == "(":
            depth += 1
        elif token == ")":
            depth -= 1
            if depth == 0:
                break
        args.append(token)

    for i, arg in enumerate(args):
        if arg == "VERSION" and i + 1 < len(args):
            version = args[i + 1].strip('"')
            if re.fullmatch(r"[0-9]+(\.[0-9]+){0,3}", version):
                return version
            sys.exit(f"CMakeLists.txt: project(... VERSION {version}) is not a version number like 1.2.3")
    sys.exit("CMakeLists.txt: project(...) has no VERSION")


def release_notes(version):
    changelog = ROOT / "CHANGELOG.md"
    text = changelog.read_text(encoding="utf-8") if changelog.exists() else ""
    section = re.search(rf"^##[ \t]+v?{re.escape(version)}(?![0-9.])[^\n]*\n(.*?)(?=^##[ \t]|\Z)",
                        text, re.MULTILINE | re.DOTALL)
    body = section.group(1).strip() if section else ""
    download = (f"**Windows, 64-bit:** download `ImageAnalyzerTool-{version}-windows-x64.zip`, unzip it "
                f"anywhere and run `ImageAnalyzerTool.exe`. Nothing else needs to be installed. Keep the "
                f"files of the folder together. If Windows says \"Windows protected your PC\", click "
                f"**More info → Run anyway** (the app is not code-signed).")
    notes = f"{body}\n\n{download}\n" if body else f"{download}\n"
    return notes, bool(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("version", help="print the version from CMakeLists.txt")
    notes = sub.add_parser("notes", help="write the release notes for a version")
    notes.add_argument("version")
    notes.add_argument("--output", type=pathlib.Path, help="file to write (default: standard output)")
    args = parser.parse_args()

    if args.command == "version":
        print(read_version())
        return

    text, found = release_notes(args.version)
    if args.output:
        args.output.write_text(text, encoding="utf-8")
        if not found:
            print(f"::warning file=CHANGELOG.md::CHANGELOG.md has no '## {args.version}' section, "
                  f"so the release notes only say how to install the download.")
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
