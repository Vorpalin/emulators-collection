#!/usr/bin/env python3

"""
Generate GitHub Wiki Markdown pages from Doxygen XML.

The script reads:

    build/doxygen/xml/

and generates:

    build/wiki/api/

The generated pages contain information about:
- classes
- structs
- enums
- functions
- variables
- inheritance
- brief descriptions
- detailed descriptions
- source files
"""

from __future__ import annotations

import html
import shutil
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


XML_DIR = Path("build/doxygen/xml")
OUTPUT_DIR = Path("build/wiki/api")


def clean_text(text: str | None) -> str:
    """Convert Doxygen XML text into readable Markdown text."""

    if not text:
        return ""

    text = html.unescape(text)

    # Basic Doxygen XML cleanup.
    replacements = {
        "<computeroutput>": "`",
        "</computeroutput>": "`",
        "<bold>": "**",
        "</bold>": "**",
        "<emphasis>": "*",
        "</emphasis>": "*",
        "<sp/>": " ",
        "<linebreak/>": "\n",
    }

    for old, new in replacements.items():
        text = text.replace(old, new)

    return text.strip()


def get_text(element: ET.Element | None, tag: str) -> str:
    """Return the text contained in a child XML element."""

    if element is None:
        return ""

    child = element.find(tag)

    if child is None:
        return ""

    return clean_text("".join(child.itertext()))


def get_compound_description(
    compound: ET.Element,
) -> tuple[str, str]:
    """Return brief and detailed descriptions."""

    brief = get_text(compound, "briefdescription")
    detailed = get_text(compound, "detaileddescription")

    return brief, detailed


def get_member_description(
    member: ET.Element,
) -> tuple[str, str]:
    """Return brief and detailed descriptions for a member."""

    brief = get_text(member, "briefdescription")
    detailed = get_text(member, "detaileddescription")

    return brief, detailed


def format_type(member: ET.Element) -> str:
    """Return a readable member type."""

    type_element = member.find("type")

    if type_element is None:
        return ""

    return clean_text("".join(type_element.itertext())).strip()


def format_args(member: ET.Element) -> str:
    """Return a readable function argument list."""

    args = []

    for param in member.findall("param"):
        type_element = param.find("type")
        name_element = param.find("declname")

        arg_type = ""

        if type_element is not None:
            arg_type = clean_text(
                "".join(type_element.itertext())
            ).strip()

        arg_name = ""

        if name_element is not None and name_element.text:
            arg_name = name_element.text.strip()

        argument = " ".join(
            part for part in (arg_type, arg_name) if part
        )

        if argument:
            args.append(argument)

    return ", ".join(args)


def format_signature(member: ET.Element) -> str:
    """Build a readable C++ function signature."""

    name = member.get("name", "")
    args = format_args(member)
    return f"{name}({args})"


def get_location(member: ET.Element) -> str:
    """Return the source file associated with a member."""

    location = member.find("location")

    if location is None:
        return ""

    file_name = location.get("file")

    if not file_name:
        return ""

    return file_name


def write_class_page(
    compound: ET.Element,
    output_dir: Path,
) -> None:
    """Generate a Markdown page for one class/struct."""

    compound_name = compound.get("name", "Unknown")

    compound_kind = compound.get("kind", "class")

    brief, detailed = get_compound_description(compound)

    output_file = output_dir / f"{compound_name}.md"

    lines: list[str] = []

    lines.append(f"# `{compound_name}`")
    lines.append("")

    if compound_kind == "struct":
        lines.append("**Type:** struct")
    else:
        lines.append("**Type:** class")

    lines.append("")

    if brief:
        lines.append(brief)
        lines.append("")

    if detailed:
        lines.append(detailed)
        lines.append("")

    # --------------------------------------------------------
    # Base classes
    # --------------------------------------------------------

    base_classes = []

    for base in compound.findall("basecompoundref"):
        if base.text:
            base_classes.append(base.text.strip())

    if base_classes:
        lines.append("## Inheritance")
        lines.append("")

        for base in base_classes:
            lines.append(f"- `{base}`")

        lines.append("")

    # --------------------------------------------------------
    # Members
    # --------------------------------------------------------

    member_groups = compound.findall("sectiondef")

    functions: list[ET.Element] = []
    variables: list[ET.Element] = []
    enums: list[ET.Element] = []
    typedefs: list[ET.Element] = []

    for section in member_groups:
        for member in section.findall("memberdef"):
            kind = member.get("kind")

            if kind == "function":
                functions.append(member)

            elif kind == "variable":
                variables.append(member)

            elif kind == "enum":
                enums.append(member)

            elif kind == "typedef":
                typedefs.append(member)

    # --------------------------------------------------------
    # Functions
    # --------------------------------------------------------

    if functions:
        lines.append("## Functions")
        lines.append("")

        for member in functions:
            name = member.get("name", "unknown")
            signature = format_signature(member)
            brief, detailed = get_member_description(member)
            return_type = format_type(member)

            lines.append(f"### `{signature}`")
            lines.append("")

            if return_type:
                lines.append(f"**Return type:** `{return_type}`")
                lines.append("")

            if brief:
                lines.append(brief)
                lines.append("")

            if detailed:
                lines.append(detailed)
                lines.append("")

            location = get_location(member)

            if location:
                lines.append(f"**Source:** `{location}`")
                lines.append("")

    # --------------------------------------------------------
    # Variables
    # --------------------------------------------------------

    if variables:
        lines.append("## Variables")
        lines.append("")

        for member in variables:
            name = member.get("name", "unknown")
            member_type = format_type(member)

            brief, detailed = get_member_description(member)

            if member_type:
                lines.append(f"### `{member_type} {name}`")
            else:
                lines.append(f"### `{name}`")

            lines.append("")

            if brief:
                lines.append(brief)
                lines.append("")

            if detailed:
                lines.append(detailed)
                lines.append("")

    # --------------------------------------------------------
    # Enums
    # --------------------------------------------------------

    if enums:
        lines.append("## Enumerations")
        lines.append("")

        for enum in enums:
            name = enum.get("name", "anonymous")

            brief, detailed = get_member_description(enum)

            lines.append(f"### `{name}`")
            lines.append("")

            if brief:
                lines.append(brief)
                lines.append("")

            if detailed:
                lines.append(detailed)
                lines.append("")

            for value in enum.findall("enumvalue"):
                value_name = value.get("name", "")

                if value_name:
                    lines.append(f"- `{value_name}`")

            lines.append("")

    # --------------------------------------------------------
    # Typedefs
    # --------------------------------------------------------

    if typedefs:
        lines.append("## Type definitions")
        lines.append("")

        for typedef in typedefs:
            name = typedef.get("name", "unknown")
            typedef_type = format_type(typedef)

            brief, detailed = get_member_description(typedef)

            if typedef_type:
                lines.append(
                    f"### `{typedef_type} {name}`"
                )
            else:
                lines.append(f"### `{name}`")

            lines.append("")

            if brief:
                lines.append(brief)
                lines.append("")

            if detailed:
                lines.append(detailed)
                lines.append("")

    # --------------------------------------------------------
    # Footer
    # --------------------------------------------------------

    lines.append("---")
    lines.append("")
    lines.append(
        "*This page was automatically generated from "
        "Doxygen documentation.*"
    )
    lines.append("")

    output_file.write_text(
        "\n".join(lines),
        encoding="utf-8",
    )

    print(f"Generated {output_file}")


def generate() -> None:
    """Generate all API documentation."""

    if not XML_DIR.exists():
        print(
            f"ERROR: Doxygen XML directory not found: {XML_DIR}",
            file=sys.stderr,
        )
        sys.exit(1)

    if OUTPUT_DIR.exists():
        shutil.rmtree(OUTPUT_DIR)

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    index_entries: list[str] = []

    index = XML_DIR / "index.xml"

    if not index.exists():
        print(
            f"ERROR: Doxygen index not found: {index}",
            file=sys.stderr,
        )
        sys.exit(1)

    tree = ET.parse(index)
    root = tree.getroot()

    compounds = root.findall("compound")

    for compound_ref in compounds:
        refid = compound_ref.get("refid")

        if not refid:
            continue

        compound_xml = XML_DIR / f"{refid}.xml"

        if not compound_xml.exists():
            continue

        compound_tree = ET.parse(compound_xml)
        compound = compound_tree.getroot()

        kind = compound.get("kind")

        if kind not in {
            "class",
            "struct",
        }:
            continue

        name_element = compound.find("compoundname")

        if name_element is None or not name_element.text:
            continue

        name = name_element.text.strip()

        write_class_page(
            compound,
            OUTPUT_DIR,
        )

        index_entries.append(name)

    # --------------------------------------------------------
    # Generate API index
    # --------------------------------------------------------

    index_file = OUTPUT_DIR / "API.md"

    lines = [
        "# API Reference",
        "",
        "Automatically generated from the C++ source code using Doxygen.",
        "",
    ]

    if index_entries:
        lines.append("## Classes and structures")
        lines.append("")

        for name in sorted(index_entries):
            lines.append(
                f"- [`{name}`]({name}.md)"
            )

        lines.append("")
    else:
        lines.extend(
            [
                "No classes or structures were found.",
                "",
            ]
        )

    index_file.write_text(
        "\n".join(lines),
        encoding="utf-8",
    )

    print(
        f"Generated {len(index_entries)} API pages."
    )


if __name__ == "__main__":
    generate()
