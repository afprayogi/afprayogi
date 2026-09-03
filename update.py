#!/usr/bin/env python3
"""Update the featured-projects section of README.md from live GitHub data.

Usage:
    python update.py [username]

Reads repos for `username` (default: afprayogi) from the GitHub API,
picks the most recently pushed-to non-fork repos, and rewrites the
block between <!--START_PROJECTS--> and <!--END_PROJECTS--> in
README.md. Also refreshes the "last updated" timestamp between
<!--LAST_UPDATED--> and <!--END_LAST_UPDATED-->.

Set the GITHUB_TOKEN environment variable to avoid GitHub's unauthenticated
rate limit (60 requests/hour).
"""

from __future__ import annotations

import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.error import HTTPError
import json

API_URL = "https://api.github.com/users/{username}/repos?sort=pushed&per_page=100"
README_PATH = Path(__file__).parent / "README.md"
MAX_PROJECTS = 5

PROJECTS_START = "<!--START_PROJECTS-->"
PROJECTS_END = "<!--END_PROJECTS-->"
UPDATED_START = "<!--LAST_UPDATED-->"
UPDATED_END = "<!--END_LAST_UPDATED-->"


def fetch_repos(username: str) -> list[dict]:
    url = API_URL.format(username=username)
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": f"{username}-readme-updater",
    }
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    request = Request(url, headers=headers)
    try:
        with urlopen(request, timeout=15) as response:
            return json.loads(response.read().decode("utf-8"))
    except HTTPError as error:
        sys.exit(f"Failed to fetch repos for '{username}': {error}")


def build_projects_block(repos: list[dict], username: str) -> str:
    non_forks = [
        repo
        for repo in repos
        if not repo.get("fork") and repo["name"].lower() != username.lower()
    ]
    non_forks.sort(key=lambda r: r.get("pushed_at", ""), reverse=True)
    top = non_forks[:MAX_PROJECTS]

    lines = []
    for repo in top:
        name = repo["name"]
        url = repo["html_url"]
        description = repo.get("description")
        language = repo.get("language")
        tag = f" · `{language.lower()}`" if language else ""
        desc_part = f" — {description}" if description else ""
        lines.append(f"- [{name}]({url}){desc_part}{tag}")

    return "\n".join(lines) if lines else "- (nothing here yet)"


def replace_between(text: str, start_marker: str, end_marker: str, new_content: str) -> str:
    pattern = re.compile(
        re.escape(start_marker) + r".*?" + re.escape(end_marker), re.DOTALL
    )
    replacement = f"{start_marker}\n{new_content}\n{end_marker}"
    if not pattern.search(text):
        sys.exit(f"Markers {start_marker!r}/{end_marker!r} not found in README.md")
    return pattern.sub(replacement, text)


def main() -> None:
    username = sys.argv[1] if len(sys.argv) > 1 else "afprayogi"

    if not README_PATH.exists():
        sys.exit(f"README.md not found at {README_PATH}")

    readme_text = README_PATH.read_text(encoding="utf-8")

    repos = fetch_repos(username)
    projects_block = build_projects_block(repos, username)
    readme_text = replace_between(readme_text, PROJECTS_START, PROJECTS_END, projects_block)

    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    updated_block = (
        f"`last sync: {timestamp}` — kept honest by [update.py](update.py), not by hand"
    )
    readme_text = replace_between(readme_text, UPDATED_START, UPDATED_END, updated_block)

    README_PATH.write_text(readme_text, encoding="utf-8")
    print(f"README.md updated for '{username}' at {timestamp}")


if __name__ == "__main__":
    main()
