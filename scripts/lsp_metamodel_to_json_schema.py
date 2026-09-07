# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Convert the LSP meta model into a JSON schema.

The meta model published with the language server protocol specification uses
its own type language ("kind": "reference" / "or" / "map" / ...) which
generate_cpp_from_schema.py does not understand. This script rewrites it into
the JSON schema subset that generator consumes.
"""

import argparse
import io
import json
import sys

from pathlib import Path

BASE_TYPES = {
    "URI": {"type": "string"},
    "DocumentUri": {"type": "string"},
    "integer": {"type": "integer"},
    "uinteger": {"type": "integer"},
    "decimal": {"type": "number"},
    "RegExp": {"type": "string"},
    "string": {"type": "string"},
    "boolean": {"type": "boolean"},
    "null": {"type": "null"},
}

# LSPAny is the recursive "any JSON value" type. Modelling it as a variant
# would need a recursive std::variant; an untyped schema maps to QJsonValue.
INLINED_REFERENCES = {
    "LSPAny": {},
    "LSPObject": {"type": "object"},
    "LSPArray": {"type": "array"},
}

def _relative_to_cwd(arg):
    """`arg` relative to the working directory, if it denotes a path below it."""
    try:
        return Path(arg).resolve().relative_to(Path.cwd()).as_posix()
    except (ValueError, OSError):
        return arg

def invocation():
    """The command line to reproduce the generated files.

    The interpreter is a fixed literal and every path is recorded relative to
    the working directory, so that the record does not differ between machines.
    """
    script = _relative_to_cwd(sys.argv[0])
    args = " ".join(_relative_to_cwd(a) for a in sys.argv[1:])
    return f"python3 {script} {args}"

class Converter:
    def __init__(self, model):
        self.structures = {s["name"]: s for s in model["structures"]}
        self.enumerations = {e["name"]: e for e in model["enumerations"]}
        self.aliases = {a["name"]: a for a in model["typeAliases"]}
        self.message_definitions = {}
        self._flattened = {}

    def convert_type(self, t):
        kind = t["kind"]
        if kind == "base":
            return dict(BASE_TYPES[t["name"]])
        if kind == "reference":
            name = t["name"]
            if name in INLINED_REFERENCES:
                return dict(INLINED_REFERENCES[name])
            return {"$ref": "#/definitions/" + name}
        if kind == "array":
            return {"type": "array", "items": self.convert_type(t["element"])}
        if kind == "map":
            return {"type": "object", "additionalProperties": self.convert_type(t["value"])}
        if kind == "or":
            return {"anyOf": [self.convert_type(i) for i in t["items"]]}
        if kind == "and":
            merged = {"type": "object", "properties": {}, "required": []}
            for item in t["items"]:
                if item["kind"] != "reference":
                    raise ValueError("unsupported intersection member: " + item["kind"])
                props, required = self.flattened_properties(item["name"])
                merged["properties"].update(props)
                merged["required"].extend(r for r in required if r not in merged["required"])
            return merged
        if kind == "tuple":
            items = [self.convert_type(i) for i in t["items"]]
            element = items[0] if all(i == items[0] for i in items) else {}
            return {"type": "array", "items": element, "minItems": len(items),
                    "maxItems": len(items)}
        if kind == "literal":
            return self.object_spec(t["value"].get("properties", []))
        if kind == "stringLiteral":
            return {"type": "string", "const": t["value"]}
        if kind == "integerLiteral":
            return {"type": "integer", "const": t["value"]}
        if kind == "booleanLiteral":
            return {"type": "boolean", "const": t["value"]}
        raise ValueError("unknown type kind: " + kind)

    def convert_property(self, prop):
        spec = self.convert_type(prop["type"])
        documentation = prop.get("documentation")
        if documentation and "description" not in spec:
            spec["description"] = documentation
        return spec

    def object_spec(self, properties, documentation=None):
        spec = {
            "type": "object",
            "properties": {p["name"]: self.convert_property(p) for p in properties},
            "required": [p["name"] for p in properties if not p.get("optional")],
        }
        if documentation:
            spec["description"] = documentation
        return spec

    def flattened_properties(self, name):
        """Properties of `name` including those of its extends/mixins bases.

        The meta model expresses inheritance and mixins structurally; the
        generator has no notion of either, so the members are merged into the
        deriving structure. Own properties win over inherited ones.
        """
        if name in self._flattened:
            return self._flattened[name]
        structure = self.structures[name]
        properties, required = {}, []

        def add(props, req):
            properties.update(props)
            required.extend(r for r in req if r not in required)

        for base in structure.get("extends", []) + structure.get("mixins", []):
            if base["kind"] != "reference":
                raise ValueError(name + ": unsupported base kind " + base["kind"])
            add(*self.flattened_properties(base["name"]))
        own = self.object_spec(structure.get("properties", []))
        add(own["properties"], own["required"])
        self._flattened[name] = (properties, required)
        return self._flattened[name]

    def convert_structure(self, name):
        properties, required = self.flattened_properties(name)
        spec = {"type": "object", "properties": dict(properties), "required": list(required)}
        documentation = self.structures[name].get("documentation")
        if documentation:
            spec["description"] = documentation
        return spec

    def convert_enumeration(self, enumeration):
        base = BASE_TYPES[enumeration["type"]["name"]]["type"]
        if enumeration.get("supportsCustomValues"):
            # Values outside the listed set are legal, so a closed C++ enum
            # would reject valid input. The names are still worth having, as
            # constants of the base type.
            spec = {"type": base,
                    "constants": [{"name": v["name"], "value": v["value"]}
                                  for v in enumeration["values"]]}
            namespace = self.constants_namespace(enumeration["name"])
            if namespace:
                spec["constantsNamespace"] = namespace
        elif base == "string":
            spec = {"type": "string", "enum": [v["value"] for v in enumeration["values"]]}
        else:
            # The generator takes the constant names from the item titles.
            spec = {"oneOf": [self._enum_item(base, v) for v in enumeration["values"]]}
        documentation = enumeration.get("documentation")
        if documentation:
            spec["description"] = documentation
        return spec

    def constants_namespace(self, name):
        """Namespace for the constants of an open enumeration, if it needs one.

        A definition that other types refer to has to keep its name for the
        alias, so its constants live next to it under a plural name. One that
        nothing refers to - the semantic token vocabularies are only ever sent
        as bare strings - takes the name for the constants themselves.
        """
        if not self.is_referenced(name):
            return None
        if name.endswith("s"):
            raise ValueError(name + ": referenced open enumeration with a plural name has "
                             "no free name left for its constants")
        return name + "s"

    def is_referenced(self, name):
        """Whether any structure, alias or message refers to `name`."""
        def refers(spec):
            if isinstance(spec, dict):
                if spec.get("kind") == "reference" and spec.get("name") == name:
                    return True
                return any(refers(v) for v in spec.values())
            if isinstance(spec, list):
                return any(refers(i) for i in spec)
            return False

        return any(refers(s) for s in self.structures.values())             or any(refers(a) for a in self.aliases.values())

    @staticmethod
    def _enum_item(base, value):
        item = {"type": base, "const": value["value"], "title": value["name"]}
        if value.get("documentation"):
            item["description"] = value["documentation"]
        return item

    def convert_alias(self, alias):
        spec = self.convert_type(alias["type"])
        documentation = alias.get("documentation")
        if documentation and "description" not in spec:
            spec["description"] = documentation
        return spec

    def definitions(self):
        definitions = {}
        for name in self.structures:
            definitions[name] = self.convert_structure(name)
        for name, enumeration in self.enumerations.items():
            definitions[name] = self.convert_enumeration(enumeration)
        for name, alias in self.aliases.items():
            if name in INLINED_REFERENCES:
                continue
            definitions[name] = self.convert_alias(alias)
        definitions.update(self.message_definitions)
        return definitions

    # C++ types for payloads that do not map to a named schema definition.
    _RAW_PAYLOAD_TYPES = {"LSPAny": "QJsonValue", "LSPObject": "QJsonObject",
                          "LSPArray": "QJsonArray"}

    def payload_type(self, type_spec, fallback_name):
        """C++ type name for a message payload, defining one where needed.

        A payload that is a plain reference reuses that type. Anything else -
        a union, an array, a bare null - has no name in the meta model, so it
        becomes a definition named after the message.
        """
        if type_spec is None:
            return None
        if type_spec["kind"] == "reference":
            name = type_spec["name"]
            return self._RAW_PAYLOAD_TYPES.get(name, name)
        if type_spec["kind"] == "base":
            if type_spec["name"] == "null":
                return None
            return {"string": "QString", "boolean": "bool", "integer": "int",
                    "uinteger": "int", "decimal": "double", "URI": "QString",
                    "DocumentUri": "QString", "RegExp": "QString"}[type_spec["name"]]
        self.message_definitions[fallback_name] = self.convert_type(type_spec)
        return fallback_name

    def messages(self, model):
        """Per-message method name, direction and payload types."""
        self.message_definitions = {}
        result = []
        for kind, entries in (("request", model["requests"]),
                              ("notification", model["notifications"])):
            for entry in entries:
                name = entry["typeName"]
                result.append({
                    "kind": kind,
                    "name": name,
                    "method": entry["method"],
                    "direction": entry["messageDirection"],
                    "documentation": entry.get("documentation"),
                    "params": self.payload_type(entry.get("params"), name + "Params"),
                    "result": self.payload_type(entry.get("result"), name + "Result"),
                    "partialResult": self.payload_type(entry.get("partialResult"),
                                                       name + "PartialResult"),
                    "registrationOptions": self.payload_type(
                        entry.get("registrationOptions"), name + "RegistrationOptions"),
                    "errorData": self.payload_type(entry.get("errorData"),
                                                   name + "ErrorData"),
                })
        return result

DIRECTIONS = {"clientToServer": "ClientToServer", "serverToClient": "ServerToClient",
              "both": "Both"}

def doc_comment(text):
    if not text:
        return []
    lines = ["/**"]
    lines += [" * " + line if line else " *" for line in text.split("\n")]
    lines.append(" */")
    return lines

def messages_header(messages, namespace, types_header, version):
    """A namespace per message, naming its method and payload types."""
    lines = [
        "/*",
        " This file is auto-generated. Do not edit manually.",
        " Generated from the language server protocol meta model " + version + " with:",
        "",
        " " + invocation(),
        "*/",
        "#pragma once",
        "",
        f'#include "{types_header}"',
        "",
        f"namespace {namespace} {{",
        "",
        "enum class MessageDirection { ClientToServer, ServerToClient, Both };",
    ]
    for message in messages:
        lines.append("")
        lines.extend(doc_comment(message["documentation"]))
        lines.append(f"struct {message['name']} {{")
        lines.append(f'    static constexpr char method[] = "{message["method"]}";')
        lines.append("    static constexpr MessageDirection direction = MessageDirection::"
                     f"{DIRECTIONS[message['direction']]};")
        lines.append("    static constexpr bool isRequest = "
                     f"{str(message['kind'] == 'request').lower()};")
        # Absent payloads are std::monostate so that every message trait has
        # the same members and can be used from a template.
        for field, alias in (("params", "Params"), ("result", "Result"),
                             ("partialResult", "PartialResult"),
                             ("registrationOptions", "RegistrationOptions"),
                             ("errorData", "ErrorData")):
            lines.append(f"    using {alias} = {message[field] or 'std::monostate'};")
        lines.append("};")
    lines.append("")
    lines.append(f"}} // namespace {namespace}")
    lines.append("")
    return "\n".join(lines)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("metamodel", help="Path to the LSP metaModel.json")
    parser.add_argument("output", help="Path to the JSON schema to write")
    parser.add_argument("--messages-output",
                        help="Path to a C++ header describing requests and notifications")
    parser.add_argument("--namespace", default="LanguageServerProtocol",
                        help="C++ namespace of the generated types")
    parser.add_argument("--types-header", default="lsptypes.h",
                        help="Header defining the generated types")
    args = parser.parse_args()

    with io.open(args.metamodel, encoding="utf-8") as f:
        model = json.load(f)

    converter = Converter(model)
    messages = converter.messages(model)
    schema = {
        "$schema": "http://json-schema.org/draft-07/schema#",
        "$comment": "This file is auto-generated. Do not edit manually. Generated with: "
                    + invocation(),
        "title": "Language Server Protocol " + model["metaData"]["version"],
        "definitions": converter.definitions(),
    }
    with io.open(args.output, "w", encoding="utf-8", newline="\n") as f:
        json.dump(schema, f, indent=2, ensure_ascii=False)
        f.write("\n")

    if args.messages_output:
        with io.open(args.messages_output, "w", encoding="utf-8", newline="\n") as f:
            f.write(messages_header(messages, args.namespace, args.types_header,
                                    model["metaData"]["version"]))

if __name__ == "__main__":
    main()
