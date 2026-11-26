#!/usr/bin/env python3
"""Update README's pinned direct DLL URL after a successful GitHub Release.

Runs only after gh release create succeeds. GitHub's GITHUB_TOKEN-authored commits
do not trigger another push workflow run. Keep versioned release URLs immutable.
"""
import base64
import json
import os
import re
import sys
import time
from urllib.error import HTTPError
from urllib.request import Request, urlopen

DIRECT_LINE = re.compile(
    r"(?m)^   \*\*Direct download \(dev-[0-9]+-[0-9]+\):\*\* \[d3d12\.dll\]"
    r"\(https://github\.com/w0uFF/w0uFF-Together/releases/download/dev-[0-9]+-[0-9]+/d3d12\.dll\)$"
)
NOTE = (
    "The direct download above points to a specific development build. "
    "For newer versions, use the [Releases page]"
)
NEW_NOTE = (
    "This direct download is automatically updated after each successful development "
    "Release. Previous builds remain available from the [Releases page]"
)


def rewrite_readme(source: str, tag: str) -> str:
    if re.fullmatch(r"dev-[0-9]+-[0-9]+", tag) is None:
        raise ValueError("Invalid development release tag")
    target = (
        f"   **Direct download ({tag}):** "
        f"[d3d12.dll](https://github.com/w0uFF/w0uFF-Together/releases/download/{tag}/d3d12.dll)"
    )
    rewritten, count = DIRECT_LINE.subn(lambda _: target, source)
    if count != 1:
        raise ValueError(f"Expected exactly one pinned direct link, found {count}")
    return rewritten.replace(NOTE, NEW_NOTE, 1)


def api(method: str, url: str, token: str, data: dict | None = None) -> dict:
    body = None if data is None else json.dumps(data).encode("utf-8")
    request = Request(url, data=body, method=method, headers={
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {token}",
        "X-GitHub-Api-Version": "2022-11-28",
        "Content-Type": "application/json",
    })
    with urlopen(request, timeout=30) as response:
        return json.load(response)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: update_release_readme.py dev-<run>-<attempt>")
    tag = sys.argv[1]
    token = os.environ["GH_TOKEN"]
    repo = os.environ["GITHUB_REPOSITORY"]
    if repo != "w0uFF/w0uFF-Together":
        raise RuntimeError("Unexpected repository")
    base = f"https://api.github.com/repos/{repo}"
    # Verify the named release and its downloadable DLL before updating README.
    release = api("GET", f"{base}/releases/tags/{tag}", token)
    if release["draft"] or not any(a["name"] == "d3d12.dll" for a in release["assets"]):
        raise RuntimeError("Release or DLL asset is not published")
    # Guard against older, slower runs replacing a newer release's link.
    releases = api("GET", f"{base}/releases?per_page=30", token)
    published = [r for r in releases if not r["draft"] and
                 any(a["name"] == "d3d12.dll" for a in r["assets"])]
    def version(r):
        m = re.fullmatch(r"dev-([0-9]+)-([0-9]+)", r["tag_name"])
        return tuple(map(int, m.groups())) if m else (-1, -1)
    if published and version(release) < max(map(version, published)):
        print(f"Skipping older release {tag}: a newer DLL release exists")
        return
    url = f"{base}/contents/README.md"
    for attempt in range(3):
        file = api("GET", url, token)
        original = base64.b64decode(file["content"]).decode("utf-8")
        updated = rewrite_readme(original, tag)
        if original == updated:
            print(f"README already links to {tag}")
            return
        try:
            api("PUT", url, token, {
                "message": f"docs: update direct DLL link to {tag} [skip ci]",
                "content": base64.b64encode(updated.encode("utf-8")).decode("ascii"),
                "sha": file["sha"],
                "branch": "main",
            })
            print(f"README direct DLL link updated to {tag}")
            return
        except HTTPError as error:
            if error.code not in (409, 422) or attempt == 2:
                raise
            time.sleep(1)
    raise RuntimeError("README update failed")


if __name__ == "__main__":
    main()
