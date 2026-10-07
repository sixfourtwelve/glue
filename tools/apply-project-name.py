#!/usr/bin/env python3
"""Apply the name in project.json across the project template."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = ROOT / "project.json"
VCPKG_PATH = ROOT / "vcpkg.json"
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"})
PROJECT_NAME_PATTERN = re.compile(r"^[a-z][a-z0-9]*(?:-[a-z0-9]+)*$")


class RenameError(RuntimeError):
    """Raised when the project cannot be renamed safely."""


def read_json(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise RenameError(f"Missing required file: {path.relative_to(ROOT)}") from error
    except json.JSONDecodeError as error:
        raise RenameError(f"Invalid JSON in {path.relative_to(ROOT)}: {error}") from error

    if not isinstance(value, dict):
        raise RenameError(f"Expected an object in {path.relative_to(ROOT)}")
    return value


def require_name(document: dict[str, object], path: Path) -> str:
    name = document.get("name")
    if not isinstance(name, str) or not name:
        raise RenameError(f'Expected a non-empty "name" field in {path.relative_to(ROOT)}')
    return name


def namespace_for(project_name: str) -> str:
    return project_name.replace("-", "_")


def replace_source_names(text: str, old_name: str, new_name: str) -> str:
    old_namespace = namespace_for(old_name)
    new_namespace = namespace_for(new_name)

    text = text.replace(f"<{old_name}/", f"<{new_name}/")
    text = text.replace(f'"{old_name}/', f'"{new_name}/')
    text = re.sub(
        rf"\b{re.escape(old_namespace)}::",
        f"{new_namespace}::",
        text,
    )
    text = re.sub(
        rf"(\bnamespace\s+){re.escape(old_namespace)}\b",
        rf"\g<1>{new_namespace}",
        text,
    )
    return text


def replace_readme_name(text: str, old_name: str, new_name: str) -> str:
    return re.sub(
        rf"(?<![A-Za-z0-9_-]){re.escape(old_name)}(?![A-Za-z0-9_-])",
        new_name,
        text,
    )


def main() -> int:
    project = read_json(CONFIG_PATH)
    vcpkg = read_json(VCPKG_PATH)
    new_name = require_name(project, CONFIG_PATH)
    old_name = require_name(vcpkg, VCPKG_PATH)

    if not PROJECT_NAME_PATTERN.fullmatch(new_name):
        raise RenameError(
            "Project name must use lowercase kebab-case, start with a letter, "
            "and contain only letters, numbers, and hyphens."
        )

    old_include_dir = ROOT / "include" / old_name
    new_include_dir = ROOT / "include" / new_name

    if old_name == new_name:
        if not new_include_dir.is_dir():
            raise RenameError(f"Missing include directory: {new_include_dir.relative_to(ROOT)}")
        print(f'Project is already named "{new_name}".')
        return 0

    if not old_include_dir.is_dir():
        raise RenameError(f"Missing include directory: {old_include_dir.relative_to(ROOT)}")
    if new_include_dir.exists():
        raise RenameError(f"Refusing to overwrite: {new_include_dir.relative_to(ROOT)}")

    source_files = sorted(
        path
        for directory in (ROOT / "include", ROOT / "source")
        for path in directory.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )

    updates: dict[Path, str] = {}
    for path in source_files:
        original = path.read_text(encoding="utf-8")
        updated = replace_source_names(original, old_name, new_name)
        if updated != original:
            updates[path] = updated

    readme_path = ROOT / "README.md"
    readme = readme_path.read_text(encoding="utf-8")
    updated_readme = replace_readme_name(readme, old_name, new_name)
    if updated_readme != readme:
        updates[readme_path] = updated_readme

    vcpkg["name"] = new_name
    updates[VCPKG_PATH] = json.dumps(vcpkg, indent=2) + "\n"

    for path, content in updates.items():
        path.write_text(content, encoding="utf-8")

    old_include_dir.rename(new_include_dir)

    print(f'Renamed project from "{old_name}" to "{new_name}".')
    print(f'C++ namespace: {namespace_for(new_name)}')
    print("Reconfigure the project before building: cmake --preset debug")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RenameError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1) from error
