#!/usr/bin/env python3
"""Generate SchemaJson.h from JSON schema files.

Drop-in replacement for GenerateSchemaJsonHeader.ps1 that does not
require PowerShell, so the build works on macOS / Linux without extra deps.

Usage:
    python GenerateSchemaJsonHeader.py <outPath>
"""

import os
import sys
from pathlib import Path


def main():
    if len(sys.argv) < 2:
        print("Usage: GenerateSchemaJsonHeader.py <outPath>", file=sys.stderr)
        sys.exit(1)

    out_path = Path(sys.argv[1])
    out_path.mkdir(parents=True, exist_ok=True)

    header_file = out_path / "SchemaJson.h"
    schema_dir = Path(__file__).parent / "schema"

    schema_files = sorted(schema_dir.glob("*.json"))
    if not schema_files:
        print(f"No schema files found in {schema_dir}", file=sys.stderr)
        sys.exit(1)

    lines = []
    indent = ""

    def w(text="", ind=0):
        lines.append("  " * ind + text + "\r\n")

    # Header
    w("// Copyright (c) Microsoft Corporation. All rights reserved.")
    w("// Licensed under the MIT License.")
    w("// WARNING: This header file was automatically generated")
    w("// by GenerateSchemaJsonHeader.py.")
    w("// Modifying this code by hand is not recommended.")
    w("#pragma once")
    w('#include <string>')
    w('#include <unordered_map>')
    w("namespace Microsoft {", 0)
    w("namespace glTF {", 0)
    w("class SchemaJson {", 0)
    w("public:", 0)

    # Collect schema data
    schemas = {}
    for sf in schema_files:
        content = sf.read_text().strip().strip("\"'")
        var_name = sf.name.replace(".", "_")
        schemas[sf.name] = (var_name, content)
        w(f"static const char* const {var_name};", 1)

    # GLTF_SCHEMA_MAP declaration
    w("static const std::unordered_map<std::string, std::string> GLTF_SCHEMA_MAP;", 1)

    w("}; // end class SchemaJson scope", 0)

    # Define variables
    for name, (var_name, content) in schemas.items():
        # Indent content for readability
        indented = content.replace("\n", "\n  ")
        w(f'const char* const SchemaJson::{var_name} = R"rawstring(', 0)
        w(f"  {indented})rawstring\";", 0)

    # Define GLTF_SCHEMA_MAP
    w("const std::unordered_map<std::string, std::string> SchemaJson::GLTF_SCHEMA_MAP =", 0)
    w("{", 0)
    items = list(schemas.items())
    for i, (name, (var_name, _)) in enumerate(items):
        comma = "," if i < len(items) - 1 else ""
        w(f'    {{ "{name}", {var_name} }}{comma}', 0)
    w("};", 0)

    w("}; // end namespace glTF scope", 0)
    w("}; // end namespace Microsoft scope", 0)

    header_file.write_text("".join(lines))
    print(f"Successfully generated {header_file}")


if __name__ == "__main__":
    main()
