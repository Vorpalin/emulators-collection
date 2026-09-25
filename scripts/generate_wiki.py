#!/usr/bin/env python3

from __future__ import annotations

import html
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


XML_DIR = Path("build/doxygen/xml")
OUTPUT_DIR = Path("build/wiki/api")


def clean_text(text: str | None) -> str:
    if not text:
        return ""

    text = html.unescape(text)

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


def element_text(element: ET.Element | None) -> str:
    if element is None:
        return ""

    return clean_text("".join(element.itertext()))


def description(element: ET.Element) -> tuple[str, str]:
    return (
        element_text(element.find("briefdescription")),
        element_text(element.find("detaileddescription")),
    )


def member_type(member: ET.Element) -> str:
    return element_text(member.find("type"))


def member_arguments(member: ET.Element) -> str:
    arguments = []

    for param in member.findall("param"):
        param_type = element_text(param.find("type"))
        param_name = element_text(param.find("declname"))

        argument = " ".join(
            part
            for part in (param_type, param_name)
            if part
        )

        if argument:
            arguments.append(argument)

    return ", ".join(arguments)


def member_name(member: ET.Element) -> str:
    name = element_text(member.find("name"))

    if name:
        return name

    name = member.get("name")

    if name:
        return name

    return ""


def member_signature(member: ET.Element) -> str:
    name = member_name(member)

    if not name:
        definition = element_text(member.find("definition"))

        if definition:
            name = definition.split("::")[-1]

    if not name:
        return ""

    args = element_text(member.find("argsstring"))

    if args:
        # Remove qualifiers that should not be displayed as part
        # of the function argument list.
        args = args.strip()

        args = args.replace(" override", "")
        args = args.replace(" final", "")

        return f"{name}{args}"

    return f"{name}({member_arguments(member)})"


def parameter_descriptions(member: ET.Element) -> list[tuple[str, str]]:
    result = []

    detailed = member.find("detaileddescription")

    if detailed is None:
        return result

    for parameter in detailed.findall(".//parameterlist[@kind='param']/parameteritem"):
        name = element_text(parameter.find("parameternamelist/parametername"))
        description = element_text(
            parameter.find("parameterdescription")
        )

        if name:
            result.append((name, description))

    return result


def return_description(member: ET.Element) -> str:
    detailed = member.find("detaileddescription")

    if detailed is None:
        return ""

    for parameter_list in detailed.findall(
        ".//simplesect[@kind='return']"
    ):
        return element_text(parameter_list)

    return ""


def source_location(member: ET.Element) -> str:
    location = member.find("location")

    if location is None:
        return ""

    return location.get("file", "")


def write_compound(compound: ET.Element) -> Path | None:
    kind = compound.get("kind")

    if kind not in {"class", "struct"}:
        return None

    name = element_text(compound.find("compoundname"))

    if not name:
        return None

    filename = name.replace("::", "_") + ".md"
    output = OUTPUT_DIR / filename

    brief, detailed = description(compound)

    lines = [
        f"# `{name}`",
        "",
        f"**Type:** {kind}",
        "",
    ]

    if brief:
        lines += [brief, ""]

    if detailed:
        lines += [detailed, ""]

    # Inheritance
    bases = []

    for base in compound.findall("basecompoundref"):
        base_name = element_text(base)

        if base_name:
            bases.append(base_name)

    if bases:
        lines += [
            "## Inheritance",
            "",
        ]

        for base in bases:
            lines.append(f"- `{base}`")

        lines.append("")

    functions = []
    variables = []
    enums = []
    typedefs = []

    for section in compound.findall("sectiondef"):
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

    # Functions
    if functions:
        lines += [
            "## Functions",
            "",
        ]

        for function in functions:
            signature = member_signature(function)

            if not signature:
                continue

            return_type = member_type(function)
            brief, detailed = description(function)

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

            parameters = parameter_descriptions(function)

            if parameters:
                lines += [
                    "#### Parameters",
                    "",
                    "| Name | Description |",
                    "|---|---|",
                ]

                for name, parameter_description in parameters:
                    lines.append(
                        f"| `{name}` | {parameter_description} |"
                    )

                lines.append("")

            returns = return_description(function)

            if returns:
                lines += [
                    "#### Returns",
                    "",
                    returns,
                    "",
                ]

            source = source_location(function)

            if source:
                lines += [
                    f"**Source:** `{source}`",
                    "",
                ]

    # Variables
    if variables:
        lines += [
            "## Variables",
            "",
        ]

        for variable in variables:
            name = member_name(variable)

            if not name:
                continue

            variable_type = member_type(variable)
            brief, detailed = description(variable)

            if variable_type:
                lines += [
                    f"### `{variable_type} {name}`",
                    "",
                ]
            else:
                lines += [
                    f"### `{name}`",
                    "",
                ]

            if brief:
                lines += [brief, ""]

            if detailed:
                lines += [detailed, ""]

    # Enums
    if enums:
        lines += [
            "## Enumerations",
            "",
        ]

        for enum in enums:
            name = enum.get("name", "anonymous")
            brief, detailed = description(enum)

            lines += [
                f"### `{name}`",
                "",
            ]

            if brief:
                lines += [brief, ""]

            if detailed:
                lines += [detailed, ""]

            for value in enum.findall("enumvalue"):
                value_name = value.get("name")

                if value_name:
                    lines.append(f"- `{value_name}`")

            lines.append("")

    # Typedefs
    if typedefs:
        lines += [
            "## Type definitions",
            "",
        ]

        for typedef in typedefs:
            name = typedef.get("name", "unknown")
            typedef_type = member_type(typedef)
            brief, detailed = description(typedef)

            if typedef_type:
                lines += [
                    f"### `{typedef_type} {name}`",
                    "",
                ]
            else:
                lines += [
                    f"### `{name}`",
                    "",
                ]

            if brief:
                lines += [brief, ""]

            if detailed:
                lines += [detailed, ""]

    lines += [
        "---",
        "",
        "*Automatically generated from Doxygen XML.*",
        "",
    ]

    output.write_text(
        "\n".join(lines),
        encoding="utf-8",
    )

    return output


def main() -> None:
    if not XML_DIR.exists():
        print(
            f"ERROR: Doxygen XML directory does not exist: {XML_DIR}",
            file=sys.stderr,
        )
        sys.exit(1)

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    # Remove previously generated pages.
    for file in OUTPUT_DIR.glob("*.md"):
        file.unlink()

    xml_files = sorted(XML_DIR.glob("*.xml"))

    print(f"Found {len(xml_files)} XML files.")

    generated = []

    for xml_file in xml_files:
        try:
            tree = ET.parse(xml_file)
        except ET.ParseError as error:
            print(
                f"WARNING: Could not parse {xml_file}: {error}",
                file=sys.stderr,
            )
            continue

        root = tree.getroot()

        # Doxygen XML files contain:
        #
        # <doxygen>
        #     <compounddef kind="class">
        #         ...
        #     </compounddef>
        # </doxygen>
        #
        # Therefore we must process compounddef, not the root.
        compounds = root.findall("compounddef")

        for compound in compounds:
            output = write_compound(compound)

            if output is not None:
                generated.append(output)

    # API index
    index = OUTPUT_DIR / "API.md"

    lines = [
        "# API Reference",
        "",
        "Automatically generated from the C++ source code using Doxygen.",
        "",
    ]

    if generated:
        lines += [
            "## Classes and structures",
            "",
        ]

        for page in sorted(generated):
            name = page.stem

            lines.append(
                f"- [`{name}`]({page.name})"
            )

        lines.append("")

    else:
        lines += [
            "No classes or structures were found.",
            "",
        ]

    index.write_text(
        "\n".join(lines),
        encoding="utf-8",
    )

    print(f"Generated {len(generated)} API pages.")

    for page in sorted(generated):
        print(f"  {page}")


if __name__ == "__main__":
    main()
