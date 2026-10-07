#!/usr/bin/env python3
"""Apply a new project name, then remove this one-shot template initializer."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = Path(__file__).resolve()
CMAKE_PATH = ROOT / "CMakeLists.txt"
VCPKG_PATH = ROOT / "vcpkg.json"
README_PATH = ROOT / "README.md"
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"})
PROJECT_NAME_PATTERN = re.compile(r"^[a-z][a-z0-9]*(?:-[a-z0-9]+)*$")
TEMPLATE_SECTION_PATTERN = re.compile(
    r"<!-- template-setup:start -->\n.*?<!-- template-setup:end -->\n*",
    re.DOTALL,
)


class RenameError(RuntimeError):
    """Raised when the project cannot be renamed safely."""


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Rename this project template and remove the initializer."
    )
    parser.add_argument(
        "name",
        help="lowercase kebab-case project name, for example: my-game",
    )
    return parser.parse_args()


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


def replace_cmake_name(text: str, old_name: str, new_name: str) -> str:
    pattern = re.compile(
        rf"(\bproject\(\s*\"?){re.escape(old_name)}(\"?\s+VERSION\b)"
    )
    updated, replacements = pattern.subn(rf"\g<1>{new_name}\g<2>", text, count=1)
    if replacements != 1:
        raise RenameError(f'Could not find project "{old_name}" in CMakeLists.txt')
    return updated


def replace_readme_name(text: str, old_name: str, new_name: str) -> str:
    text = TEMPLATE_SECTION_PATTERN.sub("", text, count=1)
    return re.sub(
        rf"(?<![A-Za-z0-9_-]){re.escape(old_name)}(?![A-Za-z0-9_-])",
        new_name,
        text,
    )


def remove_initializer() -> None:
    gitkeep_path = SCRIPT_PATH.parent / ".gitkeep"
    gitkeep_path.touch(exist_ok=True)
    SCRIPT_PATH.unlink()


def main() -> int:
    arguments = parse_arguments()
    new_name = arguments.name
    vcpkg = read_json(VCPKG_PATH)
    old_name = require_name(vcpkg, VCPKG_PATH)

    if not PROJECT_NAME_PATTERN.fullmatch(new_name):
        raise RenameError(
            "Project name must use lowercase kebab-case, start with a letter, "
            "and contain only letters, numbers, and hyphens."
        )
    if old_name == new_name:
        raise RenameError(f'Choose a name other than the template name "{old_name}"')

    old_include_dir = ROOT / "include" / old_name
    new_include_dir = ROOT / "include" / new_name

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

    cmake = CMAKE_PATH.read_text(encoding="utf-8")
    updates[CMAKE_PATH] = replace_cmake_name(cmake, old_name, new_name)

    readme = README_PATH.read_text(encoding="utf-8")
    updates[README_PATH] = replace_readme_name(readme, old_name, new_name)

    vcpkg["name"] = new_name
    updates[VCPKG_PATH] = json.dumps(vcpkg, indent=2) + "\n"

    for path, content in updates.items():
        path.write_text(content, encoding="utf-8")

    old_include_dir.rename(new_include_dir)
    remove_initializer()

    print(f'Renamed project from "{old_name}" to "{new_name}".')
    print(f'C++ namespace: {namespace_for(new_name)}')
    print("Removed the template initializer and created tools/.gitkeep.")
    print("Reconfigure the project before building: cmake --preset debug")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RenameError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1) from error
