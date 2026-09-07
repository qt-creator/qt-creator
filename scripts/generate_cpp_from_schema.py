# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import argparse
import copy
import json
import re
import sys
from pathlib import Path

# Controls whether doc/inline comments are emitted. Set via --no-comments.
_emit_comments: bool = True

# Controls whether setter/builder methods are emitted. Set via --read-only.
_read_only: bool = False

# Controls whether C++20-only constructs may be used. With coroutines
# available fromJson propagates parse errors through co_await; without them the
# awaited calls are lowered to a temporary and an early return. Set via
# --no-cxx20.
_cxx20: bool = True

# Controls whether optional nullable fields are modeled as three-state
# Patch<T> (absent / null / value) instead of std::optional<T>. Set via
# --three-state. Required for schemas with upsert patch semantics where
# omitted and null carry different meanings (e.g. ACP v2).
_three_state: bool = False

_PATCH_CLASS = '''
/**
 * Three-state field for upsert patch semantics: absent (leave unchanged),
 * null (explicit clear), or a concrete value.
 */
template<typename T>
class Patch
{
public:
    Patch() = default;
    Patch(std::nullopt_t) : m_null(true) {}
    Patch(const T &value) : m_value(value) {}

    bool isAbsent() const { return !m_null && !m_value.has_value(); }
    bool isNull() const { return m_null; }
    bool has_value() const { return m_value.has_value(); }
    const T &operator*() const { return *m_value; }
    const T *operator->() const { return &*m_value; }
    const std::optional<T> &asOptional() const { return m_value; }

    Patch &operator=(std::nullopt_t) { m_null = true; m_value.reset(); return *this; }
    Patch &operator=(const T &value) { m_null = false; m_value = value; return *this; }

    bool operator==(const Patch &other) const = default;

private:
    bool m_null = false;
    std::optional<T> m_value;
};
'''

_RECURSIVE_CLASS = '''
/**
 * Optional value stored indirectly, for types that contain themselves.
 */
template<typename T>
class Recursive
{
public:
    Recursive() = default;
    Recursive(const T &value) : m_value(std::make_unique<T>(value)) {}
    Recursive(const Recursive &other) { *this = other; }
    Recursive(Recursive &&other) = default;

    Recursive &operator=(const Recursive &other)
    {
        if (this != &other)
            m_value = other.m_value ? std::make_unique<T>(*other.m_value) : nullptr;
        return *this;
    }
    Recursive &operator=(Recursive &&other) = default;
    Recursive &operator=(const T &value)
    {
        m_value = std::make_unique<T>(value);
        return *this;
    }

    bool operator==(const Recursive &other) const
    {
        if (!m_value || !other.m_value)
            return !m_value && !other.m_value;
        return *m_value == *other.m_value;
    }

    bool has_value() const { return m_value != nullptr; }
    explicit operator bool() const { return has_value(); }
    const T &operator*() const { return *m_value; }
    const T *operator->() const { return m_value.get(); }

private:
    std::unique_ptr<T> m_value;
};
'''

_INT_JSON_HELPERS = '''
template<>
inline Utils::Result<int> fromJson<int>(const QJsonValue &val)
{
    if (!val.isDouble())
        return Utils::ResultError(QString("Expected a number"));
    return val.toInt();
}

inline QJsonValue toJsonValue(int value) { return value; }
'''

# Maps owner type name -> set of property names declared as Recursive<T>
# because the property type refers back to the owner.
_recursive_fields: dict = {}

# Set when a namespace of integer constants is used as a list element or union
# alternative, where the plain int it degrades to needs its own conversions.
_needs_int_json: bool = False

# Maps variant_type_str -> alias_name for inline union aliases already emitted.
# Prevents redefinition of fromJson/toJsonValue for equivalent variant types.
_emitted_variant_sigs: dict = {}

# Maps alias name -> the type it ultimately denotes. Aliases are typedefs, so
# two variants spelled differently can name the same C++ type and must not
# both define fromJson.
_canonical_alias: dict = {}

def _variant_alias_for(signature: str):
    """The already emitted alias denoting this variant, if any.

    Aliases are typedefs, so a union spelled out in a struct property and a
    named type alias can be the same C++ type and must share their
    serializers.
    """
    return _emitted_variant_sigs.get(_canonical_signature(signature))

def _register_variant_alias(signature: str, name: str) -> None:
    _emitted_variant_sigs[_canonical_signature(signature)] = name
    _canonical_alias[name] = f"std::variant<{_canonical_signature(signature)}>"

def _canonical_signature(signature: str) -> str:
    """Rewrite a variant signature in terms of the types the aliases denote."""
    def canonical(member: str) -> str:
        if member.startswith("QList<") and member.endswith(">"):
            return f"QList<{canonical(member[len('QList<'):-1])}>"
        return _canonical_alias.get(member, member)

    return ", ".join(canonical(m.strip()) for m in signature.split(","))

def _relative_to_cwd(arg: str) -> str:
    """`arg` relative to the working directory, if it denotes a path below it."""
    try:
        return Path(arg).resolve().relative_to(Path.cwd()).as_posix()
    except (ValueError, OSError):
        return arg

def invocation_comment() -> str:
    """The command line to reproduce the generated file.

    The interpreter is a fixed literal and every path is recorded relative to
    the working directory: neither the real interpreter name nor an absolute
    path is the same on two machines, and writing them back would churn the
    generated files for everyone else.
    """
    script = _relative_to_cwd(sys.argv[0])
    args = ' '.join(_relative_to_cwd(a) for a in sys.argv[1:])
    return f" python3 \\\n  {script} \\\n  {args}"

def make_header(namespace: str, export_header: str = None) -> str:
    export_include = f'\n#include "{export_header}"\n' if export_header else ''
    co_result_include = '\n#include <utils/co_result.h>' if _cxx20 else ''
    memory_include = '#include <memory>\n' if _recursive_fields else ''
    return f'''/*
 This file is auto-generated. Do not edit manually.
 Generated with:

{invocation_comment()}
*/
#pragma once
{export_include}
#include <utils/result.h>{co_result_include}

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <cmath>
#include <limits>
{memory_include}#include <variant>

namespace {namespace} {{
{_PATCH_CLASS if _three_state else ''}{_RECURSIVE_CLASS if _recursive_fields else ''}
template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

// Defs that carry no constraints beyond "an object" alias to QJsonObject; these
// let such aliases take part in the generated conversions unchanged.
template<> inline Utils::Result<QJsonObject> fromJson<QJsonObject>(const QJsonValue &val)
{{
    if (!val.isObject())
        return Utils::ResultError(QString("Expected JSON object"));
    return val.toObject();
}}

inline QJsonObject toJson(const QJsonObject &data) {{ return data; }}

template<typename T>
Utils::Result<T> fromJson(const QString &field, const QJsonValue &val)
{{
    const Utils::Result<T> result = fromJson<T>(val);
    if (result)
        return result;
    return Utils::ResultError(field + ": " + result.error());
}}
{_INT_JSON_HELPERS if _needs_int_json else ''}'''

def compute_recursive_fields(types: dict) -> dict:
    """Optional $ref properties whose type refers back to the declaring type.

    Such a member cannot be a ``std::optional<T>``: T is still incomplete
    where it is declared. They are stored in a ``Recursive<T>`` instead, which
    holds the value behind a pointer. Only value positions count; a reference
    below an array or a map is behind a container and stays complete-agnostic.

    A nullable reference (``anyOf`` of a ``$ref`` and ``null``) is an optional
    value position too, and needs the indirection whether or not the property
    is required.
    """
    def value_refs(spec):
        if isinstance(spec, dict):
            if "$ref" in spec:
                yield ref_type(spec["$ref"])
                return
            for key, val in spec.items():
                if key in ("items", "additionalProperties", "description"):
                    continue
                yield from value_refs(val)
        elif isinstance(spec, list):
            for item in spec:
                yield from value_refs(item)

    graph = {name: {r for r in value_refs(spec) if r in types}
             for name, spec in types.items()}

    def reaches(start, target):
        seen, pending = set(), [start]
        while pending:
            current = pending.pop()
            if current == target:
                return True
            if current in seen:
                continue
            seen.add(current)
            pending.extend(graph.get(current, ()))
        return False

    recursive = {}
    for name, spec in types.items():
        required = spec.get("required", [])
        for prop, prop_spec in spec.get("properties", {}).items():
            ref = _extract_ref(prop_spec)
            if ref:
                # A plain reference is only optional when not required; a
                # required one is stored by value and cannot be self-referential.
                if prop in required:
                    continue
                target = ref_type(ref)
            else:
                target = _extract_nullable_ref(prop_spec)
                if not target:
                    continue
            if reaches(target, name):
                recursive.setdefault(name, set()).add(prop)
    return recursive

def _is_recursive_field(owner, prop) -> bool:
    """Whether a property is stored as ``Recursive<T>`` in its owner."""
    return prop in _recursive_fields.get(owner, ())

def uses_int_constant_namespace_indirectly(types: dict) -> bool:
    """Whether a namespace of integer constants appears in a list or a union.

    In those positions the type name is not usable and degrades to plain
    ``int``, which then needs the fromJson/toJsonValue overloads the named
    types would otherwise provide.
    """
    def indirect_refs(spec):
        if isinstance(spec, dict):
            for key, val in spec.items():
                if key == "items":
                    ref = _extract_ref(val) if isinstance(val, dict) else None
                    if ref:
                        yield ref_type(ref)
                    yield from indirect_refs(val)
                elif key in ("anyOf", "oneOf") and isinstance(val, list):
                    for item in val:
                        if not isinstance(item, dict):
                            continue
                        ref = _extract_ref(item)
                        if ref:
                            yield ref_type(ref)
                        yield from indirect_refs(item)
                elif key != "description":
                    yield from indirect_refs(val)
        elif isinstance(spec, list):
            for item in spec:
                yield from indirect_refs(item)

    return any(is_integer_const_namespace(name, types)
               for spec in types.values() for name in indirect_refs(spec))

def make_footer(namespace: str) -> str:
    return f'''
}} // namespace {namespace}
'''

def make_cpp_preamble(namespace: str, header_filename: str) -> str:
    return f'''// This file is auto-generated. Do not edit manually.
#include "{header_filename}"

namespace {namespace} {{
'''

_CO_AWAIT = r'\bco_await\s+'


def _awaited_call(line, start):
    """The call expression awaited at `start`, and the index just past it."""
    depth = 0
    for i in range(line.index('(', start), len(line)):
        if line[i] == '(':
            depth += 1
        elif line[i] == ')':
            depth -= 1
            if depth == 0:
                return line[start:i + 1], i + 1
    raise ValueError(f"Unbalanced parentheses in: {line}")


def _unbraced_body_header(lines):
    """Index of the last emitted line whose body follows unbraced, else None."""
    for i in reversed(range(len(lines))):
        stripped = lines[i].strip()
        if not stripped:
            continue
        if stripped == 'else' or re.match(r'^(if|else if|for|while)\b.*\)$', stripped):
            return i
        return None
    return None


def _indent_of(line):
    return line[:len(line) - len(line.lstrip())]


def lower_co_await(lines):
    """Rewrite each `co_await <call>` as a temporary plus an early return.

    Where the awaiting statement is the unbraced body of an if or a loop, that
    body gains braces so the added statements stay part of it.
    """
    out = []
    awaited = 0
    for line in lines:
        match = re.search(_CO_AWAIT, line)
        if not match:
            out.append(line.replace('co_return', 'return'))
            continue
        header = _unbraced_body_header(out)
        if header is not None:
            out[header] += ' {'
        indent = _indent_of(line)
        while match:
            call, end = _awaited_call(line, match.end())
            tmp = f"res{awaited}"
            awaited += 1
            out.append(f"{indent}const auto {tmp} = {call};")
            out.append(f"{indent}if (!{tmp})")
            out.append(f"{indent}    return Utils::ResultError({tmp}.error());")
            line = line[:match.start()] + f"*{tmp}" + line[end:]
            match = re.search(_CO_AWAIT, line)
        out.append(line.replace('co_return', 'return'))
        if header is not None:
            out.append(_indent_of(out[header]) + '}')
    return out


def finalize_from_json(lines):
    """Adapt an emitted fromJson body to the target C++ standard.

    Without C++20 the coroutine syntax is lowered away entirely; with it, a
    body that never awaits stays a plain function instead of a coroutine.
    Also comments out the 'val' parameter name when it is not referenced in
    the body."""
    if not _cxx20:
        lines = lower_co_await(lines)
    elif not any('co_await' in line for line in lines):
        lines = [line.replace('co_return', 'return') for line in lines]
    # Suppress unused 'val' parameter in fromJson specializations
    if (len(lines) >= 2
            and 'fromJson' in lines[1]
            and 'const QJsonValue &val' in lines[1]
            and not any(re.search(r'\bval\b', line) for line in lines[2:])):
        lines = list(lines)
        lines[1] = lines[1].replace('const QJsonValue &val', 'const QJsonValue & /*val*/')
    return lines


def _escape_comment(text):
    """Break /* and */ so schema prose cannot open or close the block comment."""
    return re.sub(r'/\*|\*/', lambda m: m.group(0)[0] + '\\' + m.group(0)[1], text)


def doc_comment(text, indent=''):
    """Format a description as a /** ... */ Doxygen block comment."""
    if not _emit_comments:
        return ''
    if not text:
        return ''
    text = _escape_comment(text).strip()
    if not text:
        return ''
    lines = text.split('\n')
    while lines and not lines[-1].strip():
        lines.pop()
    if not lines:
        return ''
    if len(lines) == 1 and len(lines[0].strip()) <= 100:
        return f'{indent}/** {lines[0].strip()} */\n'
    result = f'{indent}/**\n'
    for line in lines:
        stripped = line.strip()
        result += f'{indent} * {stripped}\n' if stripped else f'{indent} *\n'
    result += f'{indent} */\n'
    return result


# Schema defs describing arbitrary JSON that are mutually recursive
# (JSONValue -> JSONObject -> JSONValue). C++ cannot express that as value
# types, so $refs to them are rewritten to the equivalent plain JSON specs,
# which map onto QJsonValue/QJsonObject/QJsonArray.
BUILTIN_JSON_DEFS = {
    "JSONValue": {},
    "JSONObject": {"type": "object"},
    "JSONArray": {"type": "array"},
}

def builtin_json_ref(ref, present):
    """The builtin a $ref names, if it points at a top level definition of this
    schema. A pointer that goes deeper, or into another file, names something
    else that happens to end in the same word."""
    parts = ref.split("/") if isinstance(ref, str) else []
    if len(parts) != 3 or parts[0] != "#" or parts[2] not in present:
        return None
    return parts[2]

def inline_builtin_json_refs(node, present):
    """Recursively replace {"$ref": "#/$defs/JSONObject"} and friends with the
    plain JSON spec they stand for, preserving any sibling keys (description)."""
    if isinstance(node, list):
        for item in node:
            inline_builtin_json_refs(item, present)
        return
    if not isinstance(node, dict):
        return
    if builtin := builtin_json_ref(node.get("$ref"), present):
        del node["$ref"]
        for key, value in BUILTIN_JSON_DEFS[builtin].items():
            node.setdefault(key, value)
    for value in node.values():
        inline_builtin_json_refs(value, present)

def cpp_type(json_type):
    mapping = {
        "string": "QString",
        "integer": "int",
        "number": "double",
        "boolean": "bool",
        "object": "QJsonObject",
        "array": "QJsonArray",
        "null": "std::monostate",
    }
    return mapping.get(json_type, json_type)

def ref_type(ref):
    # Assumes refs are like '#/$defs/TypeName'
    return ref.split("/")[-1]

def _extract_anyof_enum(spec):
    """Detect anyOf/oneOf patterns that are really enums (all items share the
    same type and carry a ``const`` value).  Returns a normalised spec dict
    ``{"type": <type>, "enum": [values], ...}`` on match, or ``None``."""
    items = spec.get("anyOf", spec.get("oneOf"))
    if not items:
        return None
    # All items must have the same "type" and most must carry "const"
    types_seen = set()
    consts = []
    for item in items:
        t = item.get("type")
        if not t:
            return None
        types_seen.add(t)
        if "const" in item:
            consts.append(item["const"])
    if len(types_seen) != 1:
        return None
    # Must have at least one const value to be an enum, and at least the
    # items with const must cover the majority; tolerate a single
    # open-ended entry (e.g. "other" without const).
    if not consts or len(consts) < len(items) - 1:
        return None
    the_type = types_seen.pop()
    return {"type": the_type, "enum": consts, "description": spec.get("description", ""),
            "_original_items": items}

def parse_enum(name, spec):
    prefix = doc_comment(spec.get('description', ''))
    # Normalise anyOf/oneOf-with-const into the standard enum form
    normalised = _extract_anyof_enum(spec)
    if normalised:
        spec = normalised
    # Only handle string enums
    if spec.get("type") == "string" and "enum" in spec:
        values = spec["enum"]
        # Use a C++ enum class if all values can be turned into valid identifiers
        sanitized = [sanitize_identifier(v) for v in values]
        valid = all(re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', s) for s in sanitized)
        if valid:
            # Build (sanitized_name, original_json_string) pairs
            pairs = list(zip(sanitized, values))
            lines = [f"enum class {name} {{"]
            lines += [f"    {s}," for s, _ in pairs]
            lines[-1] = lines[-1].rstrip(',')  # Remove trailing comma
            lines.append("};")
            lines.append("")
            # Add conversion helpers
            lines.append(f"inline QString toString({name} v) {{")
            lines.append("    switch(v) {")
            for s, orig in pairs:
                lines.append(f'        case {name}::{s}: return "{orig}";')
            lines.append("    }")
            lines.append("    return {};")
            lines.append("}")
            lines.append("")
            # Parse from QJsonValue
            fj = []
            fj.append(f"template<>")
            fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
            fj.append(f"    if (!val.isString())")
            fj.append(f'        co_return Utils::ResultError("Expected JSON string for {name}");')
            fj.append(f"    const QString str = val.toString();")
            for s, orig in pairs:
                fj.append(f'    if (str == "{orig}") co_return {name}::{s};')
            fj.append(f'    co_return Utils::ResultError("Invalid {name} value: " + str);')
            fj.append("}")
            lines.extend(finalize_from_json(fj))
            lines.append("")
            # For serialization to JSON, use toString
            if not _read_only:
                lines.append(f"inline QJsonValue toJsonValue(const {name} &v) {{")
                lines.append("    return toString(v);")
                lines.append("}")
            return prefix + "\n".join(lines)
        else:
            # Fallback: use QString typedef
            return prefix + f"using {name} = QString;\n"
    # Handle integer enums (e.g. error codes) — generate namespace with constexpr int constants
    if spec.get("type") == "integer" and "enum" in spec:
        lines = [f"namespace {name} {{"]
        # If we came from _extract_anyof_enum, use the original spec items for titles
        orig_items = spec.get("_original_items")
        for val in spec["enum"]:
            # Try to find the title from the original anyOf/oneOf items
            title = None
            if orig_items:
                for item in orig_items:
                    if item.get("const") == val:
                        title = item.get("title", "")
                        break
            if title:
                const_name = sanitize_identifier(title)
            else:
                const_name = f"Code_{str(val).replace('-', 'Neg')}"
            lines.append(f"    constexpr int {const_name} = {val};")
        lines.append(f"}} // namespace {name}")
        return prefix + "\n".join(lines)
    return ""

def is_integer_const_namespace(type_name, types):
    """Check if a type will be generated as a namespace of integer constants (not a C++ type).
    These types should be referenced as 'int' in field declarations."""
    if type_name not in types:
        return False
    spec = types[type_name]
    normalised = _extract_anyof_enum(spec)
    if normalised and normalised.get("type") == "integer":
        return True
    if spec.get("type") == "integer" and "enum" in spec:
        return True
    return False

def ref_cpp_type(ref, types):
    """C++ type for a $ref, mapping constant namespaces to their underlying int."""
    name = ref_type(ref)
    return "int" if is_integer_const_namespace(name, types) else name

def is_simple_type_alias(type_name, types):
    """Check if a type is a simple primitive alias (e.g. using Foo = QString).
    These types convert directly to QJsonValue without needing toJson()."""
    if type_name in types:
        spec = types[type_name]
        return spec.get("type") in ("string", "integer", "number", "boolean") and "enum" not in spec and "properties" not in spec
    return False

def needs_to_json(type_name, types):
    """False for values a QJsonArray or QJsonValue accepts as they are."""
    if type_name in ("QString", "int", "double", "bool"):
        return False
    return not is_simple_type_alias(type_name, types or {})

def json_call(fn, expr):
    """Wrap expr in fn, or leave it alone when no conversion is needed."""
    return f"{fn}({expr})" if fn else expr

def is_enum_type(type_name, types):
    """Check if a type is an enum"""
    if type_name in types:
        spec = types[type_name]
        if "enum" in spec and spec.get("type") == "string":
            return True
        if _extract_anyof_enum(spec) is not None:
            return True
    return False

def is_union_type(spec):
    """Check if a spec represents a union type.

    Types that have 'properties' are generated as structs (even if they also
    contain anyOf/oneOf), so they are NOT union types for serialization purposes.
    anyOf/oneOf patterns that are really enums (all const values of the same
    type) are also NOT union types.
    """
    if "properties" in spec:
        return False
    if _extract_anyof_enum(spec) is not None:
        return False
    # Check for type: ["string", "integer"] pattern
    if isinstance(spec.get("type"), list) and len(spec.get("type", [])) > 1:
        return True
    # Check for anyOf/oneOf patterns
    if "anyOf" in spec or "oneOf" in spec:
        return True
    return False

def is_allof_type(spec):
    """Check if a spec uses allOf composition"""
    return "allOf" in spec

def resolve_allof(spec, types):
    """Resolve allOf by merging properties from all referenced types
    Returns: (merged_props, merged_required) tuple
    """
    if "allOf" not in spec:
        return None, None
    
    merged_props = {}
    merged_required = []
    
    for item in spec["allOf"]:
        if "$ref" in item:
            ref_name = ref_type(item["$ref"])
            if ref_name in types:
                ref_spec = types[ref_name]
                # Recursively resolve if the referenced type also has allOf
                if "allOf" in ref_spec:
                    ref_props, ref_required = resolve_allof(ref_spec, types)
                    if ref_props:
                        merged_props.update(ref_props)
                    if ref_required:
                        merged_required.extend(ref_required)
                elif "properties" in ref_spec:
                    merged_props.update(ref_spec["properties"])
                    if "required" in ref_spec:
                        merged_required.extend(ref_spec["required"])
        elif "properties" in item:
            merged_props.update(item["properties"])
            if "required" in item:
                merged_required.extend(item["required"])
    
    return merged_props, merged_required

def get_const_fields_for_type(type_name, types, visited=None):
    """Get all const string fields for a type, resolving allOf recursively."""
    if visited is None:
        visited = set()
    if type_name in visited:
        return {}
    visited.add(type_name)
    spec = types.get(type_name, {})
    result = {}
    for fname, fspec in spec.get('properties', {}).items():
        if fspec.get('type') == 'string' and 'const' in fspec:
            result[fname] = fspec['const']
    if 'allOf' in spec:
        for item in spec['allOf']:
            if '$ref' in item:
                result.update(get_const_fields_for_type(ref_type(item['$ref']), types, visited))
    return result

def find_dispatch_field(variant_type_names, types):
    """Find a const field common to all variants with unique values, suitable for dispatch.
    Returns (field_name, {type_name: const_value}) or (None, None).
    """
    all_consts = []
    for type_name in variant_type_names:
        consts = get_const_fields_for_type(type_name, types)
        if not consts:
            return None, None
        all_consts.append(consts)
    common_fields = set(all_consts[0].keys())
    for consts in all_consts[1:]:
        common_fields &= set(consts.keys())
    for field in common_fields:
        values = [c[field] for c in all_consts]
        if len(set(values)) == len(values):
            dispatch = {variant_type_names[i]: all_consts[i][field] for i in range(len(variant_type_names))}
            return field, dispatch
    return None, None

def get_required_fields_for_type(type_name, types, visited=None):
    """Get all required fields for a type, resolving allOf recursively."""
    if visited is None:
        visited = set()
    if type_name in visited:
        return set()
    visited.add(type_name)
    spec = types.get(type_name, {})
    result = set(spec.get('required', []))
    if 'allOf' in spec:
        for item in spec['allOf']:
            if '$ref' in item:
                result |= get_required_fields_for_type(ref_type(item['$ref']), types, visited)
    return result

def get_all_fields_for_type(type_name, types, visited=None):
    """Get all defined property names for a type, resolving allOf recursively."""
    if visited is None:
        visited = set()
    if type_name in visited:
        return set()
    visited.add(type_name)
    spec = types.get(type_name, {})
    result = set(spec.get('properties', {}).keys())
    if 'allOf' in spec:
        for item in spec['allOf']:
            if '$ref' in item:
                result |= get_all_fields_for_type(ref_type(item['$ref']), types, visited)
    return result

def find_shared_fields(variant_type_names, types):
    """Find fields that are required in ALL variants with the same non-const type.
    Returns a list of (field_name, cpp_return_type) in sorted order.
    """
    if not variant_type_names:
        return []
    # Alternatives that are not generated structs have no fields to share.
    if not all("properties" in types.get(n, {}) or "allOf" in types.get(n, {})
               for n in variant_type_names):
        return []

    def field_info(type_name, field_name):
        """Return the cpp type string for a field, or None if const/unknown/inline-object."""
        spec = types.get(type_name, {})
        props = spec.get('properties', {})
        # Also check allOf
        if field_name not in props and 'allOf' in spec:
            merged, _ = resolve_allof(spec, types)
            if merged:
                props = merged
        fspec = props.get(field_name)
        if fspec is None:
            return None
        # Skip const string fields (they are not stored in the struct)
        if fspec.get('type') == 'string' and 'const' in fspec:
            return None
        # Skip inline objects that will become typed sub-structs (names differ per parent)
        if (fspec.get('type') == 'object' and '$ref' not in fspec and fspec.get('properties')):
            return None
        if '$ref' in fspec:
            return ref_type(fspec['$ref'])
        t = fspec.get('type')
        if isinstance(t, str):
            return cpp_type(t)
        return None

    # Collect required fields for each variant
    all_required = {n: get_required_fields_for_type(n, types) for n in variant_type_names}
    # Candidates: fields required by every variant
    common_required = set(all_required[variant_type_names[0]])
    for n in variant_type_names[1:]:
        common_required &= all_required[n]

    result = []
    for field in sorted(common_required):
        types_per_variant = [field_info(n, field) for n in variant_type_names]
        # All must resolve to the same non-None type
        if all(t is not None for t in types_per_variant) and len(set(types_per_variant)) == 1:
            result.append((field, types_per_variant[0]))
    return result

def find_presence_dispatch(variant_type_names, types):
    """For each variant, find a required field that is not a property of any other variant.
    Returns a dict {type_name: unique_field}. Types without a unique field are absent (try-each fallback).
    """
    all_required = {n: get_required_fields_for_type(n, types) for n in variant_type_names}
    all_fields   = {n: get_all_fields_for_type(n, types)      for n in variant_type_names}
    unique_field = {}
    for name in variant_type_names:
        for f in sorted(all_required[name]):  # sorted for determinism
            if all(f not in all_fields[other] for other in variant_type_names if other != name):
                unique_field[name] = f
                break
    return unique_field

def _extract_ref(item):
    """Extract a $ref string from a schema item.

    Handles both direct refs ({"$ref": "..."}) and refs wrapped in a single-
    element allOf ({"allOf": [{"$ref": "..."}], ...}) which is a common JSON
    Schema pattern for attaching extra metadata (title, description) to a
    reference.
    """
    if "$ref" in item:
        return item["$ref"]
    allof = item.get("allOf", [])
    refs = [entry["$ref"] for entry in allof if "$ref" in entry]
    if len(refs) == 1:
        return refs[0]
    return None

def _extract_nullable_ref(spec):
    """Detect anyOf patterns of [{$ref: ...}, {type: "null"}] — a nullable reference.

    Returns the $ref type name string on match, or None.
    """
    anyof = spec.get("anyOf", [])
    if len(anyof) != 2:
        return None
    ref_items = [item for item in anyof if _extract_ref(item)]
    null_items = [item for item in anyof if item.get("type") == "null"]
    if len(ref_items) == 1 and len(null_items) == 1:
        return ref_type(_extract_ref(ref_items[0]))
    return None

def _infer_discriminator_field(items):
    """Infer a discriminator field from anyOf/oneOf items.

    If every item has an inline property with a const value and they all share
    the same property name, that property is the implicit discriminator.
    Items without inline properties (e.g. bare $ref with a title) are allowed
    as long as at least some items define the const field.
    """
    const_fields_per_item = []
    for item in items:
        inline_props = item.get("properties", {})
        const_fields = {}
        for prop_name, prop_spec in inline_props.items():
            if "const" in prop_spec:
                const_fields[prop_name] = prop_spec["const"]
        const_fields_per_item.append(const_fields)

    # Find field names that appear with const values in at least 2 items
    from collections import Counter
    field_counts = Counter()
    for fields in const_fields_per_item:
        for field_name in fields:
            field_counts[field_name] += 1

    # The discriminator must appear in all items that have inline properties
    items_with_props = sum(1 for fields in const_fields_per_item if fields)
    for field_name, count in field_counts.most_common():
        if count >= items_with_props and count >= 2:
            return field_name
    return None

def _parse_discriminated_union(name, spec, types=None):
    """Generate code for a discriminated union with a discriminator field.

    Supports two patterns:
    1. Explicit: "discriminator": {"propertyName": "kind"} with oneOf items
    2. Implicit: anyOf/oneOf items that all share an inline property with const values

    Returns (code_string, variant_type_str) or None if not applicable.
    """
    disc_info = spec.get("discriminator")
    disc_field = disc_info.get("propertyName") if disc_info else None
    items = spec.get("oneOf", []) or spec.get("anyOf", [])
    if not items:
        return None

    # If no explicit discriminator, try to infer one from inline const properties
    if not disc_field:
        disc_field = _infer_discriminator_field(items)
    if not disc_field:
        return None

    # Extract (ref_type_name, discriminator_const_value) for each oneOf item
    variants = []  # list of (cpp_type_name, disc_value, item_spec)
    has_fallback = False
    optional_disc_variants = []  # variants that declare the value but do not require the field
    for item in items:
        ref = _extract_ref(item)
        if ref is None and "not" in item:
            # Open-union catch-all ({"title": "other", "not": {...}}):
            # unknown discriminator values are preserved as raw QJsonObject.
            has_fallback = True
            continue
        # Get the discriminator const value from inline properties
        disc_val = None
        inline_props = item.get("properties", {})
        disc_prop = inline_props.get(disc_field, {})
        if "const" in disc_prop:
            disc_val = disc_prop["const"]
        elif item.get("title"):
            # Fallback: use item title as discriminator value
            disc_val = item["title"]

        if ref and disc_val:
            variants.append((ref_type(ref), disc_val, item))
            if disc_field in inline_props and disc_field not in item.get("required", []):
                optional_disc_variants.append(ref_type(ref))
        elif not ref and disc_val and item.get("properties"):
            # Inline object variant (like the "cancelled" variant in RequestPermissionOutcome)
            # This is an inline struct with only the discriminator const field
            # Check if it has any non-const properties
            non_const_props = {k: v for k, v in inline_props.items()
                               if not (v.get("type") == "string" and "const" in v)}
            if not non_const_props:
                # Pure discriminator-only variant — no payload struct
                variants.append((None, disc_val, item))
            else:
                return None  # complex inline — bail to normal parse_union
        else:
            return None  # can't handle this item

    if not variants:
        return None

    # Collect unique C++ type names (excluding None for inline-only variants)
    cpp_types = [v[0] for v in variants if v[0] is not None]
    unique_cpp_types = list(dict.fromkeys(cpp_types))  # preserve order, dedupe
    has_duplicates = len(cpp_types) != len(unique_cpp_types)
    has_inline_only = any(v[0] is None for v in variants)

    # A variant that names its discriminator value without requiring the field
    # is what a payload that omits the field decodes to.
    default_type = optional_disc_variants[0] \
        if len(optional_disc_variants) == 1 and len(variants) > 1 else None

    # Check if we need a wrapper struct (duplicates exist or inline-only variants)
    if has_duplicates or has_inline_only:
        return _gen_discriminated_wrapper_struct(name, disc_field, variants, unique_cpp_types, spec, types,
                                                 has_fallback=has_fallback)
    else:
        # No duplicates — generate a using alias with discriminator-based dispatch
        return _gen_discriminated_alias(name, disc_field, variants, unique_cpp_types, spec, types,
                                        has_fallback=has_fallback, default_type=default_type)


def _gen_discriminated_wrapper_struct(name, disc_field, variants, unique_cpp_types, spec, types,
                                      has_fallback=False):
    """Generate a wrapper struct for discriminated unions with duplicate types or inline-only variants."""
    prefix = doc_comment(spec.get('description', ''))
    lines = []

    # Build the variant type
    # Include std::monostate if there are inline-only (no-payload) variants
    has_inline_only = any(v[0] is None for v in variants)
    variant_members = list(unique_cpp_types)
    if has_inline_only:
        variant_members.insert(0, "std::monostate")
    if has_fallback and "QJsonObject" not in variant_members:
        variant_members.append("QJsonObject")
    variant_type_str = ", ".join(variant_members)

    lines.append(f"struct {name} {{")
    lines.append(f"    using Variant = std::variant<{variant_type_str}>;")
    lines.append(f"    Variant _value;")
    lines.append(f"    QString _kind;  //!< discriminator value ({disc_field})")
    lines.append(f"")
    lines.append(f"    template<typename T> const T* get() const {{ return std::get_if<T>(&_value); }}")
    lines.append(f"    const QString& kind() const {{ return _kind; }}")
    lines.append(f"}};")
    lines.append(f"")

    # fromJson
    fj = []
    fj.append(f"template<>")
    fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
    fj.append(f"    if (!val.isObject())")
    fj.append(f'        co_return Utils::ResultError("Invalid {name}: expected object");')
    fj.append(f"    const QJsonObject obj = val.toObject();")
    fj.append(f"    const QString kind = obj.value(\"{disc_field}\").toString();")
    fj.append(f"    {name} result;")
    fj.append(f"    result._kind = kind;")

    first = True
    for cpp_type_name, disc_val, item in variants:
        kw = "if" if first else "else if"
        first = False
        if cpp_type_name is not None:
            fj.append(f"    {kw} (kind == \"{disc_val}\")")
            fj.append(f"        result._value = co_await fromJson<{cpp_type_name}>(val);")
        else:
            fj.append(f"    {kw} (kind == \"{disc_val}\")")
            fj.append(f"        result._value = std::monostate{{}};")

    if has_fallback:
        fj.append(f"    else if (kind.isEmpty())")
        fj.append(f'        co_return Utils::ResultError("Invalid {name}: missing {disc_field}");')
        fj.append(f"    else")
        fj.append(f"        result._value = obj;  // open union: preserve unknown variants raw")
    else:
        fj.append(f"    else")
        fj.append(f'        co_return Utils::ResultError("Invalid {name}: unknown {disc_field} \\"" + kind + "\\"");')
    fj.append(f"    co_return result;")
    fj.append(f"}}")
    lines.extend(finalize_from_json(fj))
    lines.append(f"")

    # toJson
    if not _read_only:
        lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
        lines.append(f"    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {{")
        if has_inline_only or has_fallback:
            lines.append(f"        using T = std::decay_t<decltype(v)>;")
            if has_inline_only:
                lines.append(f"        if constexpr (std::is_same_v<T, std::monostate>) return {{}};")
            if has_fallback:
                kw = "else if constexpr" if has_inline_only else "if constexpr"
                lines.append(f"        {kw} (std::is_same_v<T, QJsonObject>) return v;")
            lines.append(f"        else return toJson(v);")
        else:
            lines.append(f"        return toJson(v);")
        lines.append(f"    }}, data._value);")
        lines.append(f"    obj.insert(\"{disc_field}\", data._kind);")
        lines.append(f"    return obj;")
        lines.append(f"}}")
        lines.append(f"")
        lines.append(f"inline QJsonValue toJsonValue(const {name} &val) {{")
        lines.append(f"    return toJson(val);")
        lines.append(f"}}")

    return prefix + "\n".join(lines), variant_type_str


def _gen_discriminated_alias(name, disc_field, variants, unique_cpp_types, spec, types,
                             has_fallback=False, default_type=None):
    """Generate a using alias with discriminator-based dispatch for unions without duplicate types."""
    prefix = doc_comment(spec.get('description', ''))
    lines = []
    variant_members = list(unique_cpp_types)
    if has_fallback and "QJsonObject" not in variant_members:
        variant_members.append("QJsonObject")
    variant_type_str = ", ".join(variant_members)

    lines.append(f"using {name} = std::variant<{variant_type_str}>;")
    lines.append(f"")

    # fromJson with discriminator dispatch
    fj = []
    fj.append(f"template<>")
    fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
    fj.append(f"    if (!val.isObject())")
    fj.append(f'        co_return Utils::ResultError("Invalid {name}: expected object");')
    fj.append(f"    const QString dispatchValue = val.toObject().value(\"{disc_field}\").toString();")

    first = True
    for cpp_type_name, disc_val, item in variants:
        kw = "if" if first else "else if"
        first = False
        fj.append(f"    {kw} (dispatchValue == \"{disc_val}\")")
        fj.append(f"        co_return {name}(co_await fromJson<{cpp_type_name}>(val));")

    if default_type:
        fj.append(f"    if (dispatchValue.isEmpty())")
        fj.append(f"        co_return {name}(co_await fromJson<{default_type}>(val));")
    if has_fallback:
        if not default_type:
            fj.append(f"    if (dispatchValue.isEmpty())")
            fj.append(f'        co_return Utils::ResultError("Invalid {name}: missing {disc_field}");')
        fj.append(f"    co_return {name}(val.toObject());  // open union: preserve unknown variants raw")
    else:
        fj.append(f'    co_return Utils::ResultError("Invalid {name}: unknown {disc_field} \\"" + dispatchValue + "\\"");')
    fj.append(f"}}")
    lines.extend(finalize_from_json(fj))
    lines.append(f"")

    # dispatchValue helper (must come before toJson since toJson uses it)
    if _emit_comments:
        lines.append(f"/** Returns the '{disc_field}' dispatch field value for the active variant. */")
    lines.append(f"inline QString dispatchValue(const {name} &val) {{")
    lines.append(f"    return std::visit([](const auto &v) -> QString {{")
    lines.append(f"        using T = std::decay_t<decltype(v)>;")
    # Build dispatch map: for each unique type, find its disc_val
    type_to_disc = {}
    for cpp_type_name, disc_val, item in variants:
        if cpp_type_name not in type_to_disc:
            type_to_disc[cpp_type_name] = disc_val
    first = True
    for cpp_type_name, disc_val in type_to_disc.items():
        kw = "if constexpr" if first else "else if constexpr"
        first = False
        lines.append(f"        {kw} (std::is_same_v<T, {cpp_type_name}>) return \"{disc_val}\";")
    if has_fallback:
        kw = "if constexpr" if first else "else if constexpr"
        lines.append(f"        {kw} (std::is_same_v<T, QJsonObject>) return v.value(\"{disc_field}\").toString();")
    lines.append(f"        return {{}};")
    lines.append(f"    }}, val);")
    lines.append(f"}}")
    lines.append(f"")

    # toJson — re-insert the discriminator field
    if not _read_only:
        lines.append(f"inline QJsonObject toJson(const {name} &val) {{")
        lines.append(f"    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {{")
        lines.append(f"        using T = std::decay_t<decltype(v)>;")
        lines.append(f"        if constexpr (std::is_same_v<T, QJsonObject>) {{")
        lines.append(f"            return v;")
        lines.append(f"        }} else {{")
        lines.append(f"            return toJson(v);")
        lines.append(f"        }}")
        lines.append(f"    }}, val);")
        lines.append(f"    obj.insert(\"{disc_field}\", dispatchValue(val));")
        lines.append(f"    return obj;")
        lines.append(f"}}")
        lines.append(f"")
        lines.append(f"inline QJsonValue toJsonValue(const {name} &val) {{")
        lines.append(f"    return toJson(val);")
        lines.append(f"}}")

    # Shared field accessors (not for open unions: the raw QJsonObject
    # fallback alternative has no typed fields)
    ref_names = unique_cpp_types
    if types and not has_fallback:
        for field, ret_type in find_shared_fields(ref_names, types):
            lines.append(f"")
            if _emit_comments:
                lines.append(f"/** Returns the '{field}' field from the active variant. */")
            lines.append(f"inline {ret_type} {field}(const {name} &val) {{")
            lines.append(f"    return std::visit([](const auto &v) -> {ret_type} {{ return v._{field}; }}, val);")
            lines.append(f"}}")

    return prefix + "\n".join(lines), variant_type_str


def _integral_only_guard(name):
    """Body of an `if (val.isDouble())` branch that only claims integral values.

    JSON has a single number type, so a variant offering both `integer` and
    `number` would otherwise always pick the integer alternative and truncate
    fractional values. Values outside the range of `int` fall through to the
    number alternative, which keeps them instead of truncating to garbage.
    """
    return [
        f"        const double d = val.toDouble();",
        f"        if (d == std::trunc(d)",
        f"                && d >= double(std::numeric_limits<int>::min())",
        f"                && d <= double(std::numeric_limits<int>::max())) {{",
        f"            co_return {name}(static_cast<int>(d));",
        f"        }}",
    ]

def _name_array_of_union_items(spec, types, emit):
    """Give a name to array alternatives whose element type is itself a union.

    ``(Command | CodeAction)[] | null`` has no name for the element type in the
    schema, so the element would degrade to QJsonValue. The element union
    becomes its own alias, named after its members so that two schemas
    describing the same union share it, and the item spec is rewritten to
    reference that alias. Returns (rewritten_spec, extra_code_lines).
    """
    items = spec.get("anyOf", spec.get("oneOf"))
    if not items:
        return spec, []
    extra = []
    rewritten = None
    for index, item in enumerate(items):
        if item.get("type") != "array":
            continue
        element = item.get("items", {})
        element_items = element.get("anyOf", element.get("oneOf", []))
        if not element_items:
            continue
        refs = [ref_cpp_type(r, types or {}) for e in element_items if (r := _extract_ref(e))]
        if not refs or len(refs) != len(element_items):
            continue
        signature = ", ".join(refs)
        alias = "Or".join(refs)
        if emit and not _variant_alias_for(signature):
            extra.extend(_build_inline_union_code(alias, refs, types))
            _register_variant_alias(signature, alias)
        if rewritten is None:
            rewritten = copy.deepcopy(spec)
        key = "anyOf" if "anyOf" in spec else "oneOf"
        rewritten[key][index]["items"] = {"$ref": f"#/definitions/{alias}"}
    return (rewritten or spec), extra

def _union_ref_branches(name, ref_names, types):
    """fromJson branches trying the $ref members that are unions themselves, so
    that a value the object dispatch cannot handle still finds its alternative."""
    lines = []
    for ref_name in ref_names:
        if not is_union_type_name(ref_name, types or {}):
            continue
        lines.append(f"    {{")
        lines.append(f"        auto result = fromJson<{ref_name}>(val);")
        lines.append(f"        if (result) co_return {name}(*result);")
        lines.append(f"    }}")
    return lines

def _plain_item_branches(name, plain_items):
    """fromJson branches for the non-$ref members of a union, which the $ref
    dispatch below cannot recognise."""
    lines = []
    for item in plain_items:
        json_type = item["type"]
        if json_type == "array":
            items_ref = _extract_ref(item.get("items", {}))
            if items_ref:
                elem_type = ref_type(items_ref)
                lines.append(f"    if (val.isArray()) {{")
                lines.append(f"        bool ok = true;")
                lines.append(f"        QList<{elem_type}> list;")
                lines.append(f"        for (const auto &elem : val.toArray()) {{")
                lines.append(f"            auto r = fromJson<{elem_type}>(elem);")
                lines.append(f"            if (!r) {{ ok = false; break; }}")
                lines.append(f"            list.append(*r);")
                lines.append(f"        }}")
                lines.append(f"        if (ok) co_return {name}(std::move(list));")
                lines.append(f"    }}")
                continue
            item_t = item.get("items", {}).get("type")
            elem_cpp = cpp_type(item_t) if item_t else None
            elem_expr = _json_extract_expr(elem_cpp, "elem") if elem_cpp else None
            if elem_expr:
                lines.append(f"    if (val.isArray()) {{")
                lines.append(f"        QList<{elem_cpp}> list;")
                lines.append(f"        for (const auto &elem : val.toArray())")
                lines.append(f"            list.append({elem_expr});")
                lines.append(f"        co_return {name}(std::move(list));")
                lines.append(f"    }}")
            else:
                lines.append(f"    if (val.isArray())")
                lines.append(f"        co_return {name}(val.toArray());")
        elif json_type == "string":
            lines.append(f"    if (val.isString())")
            lines.append(f"        co_return {name}(val.toString());")
        elif json_type == "integer":
            lines.append(f"    if (val.isDouble())")
            lines.append(f"        co_return {name}(val.toInt());")
        elif json_type == "number":
            lines.append(f"    if (val.isDouble())")
            lines.append(f"        co_return {name}(val.toDouble());")
        elif json_type == "boolean":
            lines.append(f"    if (val.isBool())")
            lines.append(f"        co_return {name}(val.toBool());")
        elif json_type == "null":
            lines.append(f"    if (val.isNull())")
            lines.append(f"        co_return {name}(std::monostate{{}});")
    return lines

def _not_object_guard(name, nested_union_branches):
    """Emit the guard rejecting non-object input of a union of objects.

    The non-object alternatives are tried inside the guard, so it only needs a
    block when there are any."""
    error = f'        co_return Utils::ResultError("Invalid {name}: expected object");'
    if not nested_union_branches:
        return ["    if (!val.isObject())", error]
    return (["    if (!val.isObject()) {"]
            + [f"    {line}" for line in nested_union_branches]
            + [error, "    }"])

def parse_union(name, spec, skip_to_json=False, skip_from_json=False, types=None,
                emit_item_unions=True):
    """Generate code for union types (std::variant)
    skip_to_json: if True, skip generating toJsonValue function (for duplicate signatures)
    skip_from_json: if True, skip generating fromJson specialization (for duplicate variant signatures)
    """
    spec, item_union_lines = _name_array_of_union_items(spec, types, emit_item_unions)
    prefix = doc_comment(spec.get('description', ''))
    lines = list(item_union_lines)
    variant_types = []
    
    # Handle type: ["string", "integer"] pattern
    if isinstance(spec.get("type"), list):
        for json_type in spec["type"]:
            variant_types.append(cpp_type(json_type))
    
    # Handle anyOf/oneOf patterns
    elif "anyOf" in spec or "oneOf" in spec:
        for item in spec.get("anyOf", spec.get("oneOf", [])):
            ref = _extract_ref(item)
            if ref:
                variant_types.append(ref_cpp_type(ref, types or {}))
            elif "type" in item:
                json_type = item["type"]
                if json_type == "array" and "items" in item:
                    # Array item with typed elements, e.g. QList<RefType>
                    items_ref = _extract_ref(item["items"])
                    if items_ref:
                        variant_types.append(f"QList<{ref_type(items_ref)}>")
                    else:
                        variant_types.append(f"QList<{cpp_type(item['items'].get('type', 'QJsonValue'))}>")
                else:
                    variant_types.append(cpp_type(json_type))
    
    # Deduplicate variant types (preserve order) — std::variant with
    # duplicate types is ill-formed in C++.
    seen = set()
    deduped = []
    for vt in variant_types:
        if vt not in seen:
            seen.add(vt)
            deduped.append(vt)
    variant_types = deduped

    if not variant_types:
        return "", ""
    
    # Generate using declaration
    variant_type_str = ", ".join(variant_types)
    lines.append(f"using {name} = std::variant<{variant_type_str}>;")
    lines.append("")

    # Collect the fromJson specialization into a separate buffer so it can be
    # skipped when an identical variant signature was already emitted.
    fj = []
    fj.append(f"template<>")
    fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")

    # For simple type unions (string/integer)
    if isinstance(spec.get("type"), list):
        int_and_number = "integer" in spec["type"] and "number" in spec["type"]
        for i, json_type in enumerate(spec["type"]):
            cpp_t = cpp_type(json_type)
            if json_type == "string":
                fj.append(f"    if (val.isString()) {{")
                fj.append(f"        co_return {name}(val.toString());")
                fj.append(f"    }}")
            elif json_type == "integer":
                fj.append(f"    if (val.isDouble()) {{")
                if int_and_number:
                    fj.extend(_integral_only_guard(name))
                else:
                    fj.append(f"        co_return {name}(val.toInt());")
                fj.append(f"    }}")
            elif json_type == "number":
                fj.append(f"    if (val.isDouble()) {{")
                fj.append(f"        co_return {name}(val.toDouble());")
                fj.append(f"    }}")
            elif json_type == "boolean":
                fj.append(f"    if (val.isBool()) {{")
                fj.append(f"        co_return {name}(val.toBool());")
                fj.append(f"    }}")

    # For anyOf/oneOf - dispatch on const field if possible, else try each
    elif "anyOf" in spec or "oneOf" in spec:
        items = spec.get("anyOf", spec.get("oneOf", []))
        ref_names = [ref_cpp_type(r, types or {}) for item in items if (r := _extract_ref(item))]
        # Collect plain-type items (non-$ref items with a "type" field)
        plain_items = [item for item in items if not _extract_ref(item) and "type" in item]
        # Open-union catch-all ({"title": "other", "not": {...}}), matching the
        # detection in _parse_discriminated_union — not just any object-typed
        # variant, which would otherwise wrongly suppress validation and
        # shared field accessors for ordinary object unions.
        has_open_union_fallback = any(
            _extract_ref(item) is None and "not" in item for item in items)

        # If all items are plain types (no $refs), generate primitive-style fromJson
        if plain_items and not ref_names:
            plain_types = {item["type"] for item in plain_items}
            int_and_number = "integer" in plain_types and "number" in plain_types
            for item in plain_items:
                json_type = item["type"]
                if json_type == "string":
                    fj.append(f"    if (val.isString())")
                    fj.append(f"        co_return {name}(val.toString());")
                elif json_type == "integer":
                    if int_and_number:
                        fj.append(f"    if (val.isDouble()) {{")
                        fj.extend(_integral_only_guard(name))
                        fj.append(f"    }}")
                    else:
                        fj.append(f"    if (val.isDouble())")
                        fj.append(f"        co_return {name}(static_cast<int>(val.toDouble()));")
                elif json_type == "number":
                    fj.append(f"    if (val.isDouble())")
                    fj.append(f"        co_return {name}(val.toDouble());")
                elif json_type == "boolean":
                    fj.append(f"    if (val.isBool())")
                    fj.append(f"        co_return {name}(val.toBool());")
                elif json_type == "null":
                    fj.append(f"    if (val.isNull())")
                    fj.append(f"        co_return {name}(std::monostate{{}});")
                elif json_type == "array":
                    items_ref = _extract_ref(item.get("items", {}))
                    if items_ref:
                        elem_type = ref_type(items_ref)
                        list_type = f"QList<{elem_type}>"
                        # Use try-each pattern: multiple array variants are
                        # ambiguous by JSON type, so attempt parsing and fall
                        # through on failure.
                        fj.append(f"    if (val.isArray()) {{")
                        fj.append(f"        bool ok = true;")
                        fj.append(f"        {list_type} list;")
                        fj.append(f"        for (const auto &elem : val.toArray()) {{")
                        fj.append(f"            auto r = fromJson<{elem_type}>(elem);")
                        fj.append(f"            if (!r) {{ ok = false; break; }}")
                        fj.append(f"            list.append(*r);")
                        fj.append(f"        }}")
                        fj.append(f"        if (ok) co_return {name}(std::move(list));")
                        fj.append(f"    }}")
                    else:
                        item_t = item.get("items", {}).get("type")
                        elem_cpp = cpp_type(item_t) if item_t else None
                        elem_expr = _json_extract_expr(elem_cpp, "elem") if elem_cpp else None
                        if elem_expr:
                            fj.append(f"    if (val.isArray()) {{")
                            fj.append(f"        QList<{elem_cpp}> list;")
                            fj.append(f"        for (const auto &elem : val.toArray())")
                            fj.append(f"            list.append({elem_expr});")
                            fj.append(f"        co_return {name}(std::move(list));")
                            fj.append(f"    }}")
                        else:
                            fj.append(f"    if (val.isArray())")
                            fj.append(f"        co_return {name}(val.toArray());")

        elif ref_names:
            # A union of objects can still have array or scalar members; those
            # never match the object dispatch below, so they are tried first.
            fj.extend(_plain_item_branches(name, plain_items))
            nested_union_branches = _union_ref_branches(name, ref_names, types)
            dispatch_field, dispatch_map = find_dispatch_field(ref_names, types) if types else (None, None)
            if dispatch_field:
                fj.extend(_not_object_guard(name, nested_union_branches))
                fj.append(f"    const QString dispatchValue = val.toObject().value(\"{dispatch_field}\").toString();")
                first = True
                for ref_name, const_val in dispatch_map.items():
                    kw = "if" if first else "else if"
                    first = False
                    fj.append(f"    {kw} (dispatchValue == \"{const_val}\")")
                    fj.append(f"        co_return {name}(co_await fromJson<{ref_name}>(val));")
                if has_open_union_fallback:
                    fj.append(f"    co_return {name}(val.toObject());  // open union: preserve unknown variants raw")
                else:
                    fj.append(f"    co_return Utils::ResultError(\"Invalid {name}: unknown {dispatch_field} \\\"\" + dispatchValue + \"\\\"\");")
                fj.append("}")  # close fromJson function
                if not skip_from_json:
                    lines.extend(finalize_from_json(fj))
                if not skip_to_json:
                    lines.append("")
                    lines.append(f"inline QJsonObject toJson(const {name} &val) {{")
                    lines.append("    return std::visit([](const auto &v) -> QJsonObject {")
                    lines.append("        using T = std::decay_t<decltype(v)>;")
                    lines.append("        if constexpr (std::is_same_v<T, QJsonObject>) {")
                    lines.append("            return v;")
                    lines.append("        } else {")
                    lines.append("            return toJson(v);")
                    lines.append("        }")
                    lines.append("    }, val);")
                    lines.append("}")
                    lines.append("")
                    lines.append(f"inline QJsonValue toJsonValue(const {name} &val) {{")
                    lines.append("    return toJson(val);")
                    lines.append("}")
                # Always emit dispatchValue — it's named per union type so no duplication risk
                lines.append("")
                if _emit_comments:
                    lines.append(f"/** Returns the '{dispatch_field}' dispatch field value for the active variant. */")
                lines.append(f"inline QString dispatchValue(const {name} &val) {{")
                lines.append("    return std::visit([](const auto &v) -> QString {")
                lines.append("        using T = std::decay_t<decltype(v)>;")
                first = True
                for ref_name, const_val in dispatch_map.items():
                    kw = "if constexpr" if first else "else if constexpr"
                    first = False
                    lines.append(f"        {kw} (std::is_same_v<T, {ref_name}>) return \"{const_val}\";")
                if has_open_union_fallback:
                    kw = "if constexpr" if first else "else if constexpr"
                    lines.append(f"        {kw} (std::is_same_v<T, QJsonObject>) return v.value(\"{dispatch_field}\").toString();")
                lines.append("        return {};")
                lines.append("    }, val);")
                lines.append("}")
                shared_fields = [] if has_open_union_fallback or skip_to_json \
                    or len(ref_names) != len(items) else find_shared_fields(ref_names, types)
                for field, ret_type in shared_fields:
                    lines.append("")
                    if _emit_comments:
                        lines.append(f"/** Returns the '{field}' field from the active variant. */")
                    lines.append(f"inline {ret_type} {field}(const {name} &val) {{")
                    lines.append(f"    return std::visit([](const auto &v) -> {ret_type} {{ return v._{field}; }}, val);")
                    lines.append("}")
                return prefix + "\n".join(lines), variant_type_str
            else:
                # Try presence-based dispatch on unique required fields
                presence_map = find_presence_dispatch(ref_names, types) if types else {}
                if presence_map:
                    fj.extend(_not_object_guard(name, nested_union_branches))
                    fj.append(f"    const QJsonObject obj = val.toObject();")
                    # Emit a branch for each type that has a unique field
                    for ref_name in ref_names:
                        if ref_name in presence_map:
                            field = presence_map[ref_name]
                            fj.append(f"    if (obj.contains(\"{field}\"))")
                            fj.append(f"        co_return {name}(co_await fromJson<{ref_name}>(val));")
                    # Fall back to try-each for remaining ambiguous types
                    ambiguous = [r for r in ref_names if r not in presence_map]
                    for ref_name in ambiguous:
                        fj.append(f"    {{")
                        fj.append(f"        auto result = fromJson<{ref_name}>(val);")
                        fj.append(f"        if (result) co_return {name}(*result);")
                        fj.append(f"    }}")
                else:
                    if nested_union_branches:
                        fj.append(f"    if (!val.isObject()) {{")
                        fj.extend(f"    {line}" for line in nested_union_branches)
                        fj.append(f"    }}")
                    for ref_name in ref_names:
                        fj.append(f"    if (val.isObject()) {{")
                        fj.append(f"        auto result = fromJson<{ref_name}>(val);")
                        fj.append(f"        if (result) co_return {name}(*result);")
                        fj.append(f"    }}")

    if "QJsonObject" in variant_types:
        fj.append(f"    if (val.isObject())")
        fj.append(f"        co_return {name}(val.toObject());  // open union: preserve unknown variants raw")
    fj.append(f'    co_return Utils::ResultError("Invalid {name}");')
    fj.append("}")
    if not skip_from_json:
        lines.extend(finalize_from_json(fj))

    # Emit shared-field getters for presence/try-each unions too
    if "anyOf" in spec or "oneOf" in spec:
        items = spec.get("anyOf", spec.get("oneOf", []))
        ref_names = [ref_cpp_type(r, types or {}) for item in items if (r := _extract_ref(item))]
        # Only for pure $ref unions (not already handled by the const-dispatch branch above).
        # Aliases of an already emitted variant share its accessors.
        if ref_names and types and not skip_to_json and len(ref_names) == len(items) \
                and "QJsonObject" not in variant_types:
            _, had_dispatch = find_dispatch_field(ref_names, types)
            if not had_dispatch:
                for field, ret_type in find_shared_fields(ref_names, types):
                    lines.append("")
                    if _emit_comments:
                        lines.append(f"/** Returns the '{field}' field from the active variant. */")
                    lines.append(f"inline {ret_type} {field}(const {name} &val) {{")
                    lines.append(f"    return std::visit([](const auto &v) -> {ret_type} {{ return v._{field}; }}, val);")
                    lines.append("}")

    if not skip_to_json:
        lines.append("")
        # Determine if this is a primitive union (all variant members are primitive
        # C++ types like int, QString, std::monostate) or an object/ref union.
        is_primitive_union = isinstance(spec.get("type"), list)
        if not is_primitive_union and ("anyOf" in spec or "oneOf" in spec):
            items = spec.get("anyOf", spec.get("oneOf", []))
            # A union of only plain-typed items (no $refs) is primitive
            if all(not _extract_ref(item) and "type" in item for item in items):
                is_primitive_union = True

        if not is_primitive_union and _variant_needs_typed_visitor(variant_types):
            lines.extend(_typed_toJsonValue_lines(name, variant_types, types)[1:-1])
        elif not is_primitive_union:
            # For object-ref unions, generate toJson(->QJsonObject) first,
            # then toJsonValue delegates to it.
            lines.append(f"inline QJsonObject toJson(const {name} &val) {{")
            lines.append("    return std::visit([](const auto &v) -> QJsonObject {")
            lines.append("        using T = std::decay_t<decltype(v)>;")
            lines.append("        if constexpr (std::is_same_v<T, QJsonObject>) {")
            lines.append("            return v;")
            lines.append("        } else {")
            lines.append("            return toJson(v);")
            lines.append("        }")
            lines.append("    }, val);")
            lines.append("}")
            lines.append("")
            lines.append(f"inline QJsonValue toJsonValue(const {name} &val) {{")
            lines.append("    return toJson(val);")
            lines.append("}")
        else:
            # Check if any variant members are list types (need special serialization)
            has_list = any(vt.startswith("QList<") for vt in variant_types)
            # Generate toJsonValue function for primitive/list types
            lines.append(f"inline QJsonValue toJsonValue(const {name} &val) {{")
            lines.append("    return std::visit([](const auto &v) -> QJsonValue {")
            has_monostate = "std::monostate" in variant_types
            if has_monostate or has_list:
                lines.append("        using T = std::decay_t<decltype(v)>;")
            if has_monostate:
                lines.append("        if constexpr (std::is_same_v<T, std::monostate>) {")
                lines.append("            return QJsonValue(QJsonValue::Null);")
                lines.append("        } else")
            if has_list:
                for vt in variant_types:
                    if vt.startswith("QList<"):
                        elem_type = vt[len("QList<"):-1]
                        if not needs_to_json(elem_type, types or {}):
                            elem_fn = ""
                        elif is_enum_type(elem_type, types or {}) \
                                or is_union_type_name(elem_type, types or {}):
                            elem_fn = "toJsonValue"
                        else:
                            elem_fn = "toJson"
                        lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
                        lines.append(f"            QJsonArray arr;")
                        lines.append(f"            for (const auto &elem : v)")
                        lines.append(f"                arr.append({json_call(elem_fn, 'elem')});")
                        lines.append(f"            return arr;")
                        lines.append(f"        }} else")
            lines.append("        {")
            lines.append("            return QVariant::fromValue(v).toJsonValue();")
            lines.append("        }")
            lines.append("    }, val);")
            lines.append("}")

    return prefix + "\n".join(lines), variant_type_str

def is_union_type_name(type_name, types):
    """Check if a type name refers to a union type"""
    if type_name in types:
        return is_union_type(types[type_name])
    # Aliases the generator itself introduced for inline unions.
    return type_name in _emitted_variant_sigs.values()

def escape_keyword(name):
    """Escape C++ keywords by appending underscore"""
    cpp_keywords = {"default", "enum", "class", "struct", "public", "private", 
                    "protected", "virtual", "override", "final", "const", 
                    "static", "extern", "typedef", "template", "typename",
                    "namespace", "using", "operator", "new", "delete",
                    "this", "friend", "inline", "register", "volatile",
                    "auto", "void", "int", "char", "short", "long", "float",
                    "double", "signed", "unsigned", "bool", "true", "false",
                    "if", "else", "for", "while", "do", "switch", "case",
                    "break", "continue", "return", "goto", "try", "catch",
                    "throw", "sizeof", "alignof", "alignas", "decltype", "typeid",
                    "export", "union", "explicit", "mutable",
                    "nullptr", "noexcept", "constexpr", "consteval", "constinit",
                    "concept", "requires", "thread_local", "static_assert", "asm",
                    "const_cast", "static_cast", "dynamic_cast", "reinterpret_cast",
                    "co_await", "co_return", "co_yield", "wchar_t", "char8_t",
                    "char16_t", "char32_t", "and", "or", "not", "xor", "compl",
                    "bitand", "bitor", "and_eq", "or_eq", "not_eq", "xor_eq"}
    return f"{name}_" if name in cpp_keywords else name

def sanitize_identifier(value):
    """Convert an arbitrary string to a valid C++ identifier, then escape keywords."""
    # Replace common operator/special characters with named equivalents first
    # to avoid collisions (e.g. both '+1' and '-1' would otherwise become '_1').
    char_map = {'+': 'plus', '-': 'minus', '*': 'star', '/': 'slash',
                '%': 'percent', '&': 'amp', '|': 'pipe', '^': 'hat',
                '~': 'tilde', '!': 'bang', '<': 'lt', '>': 'gt',
                '=': 'eq', '?': 'q', '@': 'at', '#': 'hash',
                '$': 'dollar', '.': 'dot'}
    result = ''
    for c in value:
        result += char_map.get(c, c)
    ident = re.sub(r'[^A-Za-z0-9_]', '_', result)
    if ident and ident[0].isdigit():
        ident = '_' + ident
    return escape_keyword(ident)

def list_type(item_type, optional=False):
    """Return the C++ list type, using QStringList when item type is QString."""
    inner = "QStringList" if item_type == "QString" else f"QList<{item_type}>"
    return f"std::optional<{inner}>" if optional else inner

# Trivially-copyable C++ types that should be passed by value.
_SCALAR_CPP_TYPES = {"int", "bool", "double", "float", "qint64", "qsizetype", "qreal",
                     "long", "short", "unsigned", "unsigned int", "unsigned long",
                     "int64_t", "uint64_t", "int32_t", "uint32_t"}

def _is_scalar(cpp_type_str: str) -> bool:
    """Return True for trivially-copyable scalar types (passed by value, not by const-ref)."""
    inner = cpp_type_str
    if inner.startswith("std::optional<") and inner.endswith(">"):
        inner = inner[len("std::optional<"):-1]
    return inner in _SCALAR_CPP_TYPES

def _param_type(cpp_type_str: str) -> str:
    """Return the setter parameter type: 'T' for scalars, 'const T&' otherwise."""
    return cpp_type_str if _is_scalar(cpp_type_str) else f"const {cpp_type_str} &"

def enum_keyed_map_info(spec):
    """Detect an object with propertyNames.enum and additionalProperties (typed).

    Returns (enum_values_list, cpp_value_type) on match, or None.
    The enum_values_list items are the allowed key strings from propertyNames.enum.
    """
    if spec.get("type") != "object":
        return None
    prop_names = spec.get("propertyNames")
    if not isinstance(prop_names, dict):
        return None
    enum_vals = prop_names.get("enum")
    if not isinstance(enum_vals, list) or not enum_vals:
        return None
    add_props = spec.get("additionalProperties")
    if not isinstance(add_props, dict):
        return None
    if spec.get("properties"):
        return None
    if "$ref" in add_props:
        val_type = add_props["$ref"].split("/")[-1]
    elif add_props.get("type"):
        val_type = cpp_type(add_props["type"])
    else:
        return None
    return enum_vals, val_type

def is_typed_map(spec):
    """Return the C++ value type if spec is a typed string-keyed map
    (type: object, additionalProperties with a specific type, no named properties).
    Returns None otherwise.  Excludes enum-keyed maps (propertyNames.enum).
    """
    if spec.get("type") != "object":
        return None
    # Enum-keyed maps are handled separately
    if enum_keyed_map_info(spec) is not None:
        return None
    add_props = spec.get("additionalProperties")
    if not isinstance(add_props, dict):
        return None
    # Handle $ref case
    if "$ref" in add_props:
        ref = add_props["$ref"]
        t = ref.split("/")[-1]
        if spec.get("properties"):
            return None
        return t
    t = add_props.get("type")
    if not t:
        return None
    if spec.get("properties"):
        return None
    if t == "array" and (items_ref := _extract_ref(add_props.get("items", {}))):
        return list_type(ref_type(items_ref))
    return cpp_type(t)

def is_open_map(spec):
    """Return True if spec is an open string-keyed map whose values are untyped
    (type: object, additionalProperties == {} or True, no named properties).
    These are represented as QMap<QString, QJsonValue>.
    """
    if spec.get("type") != "object":
        return False
    add_props = spec.get("additionalProperties")
    # additionalProperties: {} or additionalProperties: true means any value
    if add_props != {} and add_props is not True:
        return False
    if spec.get("properties"):
        return False
    return True

def _json_extract_expr(cpp_t, val_expr):
    """Return the C++ expression to extract `cpp_t` from QJsonValue `val_expr`.
    Returns None for unrecognised types.
    """
    _map = {
        "QString":     f"{val_expr}.toString()",
        "int":         f"{val_expr}.toInt()",
        "double":      f"{val_expr}.toDouble()",
        "bool":        f"{val_expr}.toBool()",
        "QJsonObject": f"{val_expr}.toObject()",
        "QJsonArray":  f"{val_expr}.toArray()",
        "QJsonValue":  f"{val_expr}",
    }
    return _map.get(cpp_t)

def array_item_cpp_type(spec):
    """The element type of a plain array, any JSON value when the items are unconstrained."""
    items = spec.get("items", {})
    if is_untyped_any(items):
        return "QJsonValue"
    return cpp_type(items.get("type", "string"))

def is_untyped_any(spec):
    """Property with no type/$ref/composition constraints → any JSON value (QJsonValue)."""
    if not isinstance(spec, dict):
        return False
    if spec.get("type") is not None:
        return False
    for k in ("$ref", "enum", "const", "anyOf", "oneOf", "allOf", "items", "properties"):
        if k in spec:
            return False
    return True

def _toJsonValue_visit_lines(alias):
    """Return code lines for a toJsonValue(const alias&) using std::visit."""
    return [
        "",
        f"inline QJsonValue toJsonValue(const {alias} &val) {{",
        "    return std::visit([](const auto &v) -> QJsonValue {",
        "        using T = std::decay_t<decltype(v)>;",
        "        if constexpr (std::is_same_v<T, QJsonObject>) {",
        "            return v;",
        "        } else {",
        "            return toJson(v);",
        "        }",
        "    }, val);",
        "}",
        "",
    ]

_SCALAR_VARIANT_TYPES = {"QString", "int", "double", "bool", "std::monostate",
                         "QJsonArray", "QStringList"}

def _is_anonymous_object(spec):
    """Whether spec is an object literal without properties, carrying no schema."""
    return (spec.get("type") == "object" and not spec.get("properties")
            and not spec.get("additionalProperties") and "$ref" not in spec)

def _variant_needs_typed_visitor(variant_types):
    """Whether the alternatives differ enough to need a per-type visitor."""
    return not all(vt not in _SCALAR_VARIANT_TYPES and not vt.startswith("QList<")
                   for vt in variant_types)

def _typed_toJsonValue_lines(alias, variant_types, types):
    """toJsonValue(const alias&) dispatching on each alternative in turn.

    Needed where the alternatives do not all serialize the same way: a list
    becomes an array, a scalar its own JSON value, a struct an object.
    """
    def element_call(elem_type, expr):
        if not needs_to_json(elem_type, types or {}):
            return expr
        fn = "toJsonValue" if is_enum_type(elem_type, types or {}) \
            or is_union_type_name(elem_type, types or {}) else "toJson"
        return f"{fn}({expr})"

    lines = ["", f"inline QJsonValue toJsonValue(const {alias} &val) {{",
             "    return std::visit([](const auto &v) -> QJsonValue {",
             "        using T = std::decay_t<decltype(v)>;"]
    for vt in variant_types:
        if vt == "std::monostate":
            lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
            lines.append("            return QJsonValue(QJsonValue::Null);")
            lines.append("        } else")
        elif vt.startswith("QList<") or vt == "QStringList":
            elem_type = "QString" if vt == "QStringList" else vt[len("QList<"):-1]
            lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
            lines.append("            QJsonArray arr;")
            lines.append("            for (const auto &elem : v)")
            lines.append(f"                arr.append({element_call(elem_type, 'elem')});")
            lines.append("            return arr;")
            lines.append("        } else")
        elif vt == "QJsonObject":
            lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
            lines.append("            return v;")
            lines.append("        } else")
        elif vt not in _SCALAR_VARIANT_TYPES:
            lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
            lines.append(f"            return {element_call(vt, 'v')};")
            lines.append("        } else")
    lines.append("        {")
    lines.append("            return QVariant::fromValue(v).toJsonValue();")
    lines.append("        }")
    lines.append("    }, val);")
    lines.append("}")
    lines.append("")
    return lines

def _map_anyof_info(spec):
    """Detect a map property whose additionalProperties is an anyOf union type.

    Returns (variant_cpp_types_list, suggested_alias_name) or None.
    variant_cpp_types_list is a list of C++ type strings for the variant alternatives.
    """
    if spec.get("type") != "object":
        return None
    add_props = spec.get("additionalProperties")
    if not isinstance(add_props, dict):
        return None
    any_of = add_props.get("anyOf", add_props.get("oneOf", []))
    if not any_of:
        return None
    if spec.get("properties"):
        return None  # has named properties, not a pure map

    variant_types = []
    for item in any_of:
        ref = _extract_ref(item)
        if ref:
            variant_types.append(ref_type(ref))
        elif isinstance(item.get("type"), list):
            # Multi-type like ["string", "integer", "boolean"]
            for t in item["type"]:
                variant_types.append(cpp_type(t))
        elif item.get("type") == "array":
            items = item.get("items", {})
            item_t = cpp_type(items.get("type", "string"))
            variant_types.append(list_type(item_t))
        elif item.get("type"):
            variant_types.append(cpp_type(item["type"]))

    if not variant_types:
        return None

    return variant_types, "Value"


def _build_map_value_variant_code(alias_name, variant_types):
    """Generate a using alias + fromJson + toJsonValue for an inline variant type
    used as the value type of a QMap."""
    lines = []
    variant_str = ", ".join(variant_types)
    lines.append(f"using {alias_name} = std::variant<{variant_str}>;")
    lines.append("")

    # fromJson
    fj = []
    fj.append(f"template<>")
    fj.append(f"inline Utils::Result<{alias_name}> fromJson<{alias_name}>(const QJsonValue &val) {{")
    for vt in variant_types:
        if vt == "QString":
            fj.append(f"    if (val.isString())")
            fj.append(f"        co_return {alias_name}(val.toString());")
        elif vt == "int":
            fj.append(f"    if (val.isDouble())")
            fj.append(f"        co_return {alias_name}(val.toInt());")
        elif vt == "double":
            fj.append(f"    if (val.isDouble())")
            fj.append(f"        co_return {alias_name}(val.toDouble());")
        elif vt == "bool":
            fj.append(f"    if (val.isBool())")
            fj.append(f"        co_return {alias_name}(val.toBool());")
        elif vt == "QStringList":
            fj.append(f"    if (val.isArray()) {{")
            fj.append(f"        QStringList list;")
            fj.append(f"        for (const QJsonValue &v : val.toArray())")
            fj.append(f"            list.append(v.toString());")
            fj.append(f"        co_return {alias_name}(list);")
            fj.append(f"    }}")
        elif vt.startswith("QList<"):
            inner = vt[6:-1]  # extract T from QList<T>
            fj.append(f"    if (val.isArray()) {{")
            fj.append(f"        {vt} list;")
            fj.append(f"        for (const QJsonValue &v : val.toArray())")
            fj.append(f"            list.append(co_await fromJson<{inner}>(v));")
            fj.append(f"        co_return {alias_name}(list);")
            fj.append(f"    }}")
        else:
            fj.append(f"    if (val.isObject()) {{")
            fj.append(f"        auto result = fromJson<{vt}>(val);")
            fj.append(f"        if (result) co_return {alias_name}(*result);")
            fj.append(f"    }}")
    fj.append(f'    co_return Utils::ResultError("Invalid {alias_name}");')
    fj.append("}")
    lines.extend(finalize_from_json(fj))
    lines.append("")

    # toJsonValue
    if not _read_only:
        lines.append(f"inline QJsonValue toJsonValue(const {alias_name} &val) {{")
        lines.append("    return std::visit([](const auto &v) -> QJsonValue {")
        lines.append("        using T = std::decay_t<decltype(v)>;")
        for vt in variant_types:
            if vt == "QString":
                lines.append(f"        if constexpr (std::is_same_v<T, QString>) return v;")
            elif vt == "int":
                lines.append(f"        if constexpr (std::is_same_v<T, int>) return v;")
            elif vt == "double":
                lines.append(f"        if constexpr (std::is_same_v<T, double>) return v;")
            elif vt == "bool":
                lines.append(f"        if constexpr (std::is_same_v<T, bool>) return v;")
            elif vt == "QStringList":
                lines.append(f"        if constexpr (std::is_same_v<T, QStringList>) {{")
                lines.append(f"            QJsonArray arr;")
                lines.append(f"            for (const QString &s : v) arr.append(s);")
                lines.append(f"            return arr;")
                lines.append(f"        }}")
            elif vt.startswith("QList<"):
                lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
                lines.append(f"            QJsonArray arr;")
                lines.append(f"            for (const auto &item : v) arr.append(toJsonValue(item));")
                lines.append(f"            return arr;")
                lines.append(f"        }}")
            else:
                lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) return toJson(v);")
        lines.append("        return QJsonValue{};")
        lines.append("    }, val);")
        lines.append("}")
        lines.append("")

    return lines


_PRIMITIVE_JSON_TYPES = {"string", "integer", "number", "boolean"}

# Primitives that may take part in a union with named types.
_PRIMITIVE_UNION_JSON_TYPES = _PRIMITIVE_JSON_TYPES | {"null"}

def _nullable_primitive_of(spec):
    """The primitive type of an `anyOf: [{type: T}, {type: "null"}]` spec."""
    if not isinstance(spec, dict):
        return None
    items = spec.get("anyOf", spec.get("oneOf"))
    if not isinstance(items, list) or len(items) != 2:
        return None
    item_types = [i.get("type") for i in items
                  if isinstance(i, dict) and set(i) == {"type"}]
    if len(item_types) != 2 or "null" not in item_types:
        return None
    base = next(t for t in item_types if t != "null")
    return base if base in _PRIMITIVE_JSON_TYPES else None

def normalize_nullable_primitives(obj):
    """Rewrite nullable primitive properties into the `type: [T, "null"]` form.

    A schema is free to spell a nullable scalar as a union of the scalar and
    null. As a union it has no C++ type to name and would degrade to a
    QString; as a nullable scalar it becomes std::optional<T>.
    """
    if isinstance(obj, dict):
        properties = obj.get("properties")
        if isinstance(properties, dict):
            for spec in properties.values():
                base = _nullable_primitive_of(spec)
                if base:
                    spec.pop("anyOf", None)
                    spec.pop("oneOf", None)
                    spec["type"] = [base, "null"]
        for value in obj.values():
            normalize_nullable_primitives(value)
    elif isinstance(obj, list):
        for item in obj:
            normalize_nullable_primitives(item)

def _nullable_type(spec):
    """Handle type: [T, "null"] (nullable scalar) in JSON Schema.

    Returns (base_json_type_str, is_nullable):
      - is_nullable=True  means the value can be JSON null (map to std::optional<T>)
      - base_json_type_str is the single non-null type string, or None if unresolvable
    For plain string types or non-list types returns (None, False).
    """
    t = spec.get("type")
    if not isinstance(t, list):
        return None, False
    non_null = [x for x in t if x != "null"]
    has_null = "null" in t
    if len(non_null) == 1:
        return non_null[0], has_null
    # Multiple non-null types or empty non-null list — not a simple nullable scalar
    return None, has_null

def _nullable_ref_array_type(spec):
    """Detect type: ["array", "null"] with $ref items. Returns the item ref
    type name, or None. Only used in --three-state mode."""
    base_t, has_null = _nullable_type(spec)
    if base_t != "array" or not has_null:
        return None
    items_ref = _extract_ref(spec.get("items", {}))
    return ref_type(items_ref) if items_ref else None

def _is_patch_field(spec, is_optional, owner=None, prop=None):
    """True if an optional nullable field should be modeled as Patch<T>.

    A recursive field is stored as ``Recursive<T>``, which cannot hold the
    three states: for it null and absent both mean "no value".
    """
    if not _three_state or not is_optional:
        return False
    if owner is not None and _is_recursive_field(owner, prop):
        return False
    if _nullable_type(spec)[1]:
        return True
    return _extract_nullable_ref(spec) is not None

def nested_short_name(parent_name, child_name):
    """Strip parent name prefix from a nested child name for cleaner C++ declarations.

    E.g.:
      nested_short_name('GetPromptRequest', 'GetPromptRequestParams') -> 'Params'
      nested_short_name('GetPromptRequestParams', 'GetPromptRequestParams_meta') -> 'Meta'
      nested_short_name('ServerCapabilities', 'ServerCapabilitiesTools') -> 'Tools'
    """
    if child_name.startswith(parent_name) and len(child_name) > len(parent_name):
        suffix = child_name[len(parent_name):]
        stripped = suffix.lstrip('_')
        if stripped:
            return stripped[0].upper() + stripped[1:]
    return child_name


def _emit_toJson(name, props, types, required, lines, has_additional_props,
                 inline_enum_names, sub_struct_names, array_item_struct_names,
                 array_item_union_names, field_union_names, map_value_union_names):
    """Emit the toJson function for a struct. Extracted so it can be skipped in --read-only mode."""
    lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
    init_entries = []  # (key, value_expr) for the initializer list
    post_lines   = []  # lines emitted after the QJsonObject declaration

    for prop, spec in props.items():
        prop_name = sanitize_identifier(prop)
        is_optional = prop not in required

        def is_const_string(spec):
            return spec.get("type") == "string" and "const" in spec

        if prop in inline_enum_names:
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", toJsonValue(*data._{sanitize_identifier(prop)}));")
            else:
                init_entries.append((prop, f"toJsonValue(data._{sanitize_identifier(prop)})"))
        elif prop in sub_struct_names:
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", toJson(*data._{sanitize_identifier(prop)}));")
            else:
                init_entries.append((prop, f"toJson(data._{sanitize_identifier(prop)})"))
        elif _extract_ref(spec):
            t = ref_type(_extract_ref(spec))
            if is_integer_const_namespace(t, types):
                if is_optional:
                    post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                    post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                else:
                    init_entries.append((prop, f"data._{sanitize_identifier(prop)}"))
            else:
                is_enum = is_enum_type(t, types)
                is_union = is_union_type_name(t, types)
                is_simple = is_simple_type_alias(t, types)
                if is_simple:
                    if is_optional:
                        post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                        post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                    else:
                        init_entries.append((prop, f"data._{sanitize_identifier(prop)}"))
                else:
                    val_fn = "toJsonValue" if (is_enum or is_union) else "toJson"
                    if is_optional:
                        post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                        post_lines.append(f"        obj.insert(\"{prop}\", {val_fn}(*data._{sanitize_identifier(prop)}));")
                    else:
                        init_entries.append((prop, f"{val_fn}(data._{sanitize_identifier(prop)})"))
        elif _extract_nullable_ref(spec):
            t = _extract_nullable_ref(spec)
            if is_integer_const_namespace(t, types):
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                if prop in required:
                    post_lines.append(f"    else")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
                elif _is_patch_field(spec, is_optional, name, prop):
                    post_lines.append(f"    else if (data._{sanitize_identifier(prop)}.isNull())")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
            else:
                is_enum = is_enum_type(t, types)
                is_union = is_union_type_name(t, types)
                is_simple = is_simple_type_alias(t, types)
                val_fn = "toJsonValue" if (is_enum or is_union) else ("" if is_simple else "toJson")
                if val_fn:
                    post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                    post_lines.append(f"        obj.insert(\"{prop}\", {val_fn}(*data._{sanitize_identifier(prop)}));")
                else:
                    post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                    post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                if prop in required:
                    post_lines.append(f"    else")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
                elif _is_patch_field(spec, is_optional, name, prop):
                    post_lines.append(f"    else if (data._{sanitize_identifier(prop)}.isNull())")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            item_type = ref_type(_extract_ref(spec.get("items", {})))
            is_enum = is_enum_type(item_type, types)
            is_union = is_union_type_name(item_type, types)
            if is_simple_type_alias(item_type, types) \
                    or is_integer_const_namespace(item_type, types):
                arr_fn = ""  # plain QJsonValue-convertible alias
            else:
                arr_fn = "toJsonValue" if (is_enum or is_union) else "toJson"
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonArray arr_{prop_name};")
                post_lines.append(f"        for (const auto &v : *data._{sanitize_identifier(prop)}) arr_{prop_name}.append({json_call(arr_fn, 'v')});")
                post_lines.append(f"        obj.insert(\"{prop}\", arr_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonArray arr_{prop_name};")
                post_lines.append(f"    for (const auto &v : data._{sanitize_identifier(prop)}) arr_{prop_name}.append({json_call(arr_fn, 'v')});")
                post_lines.append(f"    obj.insert(\"{prop}\", arr_{prop_name});")
        elif _three_state and _nullable_ref_array_type(spec):
            item_type = _nullable_ref_array_type(spec)
            is_enum = is_enum_type(item_type, types)
            is_union = is_union_type_name(item_type, types)
            if is_simple_type_alias(item_type, types):
                arr_fn = ""  # plain QJsonValue-convertible alias
            else:
                arr_fn = "toJsonValue" if (is_enum or is_union) else "toJson"
            post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
            post_lines.append(f"        QJsonArray arr_{prop_name};")
            post_lines.append(f"        for (const auto &v : *data._{sanitize_identifier(prop)}) arr_{prop_name}.append({json_call(arr_fn, 'v')});")
            post_lines.append(f"        obj.insert(\"{prop}\", arr_{prop_name});")
            post_lines.append(f"    }}")
            if _is_patch_field(spec, is_optional):
                post_lines.append(f"    else if (data._{sanitize_identifier(prop)}.isNull())")
                post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
        elif prop in array_item_struct_names:
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonArray arr_{prop_name};")
                post_lines.append(f"        for (const auto &v : *data._{sanitize_identifier(prop)}) arr_{prop_name}.append(toJson(v));")
                post_lines.append(f"        obj.insert(\"{prop}\", arr_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonArray arr_{prop_name};")
                post_lines.append(f"    for (const auto &v : data._{sanitize_identifier(prop)}) arr_{prop_name}.append(toJson(v));")
                post_lines.append(f"    obj.insert(\"{prop}\", arr_{prop_name});")
        elif prop in array_item_union_names:
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonArray arr_{prop_name};")
                post_lines.append(f"        for (const auto &v : *data._{sanitize_identifier(prop)}) arr_{prop_name}.append(toJsonValue(v));")
                post_lines.append(f"        obj.insert(\"{prop}\", arr_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonArray arr_{prop_name};")
                post_lines.append(f"    for (const auto &v : data._{sanitize_identifier(prop)}) arr_{prop_name}.append(toJsonValue(v));")
                post_lines.append(f"    obj.insert(\"{prop}\", arr_{prop_name});")
        elif prop in field_union_names:
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", toJsonValue(*data._{sanitize_identifier(prop)}));")
            else:
                init_entries.append((prop, f"toJsonValue(data._{sanitize_identifier(prop)})"))
        elif spec.get("type") == "array":
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonArray arr_{prop_name};")
                post_lines.append(f"        for (const auto &v : *data._{sanitize_identifier(prop)}) arr_{prop_name}.append(v);")
                post_lines.append(f"        obj.insert(\"{prop}\", arr_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonArray arr_{prop_name};")
                post_lines.append(f"    for (const auto &v : data._{sanitize_identifier(prop)}) arr_{prop_name}.append(v);")
                post_lines.append(f"    obj.insert(\"{prop}\", arr_{prop_name});")
        elif prop in map_value_union_names:
            val_alias, full_map_type = map_value_union_names[prop]
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonObject map_{prop_name};")
                post_lines.append(f"        for (auto it = data._{sanitize_identifier(prop)}->constBegin(); it != data._{sanitize_identifier(prop)}->constEnd(); ++it)")
                post_lines.append(f"            map_{prop_name}.insert(it.key(), toJsonValue(it.value()));")
                post_lines.append(f"        obj.insert(\"{prop}\", map_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonObject map_{prop_name};")
                post_lines.append(f"    for (auto it = data._{sanitize_identifier(prop)}.constBegin(); it != data._{sanitize_identifier(prop)}.constEnd(); ++it)")
                post_lines.append(f"        map_{prop_name}.insert(it.key(), toJsonValue(it.value()));")
                post_lines.append(f"    obj.insert(\"{prop}\", map_{prop_name});")
        elif is_open_map(spec):
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonObject map_{prop_name};")
                post_lines.append(f"        for (auto it = data._{sanitize_identifier(prop)}->constBegin(); it != data._{sanitize_identifier(prop)}->constEnd(); ++it)")
                post_lines.append(f"            map_{prop_name}.insert(it.key(), it.value());")
                post_lines.append(f"        obj.insert(\"{prop}\", map_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonObject map_{prop_name};")
                post_lines.append(f"    for (auto it = data._{sanitize_identifier(prop)}.constBegin(); it != data._{sanitize_identifier(prop)}.constEnd(); ++it)")
                post_lines.append(f"        map_{prop_name}.insert(it.key(), it.value());")
                post_lines.append(f"    obj.insert(\"{prop}\", map_{prop_name});")
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            deref = "->" if is_optional else "."
            if val_type.startswith("QList<"):
                item_fn = "toJsonValue" if is_enum_type(val_type[6:-1], types) else "toJson"
                indent = "    " if is_optional else ""
                if is_optional:
                    post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"{indent}    QJsonObject map_{prop_name};")
                post_lines.append(f"{indent}    for (auto it = data._{sanitize_identifier(prop)}{deref}constBegin(); it != data._{sanitize_identifier(prop)}{deref}constEnd(); ++it) {{")
                post_lines.append(f"{indent}        QJsonArray arr_{prop_name};")
                post_lines.append(f"{indent}        for (const auto &v : it.value()) arr_{prop_name}.append({item_fn}(v));")
                post_lines.append(f"{indent}        map_{prop_name}.insert(it.key(), arr_{prop_name});")
                post_lines.append(f"{indent}    }}")
                post_lines.append(f"{indent}    obj.insert(\"{prop}\", map_{prop_name});")
                if is_optional:
                    post_lines.append(f"    }}")
            elif is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonObject map_{prop_name};")
                if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                    post_lines.append(f"        for (auto it = data._{sanitize_identifier(prop)}->constBegin(); it != data._{sanitize_identifier(prop)}->constEnd(); ++it)")
                    post_lines.append(f"            map_{prop_name}.insert(it.key(), QJsonValue(it.value()));")
                else:
                    map_fn = "toJsonValue" if is_enum_type(val_type, types)                         or is_union_type_name(val_type, types) else "toJson"
                    post_lines.append(f"        for (auto it = data._{sanitize_identifier(prop)}->constBegin(); it != data._{sanitize_identifier(prop)}->constEnd(); ++it)")
                    post_lines.append(f"            map_{prop_name}.insert(it.key(), {map_fn}(it.value()));")
                post_lines.append(f"        obj.insert(\"{prop}\", map_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonObject map_{prop_name};")
                if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                    post_lines.append(f"    for (auto it = data._{sanitize_identifier(prop)}.constBegin(); it != data._{sanitize_identifier(prop)}.constEnd(); ++it)")
                    post_lines.append(f"        map_{prop_name}.insert(it.key(), QJsonValue(it.value()));")
                else:
                    map_fn = "toJsonValue" if is_enum_type(val_type, types)                         or is_union_type_name(val_type, types) else "toJson"
                    post_lines.append(f"    for (auto it = data._{sanitize_identifier(prop)}.constBegin(); it != data._{sanitize_identifier(prop)}.constEnd(); ++it)")
                    post_lines.append(f"        map_{prop_name}.insert(it.key(), {map_fn}(it.value()));")
                post_lines.append(f"    obj.insert(\"{prop}\", map_{prop_name});")
        elif is_const_string(spec):
            const_value = spec["const"]
            init_entries.append((prop, f"QString(\"{const_value}\")"))
        else:
            _, is_nullable = _nullable_type(spec)
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                if _is_patch_field(spec, is_optional):
                    post_lines.append(f"    else if (data._{sanitize_identifier(prop)}.isNull())")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
            elif is_nullable:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                post_lines.append(f"    else")
                post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
            else:
                init_entries.append((prop, f"data._{sanitize_identifier(prop)}"))

    # Emit Q_UNUSED(data) when the parameter is never referenced
    data_is_used = (
        post_lines
        or has_additional_props
        or any("data." in v for _, v in init_entries)
    )
    if not data_is_used:
        lines.append(f"    Q_UNUSED(data)")

    # Emit QJsonObject declaration
    if init_entries:
        if len(init_entries) == 1:
            k, v = init_entries[0]
            lines.append(f"    QJsonObject obj{{{{\"{k}\", {v}}}}};")
        else:
            lines.append(f"    QJsonObject obj{{")
            for i, (k, v) in enumerate(init_entries):
                comma = "," if i < len(init_entries) - 1 else ""
                lines.append(f"        {{\"{k}\", {v}}}{comma}")
            lines.append(f"    }};")
    else:
        lines.append(f"    QJsonObject obj;")

    lines.extend(post_lines)
    if has_additional_props:
        lines.append(f"    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)")
        lines.append(f"        obj.insert(it.key(), it.value());")
    lines.append(f"    return obj;")
    lines.append("}")
    lines.append("")  # blank line after toJson


def accepts_additional_props(spec):
    """True when a schema keeps arbitrary extra keys, which must then be
    preserved verbatim instead of dropped on round-trip."""
    additional = spec.get("additionalProperties")
    return additional is True or additional == {}

def parse_struct(name, props, types, required=None, description='', nested_children=None, children_of=None, original_name=None, has_additional_props=False):
    if required is None:
        required = []
    if nested_children is None:
        nested_children = {}

    # When stripping the parent prefix from child names use the original (pre-shortening)
    # name so that grandchildren such as GetPromptRequestParams_meta can still have
    # 'GetPromptRequestParams' stripped correctly even though the parent was renamed 'Params'.
    effective_prefix = original_name if original_name is not None else name
    # Maps original $defs child_name -> short_name so that $ref fields can be qualified.
    nested_short_names: dict = {}

    def is_const_string(spec):
        return spec.get("type") == "string" and "const" in spec

    def is_inline_enum(spec):
        """Inline string enum: type==string, enum list present, no const, no $ref."""
        return (
            spec.get("type") == "string"
            and "enum" in spec
            and "const" not in spec
            and "$ref" not in spec
        )

    def needs_sub_struct(spec):
        """Inline object with non-empty properties and no $ref → generate a named sub-struct."""
        return (spec.get("type") == "object"
                and "$ref" not in spec
                and spec.get("properties"))

    def array_items_need_sub_struct(spec):
        """Array property whose items is an inline object with properties → generate a named item sub-struct."""
        items = spec.get("items", {})
        return (spec.get("type") == "array"
                and "$ref" not in items
                and items.get("type") == "object"
                and items.get("properties"))

    def array_items_anyof_ref_names(spec):
        """Return ref type name list if spec is an array whose items has anyOf with only $refs."""
        if spec.get("type") != "array":
            return []
        items = spec.get("items", {})
        any_of = items.get("anyOf", [])
        if not any_of:
            return []
        names = [ref_cpp_type(r, types) for item in any_of if (r := _extract_ref(item))]
        return names if len(names) == len(any_of) else []

    def field_anyof_ref_names(spec):
        """Return the C++ type list if spec is a non-array field with anyOf/oneOf.

        Members are $refs, arrays of $refs (as QList<RefType>) or primitives, so
        that a union of a flag and an options object keeps both alternatives.
        """
        if spec.get("type") == "array":
            return []
        any_of = spec.get("anyOf", spec.get("oneOf", []))
        if not any_of:
            return []
        # A nullable reference is an optional field, not a variant.
        if _extract_nullable_ref(spec):
            return []
        names = []
        for item in any_of:
            ref = _extract_ref(item)
            if ref:
                names.append(ref_cpp_type(ref, types))
            elif item.get("type") == "array" and "$ref" in item.get("items", {}):
                names.append(list_type(ref_type(item["items"]["$ref"])))
            elif set(item) == {"type"} and item["type"] in _PRIMITIVE_UNION_JSON_TYPES:
                names.append(cpp_type(item["type"]))
            elif _is_anonymous_object(item):
                names.append("QJsonObject")
            else:
                return []  # unrecognised item shape — bail out
        return names if len(set(names)) > 1 else []

    # Recursively generate sub-structs for inline nested objects.
    sub_struct_blocks = []   # code strings to prepend
    sub_struct_names  = {}   # prop_name -> generated sub-struct type name
    array_item_struct_names = {}  # prop_name -> generated sub-struct type name for array items
    array_item_union_names  = {}  # prop_name -> generated union alias type name for anyOf array items
    field_union_names       = {}  # prop_name -> generated union alias type name for anyOf field
    map_value_union_names   = {}  # prop_name -> (map_val_alias, full_map_type) for maps with anyOf value types
    inline_enum_names       = {}  # prop_name -> nested enum class short name

    # Process exclusively-owned nested child types ($defs types only used here)
    child_struct_inserts = []   # indented struct defs placed inside this struct body
    child_preamble_blocks = []  # inline sub-structs of children; stay at namespace scope
    child_serial_blocks = []    # qualified fromJson/toJson for children, emitted after parent };

    for child_name, child_details in nested_children.items():
        child_props_n = child_details.get('properties', {})
        child_required_n = child_details.get('required', [])
        child_desc_n = child_details.get('description', '')
        # Generate full child code without further nesting
        grandchildren = (children_of or {}).get(child_name, {})
        short_name = nested_short_name(effective_prefix, child_name)
        nested_short_names[child_name] = short_name
        child_full = parse_struct(short_name, child_props_n, types, child_required_n, child_desc_n, nested_children=grandchildren, children_of=children_of, original_name=child_name, has_additional_props=accepts_additional_props(child_details))
        _collect_sub_struct_output(child_full, short_name, name,
                                   child_preamble_blocks, child_struct_inserts, child_serial_blocks)

    for prop, spec in props.items():
        if needs_sub_struct(spec):
            sub_name = name + prop[0].upper() + prop[1:]
            short_sub_name = nested_short_name(name, sub_name)
            sub_code = parse_struct(short_sub_name,
                                    spec["properties"],
                                    types,
                                    spec.get("required", []),
                                    spec.get("description", ""),
                                    original_name=sub_name,
                                    has_additional_props=accepts_additional_props(spec))
            _collect_sub_struct_output(sub_code, short_sub_name, name,
                                       child_preamble_blocks, child_struct_inserts, child_serial_blocks)
            sub_struct_names[prop] = short_sub_name
        elif array_items_need_sub_struct(spec):
            sub_name = name + prop[0].upper() + prop[1:] + "Item"
            short_sub_name = nested_short_name(name, sub_name)
            items_spec = spec["items"]
            sub_code = parse_struct(short_sub_name,
                                    items_spec["properties"],
                                    types,
                                    items_spec.get("required", []),
                                    items_spec.get("description", ""),
                                    original_name=sub_name,
                                    has_additional_props=accepts_additional_props(items_spec))
            _collect_sub_struct_output(sub_code, short_sub_name, name,
                                       child_preamble_blocks, child_struct_inserts, child_serial_blocks)
            array_item_struct_names[prop] = short_sub_name
        elif is_inline_enum(spec):
            enum_class_name = prop[0].upper() + prop[1:]
            values = spec["enum"]
            # Generate indented enum class to be inserted into the struct body
            enum_lines = []
            if _emit_comments and spec.get("description"):
                for cl in doc_comment(spec["description"], indent="    ").rstrip("\n").split("\n"):
                    enum_lines.append(cl)
            enum_lines.append(f"    enum class {enum_class_name} {{")
            for v in values:
                enum_lines.append(f"        {sanitize_identifier(v)},")
            if enum_lines[-1].endswith(","):
                enum_lines[-1] = enum_lines[-1].rstrip(",")
            enum_lines.append("    };")
            enum_lines.append("")
            child_struct_inserts.append("\n".join(enum_lines) + "\n")
            # Generate serializers for the qualified type (emitted after struct closes)
            qname = f"{name}::{enum_class_name}"
            ser_lines = []
            ser_lines.append(f"inline QString toString(const {qname} &v) {{")
            ser_lines.append("    switch(v) {")
            for v in values:
                ser_lines.append(f"        case {qname}::{sanitize_identifier(v)}: return \"{v}\";")
            ser_lines.append("    }")
            ser_lines.append("    return {};")
            ser_lines.append("}")
            ser_lines.append("")
            ser_lines.append(f"template<>")
            ser_lines.append(f"inline Utils::Result<{qname}> fromJson<{qname}>(const QJsonValue &val) {{")
            ser_lines.append(f"    if (!val.isString())")
            ser_lines.append(f"        return Utils::ResultError(\"Expected JSON string for {qname}\");")
            ser_lines.append(f"    const QString str = val.toString();")
            for v in values:
                ser_lines.append(f"    if (str == \"{v}\") return {qname}::{sanitize_identifier(v)};")
            ser_lines.append(f"    return Utils::ResultError(\"Invalid {qname} value: \" + str);")
            ser_lines.append("}")
            ser_lines.append("")
            if not _read_only:
                ser_lines.append(f"inline QJsonValue toJsonValue(const {qname} &v) {{")
                ser_lines.append("    return toString(v);")
                ser_lines.append("}")
                ser_lines.append("")
            child_serial_blocks.append("\n".join(ser_lines))
            inline_enum_names[prop] = enum_class_name
        else:
            ref_names = array_items_anyof_ref_names(spec)
            if ref_names:
                variant_str = ", ".join(ref_names)
                if existing := _variant_alias_for(variant_str):
                    # Reuse the already-emitted alias to avoid fromJson redefinition
                    union_alias = existing
                else:
                    union_alias = name + prop[0].upper() + prop[1:] + "Item"
                    sub_struct_blocks.append("\n".join(_build_inline_union_code(union_alias, ref_names, types)))
                    _register_variant_alias(variant_str, union_alias)
                array_item_union_names[prop] = union_alias
            else:
                fref_names = field_anyof_ref_names(spec)
                if fref_names:
                    variant_str = ", ".join(fref_names)
                    if existing := _variant_alias_for(variant_str):
                        # Reuse the already-emitted alias to avoid fromJson redefinition
                        union_alias = existing
                    else:
                        union_alias = name + prop[0].upper() + prop[1:]
                        sub_struct_blocks.append("\n".join(_build_inline_union_code(union_alias, fref_names, types)))
                        _register_variant_alias(variant_str, union_alias)
                    field_union_names[prop] = union_alias
                elif _map_anyof_info(spec):
                    # Map with anyOf-typed additionalProperties values
                    variant_types, val_alias = _map_anyof_info(spec)
                    val_alias = name + prop[0].upper() + prop[1:] + "Value"
                    variant_str = ", ".join(variant_types)
                    if existing := _variant_alias_for(variant_str):
                        val_alias = existing
                    else:
                        block = _build_map_value_variant_code(val_alias, variant_types)
                        sub_struct_blocks.append("\n".join(block))
                        _register_variant_alias(variant_str, val_alias)
                    full_map_type = f"QMap<QString, {val_alias}>"
                    map_value_union_names[prop] = (val_alias, full_map_type)

    # Member variables are prefixed with _ so that builder methods can use the plain field name.
    # Builder functions use escape_keyword(prop) to avoid clashing with C++ reserved words.
    lines = [f"struct {name} {{"]  # type: list[str]
    # Nested child struct definitions appear first so the type names are in scope
    for _child_insert in child_struct_inserts:
        lines.append(_child_insert)
    # Maps prop -> declared C++ type (used later by getter methods)
    prop_decl_types: dict = {}
    for prop, spec in props.items():
        is_optional = prop not in required
        prop_desc = spec.get('description', '').strip() if _emit_comments else ''
        if '\n' in prop_desc:
            pre_lines = doc_comment(prop_desc, indent='    ').rstrip('\n').split('\n')
            inline_comment = ''
        else:
            pre_lines = []
            inline_comment = f'  //!< {prop_desc}' if prop_desc else ''
        if prop in inline_enum_names:
            t = inline_enum_names[prop]
            decl_type = f"std::optional<{t}>" if is_optional else t
            # Doc comment already emitted on the nested enum class; skip it here.
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};")
        elif prop in sub_struct_names:
            t = sub_struct_names[prop]
            decl_type = f"std::optional<{t}>" if is_optional else t
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        # Handle $ref (direct or allOf-wrapped)
        elif _extract_ref(spec):
            t = ref_type(_extract_ref(spec))
            if is_integer_const_namespace(t, types):
                t = "int"  # namespace of int constants, not a C++ type
            else:
                t = nested_short_names.get(t, t)  # use short name if nested
            if _is_recursive_field(name, prop):
                decl_type = f"Recursive<{t}>"
            else:
                decl_type = f"std::optional<{t}>" if is_optional else t
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        # Handle nullable $ref (anyOf with $ref + null)
        elif _extract_nullable_ref(spec):
            t = _extract_nullable_ref(spec)
            if is_integer_const_namespace(t, types):
                t = "int"
            else:
                t = nested_short_names.get(t, t)
            if _is_recursive_field(name, prop):
                decl_type = f"Recursive<{t}>"
            elif _is_patch_field(spec, is_optional, name, prop):
                decl_type = f"Patch<{t}>"
            else:
                decl_type = f"std::optional<{t}>"  # always optional (nullable)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            item_type = ref_type(_extract_ref(spec.get("items", {})))
            if is_integer_const_namespace(item_type, types):
                item_type = "int"  # namespace of int constants, not a C++ type
            else:
                item_type = nested_short_names.get(item_type, item_type)  # short name if nested
            decl_type = list_type(item_type, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif _three_state and _nullable_ref_array_type(spec):
            item_type = _nullable_ref_array_type(spec)
            item_type = nested_short_names.get(item_type, item_type)
            inner = list_type(item_type)
            decl_type = f"Patch<{inner}>" if is_optional else f"std::optional<{inner}>"
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif prop in array_item_struct_names:
            t = array_item_struct_names[prop]
            decl_type = list_type(t, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif prop in array_item_union_names:
            t = array_item_union_names[prop]
            decl_type = list_type(t, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif prop in field_union_names:
            t = field_union_names[prop]
            decl_type = f"std::optional<{t}>" if is_optional else t
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif prop in map_value_union_names:
            val_alias, full_map_type = map_value_union_names[prop]
            decl_type = f"std::optional<{full_map_type}>" if is_optional else full_map_type
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif spec.get("type") == "array":
            item_type = array_item_cpp_type(spec)
            decl_type = list_type(item_type, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif is_open_map(spec):
            inner = "QMap<QString, QJsonValue>"
            decl_type = f"std::optional<{inner}>" if is_optional else inner
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            inner = f"QMap<QString, {val_type}>"
            decl_type = f"std::optional<{inner}>" if is_optional else inner
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        elif is_const_string(spec):
            pass  # const string fields are not stored in the struct
        elif is_untyped_any(spec):
            decl_type = "std::optional<QJsonValue>" if is_optional else "QJsonValue"
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        else:
            base_t, is_nullable = _nullable_type(spec)
            t = base_t if base_t else spec.get("type", "string")
            if _is_patch_field(spec, is_optional):
                decl_type = f"Patch<{cpp_type(t)}>"
            elif is_optional or is_nullable:
                decl_type = f"std::optional<{cpp_type(t)}>"
            else:
                decl_type = cpp_type(t)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)}{{}};{inline_comment}")
        if not is_const_string(spec):
            prop_decl_types[prop] = decl_type

    if has_additional_props:
        lines.append(f"    QJsonObject _additionalProperties;  //!< additional properties")

    # Builder methods — named after the field (keyword-escaped), return *this by reference.
    if not _read_only:
        lines.append("")

    def singular_add_name(prop):
        """Return the singular 'addXxx' method name for a collection field named prop."""
        if prop.endswith('ies'):
            singular = prop[:-3] + 'y'
        elif prop.endswith('s'):
            singular = prop[:-1]
        else:
            singular = prop
        return 'add' + singular[0].upper() + singular[1:]

    for prop, spec in props.items():
        if _read_only:
            break
        prop_name = sanitize_identifier(prop)
        if is_const_string(spec):
            continue  # no stored member, no builder
        is_optional = prop not in required
        # Determine inner_type (full collection type) and list_item_type (element type, or None)
        list_item_type = None
        if prop in inline_enum_names:
            inner_type = inline_enum_names[prop]
        elif prop in sub_struct_names:
            inner_type = sub_struct_names[prop]
        elif _extract_ref(spec):
            inner_type = ref_type(_extract_ref(spec))
            if is_integer_const_namespace(inner_type, types):
                inner_type = "int"
            else:
                inner_type = nested_short_names.get(inner_type, inner_type)  # use short name if nested
        elif _extract_nullable_ref(spec):
            inner_type = _extract_nullable_ref(spec)
            if is_integer_const_namespace(inner_type, types):
                inner_type = "int"
            else:
                inner_type = nested_short_names.get(inner_type, inner_type)
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            list_item_type = ref_type(_extract_ref(spec.get("items", {})))
            if is_integer_const_namespace(list_item_type, types):
                list_item_type = "int"
            else:
                list_item_type = nested_short_names.get(list_item_type, list_item_type)
            inner_type = list_type(list_item_type)
        elif _three_state and _nullable_ref_array_type(spec):
            item_type = _nullable_ref_array_type(spec)
            inner_type = list_type(nested_short_names.get(item_type, item_type))
        elif prop in array_item_struct_names:
            list_item_type = array_item_struct_names[prop]
            inner_type = list_type(list_item_type)
        elif prop in array_item_union_names:
            list_item_type = array_item_union_names[prop]
            inner_type = list_type(list_item_type)
        elif prop in field_union_names:
            inner_type = field_union_names[prop]
        elif prop in map_value_union_names:
            _, inner_type = map_value_union_names[prop]
        elif spec.get("type") == "array":
            list_item_type = array_item_cpp_type(spec)
            inner_type = list_type(list_item_type)
        elif is_open_map(spec):
            inner_type = "QMap<QString, QJsonValue>"
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            inner_type = f"QMap<QString, {val_type}>"
        elif is_untyped_any(spec):
            inner_type = "QJsonValue"
        else:
            base_t, is_nullable = _nullable_type(spec)
            inner_type = cpp_type(base_t if base_t else spec.get("type", "string"))
            if is_nullable and not is_optional:
                inner_type = f"std::optional<{inner_type}>"
        if _is_recursive_field(name, prop):
            setter_type = inner_type
        elif _is_patch_field(spec, is_optional, name, prop):
            setter_type = f"Patch<{inner_type}>"
        elif is_optional:
            setter_type = f"std::optional<{inner_type}>"
        else:
            setter_type = inner_type
        lines.append(f"    {name}& {prop_name}({_param_type(setter_type)} v) {{ _{sanitize_identifier(prop)} = v; return *this; }}")
        if _is_patch_field(spec, is_optional, name, prop):
            lines.append(f"    {name}& {prop_name}({_param_type(inner_type)} v) {{ _{sanitize_identifier(prop)} = v; return *this; }}")
        # For open-map fields emit a per-key adder and a QJsonObject merger
        if is_open_map(spec):
            add_name = singular_add_name(prop)
            if is_optional:
                lines.append(
                    f"    {name}& {add_name}(const QString &key, const QJsonValue &v) "
                    f"{{ if (!_{sanitize_identifier(prop)}) _{sanitize_identifier(prop)} = QMap<QString, QJsonValue>{{}}; "
                    f"(*_{sanitize_identifier(prop)})[key] = v; return *this; }}")
                lines.append(
                    f"    {name}& {prop_name}(const QJsonObject &obj) "
                    f"{{ if (!_{sanitize_identifier(prop)}) _{sanitize_identifier(prop)} = QMap<QString, QJsonValue>{{}}; "
                    f"for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_{sanitize_identifier(prop)})[it.key()] = it.value(); "
                    f"return *this; }}")
            else:
                lines.append(
                    f"    {name}& {add_name}(const QString &key, const QJsonValue &v) "
                    f"{{ _{sanitize_identifier(prop)}[key] = v; return *this; }}")
                lines.append(
                    f"    {name}& {prop_name}(const QJsonObject &obj) "
                    f"{{ for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _{sanitize_identifier(prop)}[it.key()] = it.value(); "
                    f"return *this; }}")
        # For maps with anyOf-typed values also emit a per-key adder
        elif prop in map_value_union_names:
            val_alias, full_map_type = map_value_union_names[prop]
            add_name = singular_add_name(prop)
            if is_optional:
                lines.append(
                    f"    {name}& {add_name}(const QString &key, {_param_type(val_alias)} v) "
                    f"{{ if (!_{sanitize_identifier(prop)}) _{sanitize_identifier(prop)} = {full_map_type}{{}}; "
                    f"(*_{sanitize_identifier(prop)})[key] = v; return *this; }}")
            else:
                lines.append(
                    f"    {name}& {add_name}(const QString &key, {_param_type(val_alias)} v) "
                    f"{{ _{sanitize_identifier(prop)}[key] = v; return *this; }}")
        # For typed QMap fields also emit a per-key adder
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            add_name = singular_add_name(prop)
            if is_optional:
                lines.append(
                    f"    {name}& {add_name}(const QString &key, {_param_type(val_type)} v) "
                    f"{{ if (!_{sanitize_identifier(prop)}) _{sanitize_identifier(prop)} = QMap<QString, {val_type}>{{}}; "
                    f"(*_{sanitize_identifier(prop)})[key] = v; return *this; }}")
            else:
                lines.append(
                    f"    {name}& {add_name}(const QString &key, {_param_type(val_type)} v) "
                    f"{{ _{sanitize_identifier(prop)}[key] = v; return *this; }}")
        # For QList fields also emit a per-element adder
        elif list_item_type is not None:
            add_name = singular_add_name(prop)
            bare_list = list_type(list_item_type)   # e.g. QStringList or QList<Foo>
            if is_optional:
                lines.append(
                    f"    {name}& {add_name}({_param_type(list_item_type)} v) "
                    f"{{ if (!_{sanitize_identifier(prop)}) _{sanitize_identifier(prop)} = {bare_list}{{}}; "
                    f"(*_{sanitize_identifier(prop)}).append(v); return *this; }}")
            else:
                lines.append(
                    f"    {name}& {add_name}({_param_type(list_item_type)} v) "
                    f"{{ _{sanitize_identifier(prop)}.append(v); return *this; }}")

    if has_additional_props and not _read_only:
        lines.append(f"    {name}& additionalProperties(const QString &key, const QJsonValue &v) {{ _additionalProperties.insert(key, v); return *this; }}")
        lines.append(f"    {name}& additionalProperties(const QJsonObject &obj) {{ for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }}")

    # Getter methods — return const ref with explicit type, named after the field (keyword-escaped).
    lines.append("")
    for prop, spec in props.items():
        prop_name = sanitize_identifier(prop)
        if is_const_string(spec):
            continue  # no stored member, no getter
        if prop in inline_enum_names:
            t = inline_enum_names[prop]
            is_optional = prop not in required
            decl_type = f"std::optional<{t}>" if is_optional else t
            lines.append(f"    const {decl_type}& {prop_name}() const {{ return _{sanitize_identifier(prop)}; }}")
            continue
        ret_type = prop_decl_types.get(prop, "auto")
        lines.append(f"    const {ret_type}& {prop_name}() const {{ return _{sanitize_identifier(prop)}; }}")
        # For open-map fields also emit an AsObject() convenience getter
        if is_open_map(spec):
            is_optional = prop not in required
            as_obj_name = prop_name + "AsObject"
            if is_optional:
                lines.append(
                    f"    QJsonObject {as_obj_name}() const {{ "
                    f"if (!_{sanitize_identifier(prop)}) return {{}}; "
                    f"QJsonObject o; for (auto it = _{sanitize_identifier(prop)}->constBegin(); it != _{sanitize_identifier(prop)}->constEnd(); ++it) o.insert(it.key(), it.value()); "
                    f"return o; }}")
            else:
                lines.append(
                    f"    QJsonObject {as_obj_name}() const {{ "
                    f"QJsonObject o; for (auto it = _{sanitize_identifier(prop)}.constBegin(); it != _{sanitize_identifier(prop)}.constEnd(); ++it) o.insert(it.key(), it.value()); "
                    f"return o; }}")

    if has_additional_props:
        lines.append(f"    const QJsonObject& additionalProperties() const {{ return _additionalProperties; }}")

    lines.append("")
    lines.append(f"    bool operator==(const {name} &other) const = default;")

    lines.append("};\n")
    # Emit serializers for nested child types at namespace scope (before parent serializers)
    for _child_serial in child_serial_blocks:
        lines.append(_child_serial)
    # Parse function — collected into fj_lines so we can strip co_return→return when no co_await
    fj_lines = []
    fj_lines.append(f"template<>")
    fj_lines.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
    fj_lines.append(f"    if (!val.isObject())")
    fj_lines.append(f"        co_return Utils::ResultError(\"Expected JSON object for {name}\");")
    fj_lines.append(f"    const QJsonObject obj = val.toObject();")
    
    # Validate required fields
    for req_field in required:
        fj_lines.append(f"    if (!obj.contains(\"{req_field}\"))")
        fj_lines.append(f"        co_return Utils::ResultError(\"Missing required field: {req_field}\");")
    
    fj_lines.append(f"    {name} result;")
    for prop, spec in props.items():
        prop_name = sanitize_identifier(prop)
        is_optional = prop not in required
        if prop in inline_enum_names:
            t_fj = f"{name}::{inline_enum_names[prop]}"
            if is_optional:
                fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
            else:
                fj_lines.append(f"    result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
        elif prop in sub_struct_names:
            t = sub_struct_names[prop]
            t_fj = f"{name}::{t}"  # inline sub-structs are always nested inside this struct
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject())")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
        elif _extract_ref(spec):
            t = ref_type(_extract_ref(spec))
            if is_integer_const_namespace(t, types):
                # Integer const namespace — read as int directly
                if is_optional:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isDouble())")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = obj[\"{prop}\"].toInt();")
                else:
                    fj_lines.append(f"    result._{sanitize_identifier(prop)} = obj.value(\"{prop}\").toInt();")
            else:
                # Use qualified short-name when the child type is nested inside this struct
                t_fj = f"{name}::{nested_short_names[t]}" if t in nested_short_names else t
                is_enum = is_enum_type(t, types)
                is_union = is_union_type_name(t, types)
                is_simple = is_simple_type_alias(t, types)
                if is_simple:
                    # Simple primitive alias — read value directly
                    simple_type = types[t].get("type") if t in types else None
                    if simple_type == "string":
                        fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isString())")
                    elif simple_type in ("integer", "number"):
                        fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isDouble())")
                    elif simple_type == "boolean":
                        fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isBool())")
                    else:
                        fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
                elif is_enum:
                    if is_optional:
                        fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
                        fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
                    else:
                        fj_lines.append(f"    result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
                elif is_union:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
                else:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject())")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
        elif _extract_nullable_ref(spec):
            t = _extract_nullable_ref(spec)
            is_patch = _is_patch_field(spec, is_optional, name, prop)
            if is_integer_const_namespace(t, types):
                fj_lines.append(f"    if (obj.contains(\"{prop}\") && !obj[\"{prop}\"].isNull())")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = obj[\"{prop}\"].toInt();")
                if is_patch:
                    fj_lines.append(f"    else if (obj.contains(\"{prop}\"))")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = std::nullopt;")
            else:
                t_fj = f"{name}::{nested_short_names[t]}" if t in nested_short_names else t
                # Nullable ref: only parse when present and non-null
                fj_lines.append(f"    if (obj.contains(\"{prop}\") && !obj[\"{prop}\"].isNull())")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(\"{prop}\", obj[\"{prop}\"]);")
                if is_patch:
                    fj_lines.append(f"    else if (obj.contains(\"{prop}\"))")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = std::nullopt;")
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            item_type = ref_type(_extract_ref(spec.get("items", {})))
            # Use qualified short-name when the item type is nested inside this struct
            item_type_fj = f"{name}::{nested_short_names[item_type]}" if item_type in nested_short_names else item_type
            if is_integer_const_namespace(item_type, types):
                item_type_fj = "int"
                item_expr = "v.toInt()"
            else:
                item_expr = f"co_await fromJson<{item_type_fj}>(\"{prop}\", v)"
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(item_type_fj)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            list_{prop_name}.append({item_expr});")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            result._{sanitize_identifier(prop)}.append({item_expr});")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif _three_state and _nullable_ref_array_type(spec):
            item_type = _nullable_ref_array_type(spec)
            item_type_fj = f"{name}::{nested_short_names[item_type]}" if item_type in nested_short_names else item_type
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            fj_lines.append(f"        {list_type(item_type_fj)} list_{prop_name};")
            fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
            fj_lines.append(f"            list_{prop_name}.append(co_await fromJson<{item_type_fj}>(\"{prop}\", v));")
            fj_lines.append(f"        }}")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            fj_lines.append(f"    }}")
            if _is_patch_field(spec, is_optional):
                fj_lines.append(f"    else if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isNull())")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = std::nullopt;")
        elif prop in array_item_struct_names:
            t = array_item_struct_names[prop]
            t_fj = f"{name}::{t}"  # inline sub-structs are always nested
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(t_fj)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            list_{prop_name}.append(co_await fromJson<{t_fj}>(\"{prop}\", v));")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            result._{sanitize_identifier(prop)}.append(co_await fromJson<{t_fj}>(\"{prop}\", v));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif prop in field_union_names:
            t = field_union_names[prop]
            fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t}>(\"{prop}\", obj[\"{prop}\"]);")
        elif prop in array_item_union_names:
            t = array_item_union_names[prop]
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(t)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            list_{prop_name}.append(co_await fromJson<{t}>(\"{prop}\", v));")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            result._{sanitize_identifier(prop)}.append(co_await fromJson<{t}>(\"{prop}\", v));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif spec.get("type") == "array":
            item_type = array_item_cpp_type(spec)
            _item_expr = _json_extract_expr(item_type, "v")
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(item_type)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                if _item_expr:
                    fj_lines.append(f"            list_{prop_name}.append({_item_expr});")
                else:
                    fj_lines.append(f"            // Unknown array item type: {item_type}")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                if _item_expr:
                    fj_lines.append(f"            result._{sanitize_identifier(prop)}.append({_item_expr});")
                else:
                    fj_lines.append(f"            // Unknown array item type: {item_type}")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif prop in map_value_union_names:
            val_alias, full_map_type = map_value_union_names[prop]
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject()) {{")
            fj_lines.append(f"        const QJsonObject mapObj_{prop_name} = obj[\"{prop}\"].toObject();")
            fj_lines.append(f"        {full_map_type} map_{prop_name};")
            fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it) {{")
            fj_lines.append(f"            map_{prop_name}.insert(it.key(), co_await fromJson<{val_alias}>(\"{prop}\", it.value()));")
            fj_lines.append(f"        }}")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = map_{prop_name};")
            fj_lines.append(f"    }}")
        elif is_open_map(spec):
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject()) {{")
            fj_lines.append(f"        const QJsonObject mapObj_{prop_name} = obj[\"{prop}\"].toObject();")
            fj_lines.append(f"        QMap<QString, QJsonValue> map_{prop_name};")
            fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it)")
            fj_lines.append(f"            map_{prop_name}.insert(it.key(), it.value());")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = map_{prop_name};")
            fj_lines.append(f"    }}")
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject()) {{")
            fj_lines.append(f"        const QJsonObject mapObj_{prop_name} = obj[\"{prop}\"].toObject();")
            fj_lines.append(f"        QMap<QString, {val_type}> map_{prop_name};")
            if val_type.startswith("QList<"):
                item_type = val_type[len("QList<"):-1]
                fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it) {{")
                fj_lines.append(f"            {val_type} list_{prop_name};")
                fj_lines.append(f"            for (const QJsonValue &v : it.value().toArray())")
                fj_lines.append(f"                list_{prop_name}.append(co_await fromJson<{item_type}>(v));")
                fj_lines.append(f"            map_{prop_name}.insert(it.key(), list_{prop_name});")
                fj_lines.append(f"        }}")
            elif val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                extract = _json_extract_expr(val_type, "it.value()") or "it.value().toString()"
                fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it)")
                fj_lines.append(f"            map_{prop_name}.insert(it.key(), {extract});")
            else:
                fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it) {{")
                fj_lines.append(f"            map_{prop_name}.insert(it.key(), co_await fromJson<{val_type}>(\"{prop}\", it.value()));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = map_{prop_name};")
            fj_lines.append(f"    }}")
        elif is_const_string(spec):
            const_value = spec["const"]
            fj_lines.append(f"    if (obj.value(\"{prop}\").toString() != \"{const_value}\")")
            fj_lines.append(f"        co_return Utils::ResultError(\"Field '{prop}' must be '{const_value}', got: \" + obj.value(\"{prop}\").toString());")
        else:
            if is_untyped_any(spec):
                base_t, is_nullable = None, False
                ct = "QJsonValue"
            else:
                base_t, is_nullable = _nullable_type(spec)
                t = base_t if base_t else spec.get("type", "string")
                ct = cpp_type(t)
            _scalar_expr = _json_extract_expr(ct, f'obj.value("{prop}")')
            braced_presence = is_optional and _is_patch_field(spec, is_optional)
            if is_optional:
                if braced_presence:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\")) {{")
                else:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
                indent = "        "
            else:
                indent = "    "
            if is_nullable:
                # Value is present (required) but may be JSON null — only assign when non-null
                fj_lines.append(f"{indent}if (!obj[\"{prop}\"].isNull()) {{")
                if _scalar_expr:
                    fj_lines.append(f"{indent}    result._{sanitize_identifier(prop)} = {_scalar_expr};")
                else:
                    fj_lines.append(f"{indent}    // Unknown property type: {ct}")
                if _is_patch_field(spec, is_optional):
                    fj_lines.append(f"{indent}}} else {{")
                    fj_lines.append(f"{indent}    result._{sanitize_identifier(prop)} = std::nullopt;")
                fj_lines.append(f"{indent}}}")
            elif _scalar_expr:
                fj_lines.append(f"{indent}result._{sanitize_identifier(prop)} = {_scalar_expr};")
            else:
                fj_lines.append(f"{indent}// Unknown property type: {ct}")
            if braced_presence:
                fj_lines.append(f"    }}")
    if has_additional_props:
        known_keys = list(props.keys())
        fj_lines.append(f"    {{")
        quoted = ", ".join(f'"{k}"' for k in known_keys)
        fj_lines.append(f"        const QSet<QString> knownKeys{{{quoted}}};")
        fj_lines.append(f"        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {{")
        fj_lines.append(f"            if (!knownKeys.contains(it.key()))")
        fj_lines.append(f"                result._additionalProperties.insert(it.key(), it.value());")
        fj_lines.append(f"        }}")
        fj_lines.append(f"    }}")
    fj_lines.append(f"    co_return result;")
    fj_lines.append("}")
    lines.extend(finalize_from_json(fj_lines))
    lines.append("")
    # toJson function — two-pass:
    # Pass 1: collect required single-value fields for an initializer list.
    # Pass 2: emit optional and array fields as post-init insert calls.
    if not _read_only:
        _emit_toJson(name, props, types, required, lines, has_additional_props,
                     inline_enum_names, sub_struct_names, array_item_struct_names,
                     array_item_union_names, field_union_names, map_value_union_names)
    lines.append("")  # ensures \n\n so the next comment is separated
    preamble = "".join(child_preamble_blocks) + "".join(sub_struct_blocks)
    return preamble + doc_comment(description) + "\n".join(lines)

def _split_parse_struct_output(code, name):
    """Split parse_struct() output for 'name' into (preamble, struct_def, serializers).

    preamble:    inline sub-struct blocks before the main struct (at namespace scope)
    struct_def:  optional doc comment + struct...{...};\n
    serializers: template fromJson + toJson functions

    Strategy: locate ``struct Name {`` at column 0 using MULTILINE (no DOTALL).
    Then look backwards for an immediately-preceding doc comment.  The old DOTALL
    version with a lazy wildcard in an optional group greedily spanned multiple
    /** ... */ blocks, causing incorrect splits.
    """
    # Locate `struct Name {` at the start of a line
    struct_pat = re.compile(
        r'^struct\s+' + re.escape(name) + r'\s+\{',
        re.MULTILINE,
    )
    m = struct_pat.search(code)
    if not m:
        return code, '', ''

    struct_line_start = m.start()
    prefix = code[:struct_line_start]

    # If the prefix ends with '*/\n', a doc comment closes immediately before the
    # struct line.  Include it in struct_def by scanning backwards for the
    # opening '/**' that sits at the start of a line.
    preamble_end = struct_line_start
    if prefix.endswith('*/\n'):
        idx = len(prefix) - 3  # point at the '*' in '*/'
        while idx >= 2:
            if prefix[idx - 2:idx + 1] == '/**':
                if idx == 2 or prefix[idx - 3] == '\n':
                    preamble_end = idx - 2
                break
            idx -= 1

    preamble = code[:preamble_end]
    rest = code[preamble_end:]

    # Find the outer struct closing `};` at column 0
    m2 = re.search(r'^};\n', rest, re.MULTILINE)
    if m2:
        struct_def = rest[:m2.end()]
        serializers = rest[m2.end():]
    else:
        struct_def = rest
        serializers = ''
    return preamble, struct_def, serializers

def _qualify_serializers(code, child_name, qualified_name):
    """In serializer code for 'child_name', replace type references with 'qualified_name'."""
    # Replace 'ChildName::' prefix first so that deeper nesting (grandchildren, etc.)
    # is handled before the unqualified 'ChildName' replacements.
    code = code.replace(
        f'{child_name}::',
        f'{qualified_name}::',
    )
    code = code.replace(
        f'Utils::Result<{child_name}>',
        f'Utils::Result<{qualified_name}>',
    )
    code = code.replace(
        f'fromJson<{child_name}>',
        f'fromJson<{qualified_name}>',
    )
    code = code.replace(
        f'toJson(const {child_name} &',
        f'toJson(const {qualified_name} &',
    )
    # Local result variable inside fromJson body
    code = code.replace(
        f'    {child_name} result;\n',
        f'    {qualified_name} result;\n',
    )
    return code

def _build_inline_union_code(union_alias, ref_names, types):
    """Build code lines for an inline union alias: using decl + fromJson + toJsonValue.

    The resulting lines are intended to be joined with '\n' and appended to
    sub_struct_blocks.  Callers must also update _emitted_variant_sigs.

    ref_names may include list types like QList<Foo> for anyOf items that are arrays.
    """
    variant_str = ", ".join(ref_names)
    union_lines = [f"using {union_alias} = std::variant<{variant_str}>;", ""]

    # Separate list types from object/scalar types
    list_types = [(rn, rn[6:-1]) for rn in ref_names if rn.startswith("QList<")]  # (full, inner)
    obj_names = [rn for rn in ref_names
                 if not rn.startswith("QList<") and rn not in _SCALAR_VARIANT_TYPES
                 and rn != "QJsonObject"]
    scalar_names = [rn for rn in ref_names if rn in _SCALAR_VARIANT_TYPES]

    fj = [
        "template<>",
        f"inline Utils::Result<{union_alias}> fromJson<{union_alias}>(const QJsonValue &val) {{",
    ]

    # A scalar alternative is recognised by the JSON type, before any object is
    # probed for a match.
    _scalar_tests = {"std::monostate": ("isNull()", "std::monostate{}"),
                     "bool": ("isBool()", "val.toBool()"),
                     "QString": ("isString()", "val.toString()"),
                     "int": ("isDouble()", "val.toInt()"),
                     "double": ("isDouble()", "val.toDouble()")}
    for scalar in scalar_names:
        if test := _scalar_tests.get(scalar):
            fj.append(f"    if (val.{test[0]})")
            fj.append(f"        co_return {union_alias}({test[1]});")

    # Handle list types first (check isArray)
    for list_t, inner_t in list_types:
        fj.append(f"    if (val.isArray()) {{")
        fj.append(f"        {list_t} list;")
        fj.append(f"        for (const QJsonValue &v : val.toArray())")
        fj.append(f"            list.append(co_await fromJson<{inner_t}>(v));")
        fj.append(f"        co_return {union_alias}(std::move(list));")
        fj.append(f"    }}")

    # Handle object types with dispatch or presence
    dispatch_field, dispatch_map = find_dispatch_field(obj_names, types) if (types and obj_names) else (None, None)
    if dispatch_field:
        fj.append(f"    if (!val.isObject())")
        fj.append(f'        co_return Utils::ResultError("Invalid {union_alias}: expected object or array");')
        fj.append(f"    const QString dispatchValue = val.toObject().value(\"{dispatch_field}\").toString();")
        first = True
        for ref_name, const_val in dispatch_map.items():
            kw = "if" if first else "else if"
            first = False
            fj.append(f"    {kw} (dispatchValue == \"{const_val}\")")
            fj.append(f"        co_return {union_alias}(co_await fromJson<{ref_name}>(val));")
        fj.append(f"    co_return Utils::ResultError(\"Invalid {union_alias}: unknown {dispatch_field} \\\"\" + dispatchValue + \"\\\"\");")
    else:
        presence_map = find_presence_dispatch(obj_names, types) if (types and obj_names) else {}
        if presence_map:
            fj.append(f"    if (!val.isObject())")
            fj.append(f'        co_return Utils::ResultError("Invalid {union_alias}: expected object or array");')
            fj.append(f"    const QJsonObject obj = val.toObject();")
            for ref_name in obj_names:
                if ref_name in presence_map:
                    field = presence_map[ref_name]
                    fj.append(f"    if (obj.contains(\"{field}\"))")
                    fj.append(f"        co_return {union_alias}(co_await fromJson<{ref_name}>(val));")
            for ref_name in obj_names:
                if ref_name not in presence_map:
                    fj.append(f"    {{")
                    fj.append(f"        auto result = fromJson<{ref_name}>(val);")
                    fj.append(f"        if (result) co_return {union_alias}(*result);")
                    fj.append(f"    }}")
        else:
            for ref_name in obj_names:
                fj.append(f"    {{")
                fj.append(f"        auto result = fromJson<{ref_name}>(val);")
                fj.append(f"        if (result) co_return {union_alias}(*result);")
                fj.append(f"    }}")
        if "QJsonObject" in ref_names:
            fj.append(f"    if (val.isObject())")
            fj.append(f"        co_return {union_alias}(val.toObject());")
        fj.append(f'    co_return Utils::ResultError("Invalid {union_alias}");')
    fj.append("}")
    union_lines.extend(finalize_from_json(fj))

    # toJsonValue — per-alternative when they do not serialize alike
    if not _read_only:
        if _variant_needs_typed_visitor(ref_names):
            union_lines.extend(_typed_toJsonValue_lines(union_alias, ref_names, types))
        else:
            union_lines.extend(_toJsonValue_visit_lines(union_alias))
    return union_lines

def _collect_sub_struct_output(sub_code, short_name, parent_name,
                               child_preamble_blocks, child_struct_inserts, child_serial_blocks):
    """Split parse_struct() output and distribute it into the caller's collection lists.

    Handles indentation of the struct definition and qualification of serializer names.
    """
    pre, struct_def, serials = _split_parse_struct_output(sub_code, short_name)
    if pre.strip():
        child_preamble_blocks.append(pre)
    if struct_def:
        indented = '\n'.join(('    ' + l) if l.strip() else l
                              for l in struct_def.rstrip('\n').split('\n')) + '\n'
        child_struct_inserts.append(indented)
    if serials.strip():
        child_serial_blocks.append(
            _qualify_serializers(serials, short_name, f"{parent_name}::{short_name}")
        )

def collect_refs_in_spec(spec):
    """Recursively collect all $ref type names from a schema spec (incl. nested inline objects)."""
    deps = set()
    if '$ref' in spec:
        deps.add(ref_type(spec['$ref']))
    for pspec in spec.get('properties', {}).values():
        deps |= collect_refs_in_spec(pspec)
    if 'items' in spec:
        deps |= collect_refs_in_spec(spec['items'])
    add_props = spec.get('additionalProperties')
    if isinstance(add_props, dict):
        deps |= collect_refs_in_spec(add_props)
    for key in ('allOf', 'anyOf', 'oneOf'):
        for item in spec.get(key, []):
            deps |= collect_refs_in_spec(item)
    return deps

def get_type_deps(type_spec):
    return collect_refs_in_spec(type_spec)

def topo_sort_types(types):
    from collections import defaultdict
    graph = defaultdict(set)
    for name, spec in types.items():
        graph[name] = get_type_deps(spec)
    visited = set()
    order = []
    def visit(n):
        if n in visited:
            return
        visited.add(n)
        for dep in sorted(graph[n]):
            if dep in types:
                visit(dep)
        order.append(n)
    for n in types:
        visit(n)
    return order


def compute_exclusive_parents(types):
    """Top-level $defs types are never nested — they remain at namespace scope.
    Only inline anonymous sub-structs (handled inside parse_struct via
    needs_sub_struct / array_items_need_sub_struct) are nested.
    """
    return {}

def _split_code_block(code_block, export_macro=None):
    """Split a generated code block into (header_code, cpp_code).

    header_code: type declarations (enum class, struct, using), plus forward declarations
                 of free functions.
    cpp_code:    function definitions (without 'inline' keyword).

    The split is done by parsing the generated text line-by-line to identify
    function definitions (inline ... { ... }) and template<> specializations.

    If export_macro is provided, forward declarations in the header are annotated
    with the macro (e.g. ``ACPLIB_EXPORT``).
    """
    if not code_block or not code_block.strip():
        return code_block, ''

    lines = code_block.split('\n')
    h_lines = []
    cpp_lines = []
    i = 0

    def _find_matching_brace(lines, start_line):
        """Find the line index of the closing '}' that matches the '{' on start_line.
        Returns the index of the line containing the matching '}'."""
        depth = 0
        for j in range(start_line, len(lines)):
            depth += lines[j].count('{') - lines[j].count('}')
            if depth <= 0:
                return j
        return len(lines) - 1

    def _make_forward_decl(func_lines):
        """Given lines of a function definition, produce a forward declaration.
        For template<> specializations, include the template<> prefix.
        Returns a list of declaration lines.
        When export_macro is set, inserts it after template<> (if present)
        or at the start of the return type."""
        # Find the line with the function signature (has the opening '{')
        decl_lines = []
        is_template = False
        for fl in func_lines:
            stripped = fl.strip()
            if stripped == 'template<>':
                decl_lines.append(stripped)
                is_template = True
                continue
            if '{' in stripped:
                # Extract everything before the '{'
                sig = stripped[:stripped.index('{')].strip()
                # Remove 'inline' keyword
                sig = re.sub(r'\binline\s+', '', sig)
                if export_macro:
                    sig = export_macro + ' ' + sig
                decl_lines.append(sig + ';')
                break
            else:
                # Part of multi-line signature
                cleaned = re.sub(r'\binline\s+', '', stripped)
                decl_lines.append(cleaned)
        return decl_lines

    def _format_func_def(func_lines):
        """Remove 'inline' keyword from function definition lines.
        Also moves the opening brace of function signatures to a new line (Allman style)."""
        result = []
        for fl in func_lines:
            fl = re.sub(r'\binline\s+', '', fl)
            # Move the function-level opening brace to its own line.
            # Only applies to non-indented lines ending with ' {' — these are
            # function signatures, not lambda captures or nested blocks.
            if not fl.startswith(' ') and fl.rstrip().endswith('{'):
                sig = fl.rstrip()[:-1].rstrip()
                if sig:  # skip a bare '{' line
                    result.append(sig)
                    result.append('{')
                    continue
            result.append(fl)
        return result

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Detect template<> inline ... on a single line (type alias fromJson)
        if stripped.startswith('template<>') and 'inline' in stripped and '{' in stripped:
            end = _find_matching_brace(lines, i)
            func_lines = lines[i:end + 1]
            # For _make_forward_decl, split the template<> prefix onto its own line
            split_lines = ['template<>'] + [fl.replace('template<> ', '', 1) if fl.strip().startswith('template<>') else fl for fl in func_lines[0:1]] + func_lines[1:]
            h_lines.extend(_make_forward_decl(split_lines))
            h_lines.append('')
            cpp_lines.extend(_format_func_def(func_lines))
            cpp_lines.append('')
            i = end + 1
            continue

        # Detect template<> specialization (fromJson, etc.) — template<> on its own line
        if stripped == 'template<>':
            # Next non-empty line should be the function signature
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                if 'inline' in next_line and '{' in next_line:
                    # Single or multi-line function starting on next line
                    end = _find_matching_brace(lines, i + 1)
                    func_lines = lines[i:end + 1]
                    # Forward declaration for header
                    h_lines.extend(_make_forward_decl(func_lines))
                    h_lines.append('')
                    # Definition for cpp
                    cpp_lines.extend(_format_func_def(func_lines))
                    cpp_lines.append('')
                    i = end + 1
                    continue
                elif 'inline' in next_line and '{' not in next_line:
                    # Multi-line signature — find the line with '{'
                    sig_end = i + 1
                    while sig_end < len(lines) and '{' not in lines[sig_end]:
                        sig_end += 1
                    end = _find_matching_brace(lines, sig_end)
                    func_lines = lines[i:end + 1]
                    h_lines.extend(_make_forward_decl(func_lines))
                    h_lines.append('')
                    cpp_lines.extend(_format_func_def(func_lines))
                    cpp_lines.append('')
                    i = end + 1
                    continue
            # Not a function def, keep in header
            h_lines.append(line)
            i += 1
            continue

        # Detect inline free functions at column 0 (toString, toJson, toJsonValue, dispatchValue, shared field accessors)
        if stripped.startswith('inline ') and '{' in stripped and not stripped.startswith('inline enum') and not stripped.startswith('inline struct'):
            end = _find_matching_brace(lines, i)
            func_lines = lines[i:end + 1]
            h_lines.extend(_make_forward_decl(func_lines))
            h_lines.append('')
            cpp_lines.extend(_format_func_def(func_lines))
            cpp_lines.append('')
            i = end + 1
            continue

        # Detect doc comments that precede inline functions (/** ... */ followed by inline ...)
        if stripped.startswith('/**') and stripped.endswith('*/'):
            # Single-line doc comment — check if next non-empty line is an inline function
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines) and lines[j].strip().startswith('inline ') and '{' in lines[j].strip():
                end = _find_matching_brace(lines, j)
                func_lines = lines[i:end + 1]  # include doc comment
                # For header: doc comment + forward decl
                h_lines.append(line)  # doc comment
                h_lines.extend(_make_forward_decl(lines[j:end + 1]))
                h_lines.append('')
                # For cpp: no doc comment, just definition
                cpp_lines.extend(_format_func_def(lines[j:end + 1]))
                cpp_lines.append('')
                i = end + 1
                continue

        # Everything else stays in the header (struct defs, enum defs, using decls, blank lines, comments)
        h_lines.append(line)
        i += 1

    return '\n'.join(h_lines), '\n'.join(cpp_lines)


def main():
    parser = argparse.ArgumentParser(
        description="Generate C++ header from a JSON schema."
    )
    parser.add_argument("schema", help="Path to the input JSON schema file")
    parser.add_argument("output", help="Path to the output C++ header file")
    parser.add_argument(
        "--namespace",
        default="GeneratedSchema",
        help="C++ namespace to wrap the generated code in (default: GeneratedSchema)",
    )
    parser.add_argument(
        "--no-comments",
        action="store_true",
        default=False,
        help="Suppress all doc and inline comments in the generated output",
    )
    parser.add_argument(
        "--cpp-output",
        default=None,
        help="Path to the output C++ implementation file. When provided, function "
             "definitions are written here and only declarations remain in the header.",
    )
    parser.add_argument(
        "--export-macro",
        default=None,
        help="DLL export/import macro name (e.g. ACPLIB_EXPORT). Used only with "
             "--cpp-output to annotate declarations in the header.",
    )
    parser.add_argument(
        "--export-header",
        default=None,
        help="Header to #include for the export macro (e.g. acp_global.h). "
             "Used only with --export-macro.",
    )
    parser.add_argument(
        "--read-only",
        action="store_true",
        default=False,
        help="Skip generation of setter/builder methods (setXYZ, addXYZ). "
             "Only getters and fromJson/toJson are emitted.",
    )
    parser.add_argument(
        "--three-state",
        action="store_true",
        default=False,
        help="Model optional nullable fields as three-state Patch<T> "
             "(absent/null/value) instead of std::optional<T>. Needed for "
             "schemas with upsert patch semantics (e.g. ACP v2).",
    )
    parser.add_argument(
        "--no-cxx20",
        action="store_true",
        default=False,
        help="Avoid C++20-only constructs, for targets built as C++17. fromJson "
             "then propagates errors with explicit early returns instead of "
             "co_await, and <utils/co_result.h> is not included.",
    )
    args = parser.parse_args()
    global _emit_comments, _emitted_variant_sigs, _read_only, _three_state, _cxx20
    _emit_comments = not args.no_comments
    _read_only = args.read_only
    _three_state = args.three_state
    _cxx20 = not args.no_cxx20
    _emitted_variant_sigs = {}  # reset per run
    _canonical_alias.clear()
    schema_path = Path(args.schema)
    output_path = Path(args.output)
    namespace = args.namespace
    with open(schema_path, encoding="utf-8") as f:
        schema = json.load(f)
    normalize_nullable_primitives(schema)

    def _get_types(s):
        if "definitions" in s:
            return dict(s["definitions"])
        if "components" in s and "schemas" in s["components"]:
            return dict(s["components"]["schemas"])
        if "$defs" in s:
            return dict(s["$defs"])
        return None

    def _collect_external_refs(obj):
        """Yield relative file paths from $ref values that point to external files."""
        if isinstance(obj, dict):
            ref = obj.get("$ref")
            if isinstance(ref, str) and not ref.startswith("#") and not ref.startswith("http"):
                # Strip any internal pointer (e.g. "file.json#/definitions/Foo")
                yield ref.split("#")[0]
            for v in obj.values():
                yield from _collect_external_refs(v)
        elif isinstance(obj, list):
            for item in obj:
                yield from _collect_external_refs(item)

    def _title_to_type_name(title):
        """Convert a schema title to a C++ type name, e.g. 'ACP Agent' -> 'ACPAgent'."""
        words = re.sub(r'[^A-Za-z0-9 ]', '', title).split()
        return ''.join(w[0].upper() + w[1:] for w in words if w)

    def _rewrite_external_refs(obj, ref_map):
        """Recursively rewrite external $ref values using ref_map."""
        if isinstance(obj, dict):
            if "$ref" in obj and obj["$ref"] in ref_map:
                obj["$ref"] = ref_map[obj["$ref"]]
            for v in obj.values():
                _rewrite_external_refs(v, ref_map)
        elif isinstance(obj, list):
            for item in obj:
                _rewrite_external_refs(item, ref_map)

    def _root_object_spec(s):
        """Extract a type spec dict from a schema's root object, or None."""
        if s.get("type") != "object" or not s.get("properties"):
            return None
        spec = {
            "type": "object",
            "properties": s["properties"],
            "required": s.get("required", []),
            "description": s.get("description", ""),
        }
        if "additionalProperties" in s:
            spec["additionalProperties"] = s["additionalProperties"]
        return spec

    # Support 'definitions', 'components.schemas', or '$defs'
    types = _get_types(schema)
    if types is None:
        # Resolve external $ref files and merge their definitions
        types = {}
        seen_files = set()
        ref_map = {}
        for rel_path in _collect_external_refs(schema):
            if rel_path in seen_files:
                continue
            seen_files.add(rel_path)
            ext_path = schema_path.parent / rel_path
            if ext_path.is_file():
                with open(ext_path, encoding="utf-8") as ef:
                    ext_schema = json.load(ef)
                ext_types = _get_types(ext_schema)
                if ext_types:
                    types.update(ext_types)
                # Add external root object as a type if it defines properties
                ext_root = _root_object_spec(ext_schema)
                if ext_root:
                    ext_name = _title_to_type_name(
                        ext_schema.get("title", ext_path.stem.split('.')[0].capitalize()))
                    ref_map[rel_path] = f"#/external/{ext_name}"
                    types[ext_name] = ext_root
        # Rewrite bare external-file $refs to internal type references
        if ref_map:
            _rewrite_external_refs(schema, ref_map)

    # Add root schema object as a type if it defines properties
    root_spec = _root_object_spec(schema)
    if root_spec:
        root_name = _title_to_type_name(
            schema.get("title", schema_path.stem.split('.')[0].capitalize()))
        if root_name not in types:
            if types is None:
                types = {}
            types[root_name] = root_spec

    if not types:
        print("Schema format not recognized. Needs 'definitions', 'components.schemas', or '$defs'.")
        sys.exit(1)

    # Inline the recursive arbitrary-JSON defs before anything inspects the graph.
    builtins_present = {n for n in BUILTIN_JSON_DEFS if n in types}
    if builtins_present:
        for name in builtins_present:
            del types[name]
        inline_builtin_json_refs(types, builtins_present)

    # --- BEGIN FULL REFACTOR: Robust dependency-graph-based emission ---
    global _recursive_fields, _needs_int_json
    _recursive_fields = compute_recursive_fields(types)
    _needs_int_json = uses_int_constant_namespace_indirectly(types)
    export_header = args.export_header if args.cpp_output else None
    code = [make_header(namespace, export_header=export_header)]
    order = topo_sort_types(types)

    # Build dependency graph for all types (structs, enums, aliases, unions)
    emitted = set()
    variant_signatures = {}
    alias_fromjson_emitted = set()  # track underlying types that already have fromJson
    if _needs_int_json:
        alias_fromjson_emitted.add("int")  # the header already provides fromJson<int>

    # Helper: extract all $ref-referenced type names from a spec (non-recursive).
    # Only follows actual JSON $ref links, NOT description text.
    def collect_refs(spec):
        """Walk a spec dict/list and yield every $ref type name."""
        if isinstance(spec, dict):
            if "$ref" in spec:
                yield ref_type(spec["$ref"])
            for key, val in spec.items():
                if key == "description":
                    continue  # skip free text
                yield from collect_refs(val)
        elif isinstance(spec, list):
            for item in spec:
                yield from collect_refs(item)

    def extract_type_alias_deps(alias_spec, seen=None):
        """Return the set of type names that *alias_spec* directly references via $ref,
        plus the transitive closure of their $ref references.
        Only follows structural $ref links; description strings are ignored."""
        if seen is None:
            seen = set()
        deps = set()
        for t in collect_refs(alias_spec):
            if t in types and t not in seen:
                deps.add(t)
                seen.add(t)
                deps |= extract_type_alias_deps(types[t], seen)
        return deps

    # Helper: is this a type alias (including unions/variants)?
    def is_type_alias(name, spec):
        if is_union_type(spec):
            return True
        if not ("properties" in spec or "enum" in spec or is_allof_type(spec)):
            return True
        return False

    # Emit all types in topological (dependency) order.
    # The topo_sort_types result guarantees that when we reach a type,
    # all types it references via $ref have already been visited.
    # For union/alias types we additionally verify deps are emitted,
    # and defer + retry if needed (handles any residual cross-union deps).
    deferred = []
    for name in order:
        spec = types[name]
        # 1. Enums (classic pattern or anyOf/oneOf with const values)
        if ("enum" in spec and spec.get("type") == "string") or _extract_anyof_enum(spec):
            code.append(parse_enum(name, spec))
            emitted.add(name)
            continue
        # 2. Structs (objects)
        if "properties" in spec or is_allof_type(spec):
            if is_allof_type(spec):
                merged_props, merged_required = resolve_allof(spec, types)
                if merged_props:
                    code.append(parse_struct(name, merged_props, types, merged_required, spec.get("description", "")))
                    emitted.add(name)
            else:
                has_additional_props = accepts_additional_props(spec)
                # Types with both properties and oneOf (discriminated struct+variant pattern)
                # need additionalProperties to preserve variant-specific fields
                if not has_additional_props and ("oneOf" in spec or "anyOf" in spec):
                    has_additional_props = True
                code.append(parse_struct(name, spec["properties"], types, spec.get("required", []), spec.get("description", ""), has_additional_props=has_additional_props))
                emitted.add(name)
            continue
        # 3. Type aliases (including unions/variants)
        if is_type_alias(name, spec):
            deps = set(collect_refs(spec)) & set(types.keys())
            if all(d in emitted for d in deps):
                _emit_type_alias(name, spec, code, emitted, variant_signatures, alias_fromjson_emitted, types)
            else:
                deferred.append(name)
            continue

    # Retry deferred union/alias types until all are emitted
    while deferred:
        progress = False
        still_deferred = []
        for name in deferred:
            spec = types[name]
            deps = set(collect_refs(spec)) & set(types.keys())
            if all(d in emitted for d in deps):
                _emit_type_alias(name, spec, code, emitted, variant_signatures, alias_fromjson_emitted, types)
                progress = True
            else:
                still_deferred.append(name)
        deferred = still_deferred
        if not progress:
            raise RuntimeError("Could not resolve all type dependencies. Remaining: " + ", ".join(deferred))

    code.append(make_footer(namespace))

    if args.cpp_output:
        # Split mode: separate declarations (.h) from definitions (.cpp)
        cpp_output_path = Path(args.cpp_output)
        header_filename = output_path.name
        export_macro = args.export_macro

        h_blocks = [make_header(namespace, export_header=export_header)]
        cpp_blocks = [make_cpp_preamble(namespace, header_filename)]

        # Skip the first element (make_header) and last (make_footer) —
        # process only the type code blocks in between.
        for block in code[1:-1]:
            h_part, cpp_part = _split_code_block(block, export_macro=export_macro)
            if h_part and h_part.strip():
                h_blocks.append(h_part)
            if cpp_part and cpp_part.strip():
                cpp_blocks.append(cpp_part)

        h_blocks.append(make_footer(namespace))
        cpp_blocks.append(make_footer(namespace))

        h_output = "\n".join(h_blocks)
        h_output = re.sub(r'\n{3,}', '\n\n', h_output)
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(h_output)

        cpp_output = "\n".join(cpp_blocks)
        cpp_output = re.sub(r'\n{3,}', '\n\n', cpp_output)
        with open(cpp_output_path, "w", encoding="utf-8") as f:
            f.write(cpp_output)

        print(f"Generated C++ header at {output_path}")
        print(f"Generated C++ implementation at {cpp_output_path}")
    else:
        output = "\n".join(code)
        output = re.sub(r'\n{3,}', '\n\n', output)
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(output)
        print(f"Generated C++ header at {output_path}")

def _emit_type_alias(name, spec, code, emitted, variant_signatures, alias_fromjson_emitted, types):
    """Emit a type alias (union or primitive alias) and record it as emitted."""
    if is_union_type(spec):
        # Try discriminated union first (handles explicit discriminator field)
        disc_result = _parse_discriminated_union(name, spec, types=types)
        if disc_result is not None:
            result, _ = disc_result
            code.append(result)
            emitted.add(name)
            return
        _, signature = parse_union(name, spec, skip_to_json=True, skip_from_json=True,
                                   types=types, emit_item_unions=False)
        if _variant_alias_for(signature):
            result, _ = parse_union(name, spec, skip_to_json=True, skip_from_json=True, types=types)
            code.append(result)
            _canonical_alias[name] = f"std::variant<{_canonical_signature(signature)}>"
        else:
            result, _ = parse_union(name, spec, skip_to_json=_read_only, skip_from_json=False, types=types)
            code.append(result)
            _register_variant_alias(signature, name)
    elif enum_keyed_map_info(spec) is not None:
        enum_vals, val_type = enum_keyed_map_info(spec)
        prefix = doc_comment(spec.get('description', ''))
        lines = []
        if prefix:
            lines.append(prefix.rstrip('\n'))

        # Build (sanitized_field, original_json_key) pairs
        pairs = [(sanitize_identifier(v), v) for v in enum_vals]

        lines.append(f"struct {name} {{")
        for field, orig in pairs:
            lines.append(f"    std::optional<{val_type}> _{field};")
        lines.append(f"")
        # Builder-style setters
        if not _read_only:
            for field, orig in pairs:
                lines.append(f"    {name}& {field}(const std::optional<{val_type}> & v) {{ _{field} = v; return *this; }}")
            lines.append(f"")
        # Const reference getters
        for field, orig in pairs:
            lines.append(f"    const std::optional<{val_type}>& {field}() const {{ return _{field}; }}")
        lines.append(f"}};")
        lines.append(f"")

        # fromJson
        fj = []
        fj.append(f"template<>")
        fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
        fj.append(f"    if (!val.isObject())")
        fj.append(f'        co_return Utils::ResultError("Expected JSON object for {name}");')
        fj.append(f"    const QJsonObject obj = val.toObject();")
        fj.append(f"    {name} result;")
        for field, orig in pairs:
            if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                extract = _json_extract_expr(val_type, f'obj.value("{orig}")') or f'obj.value("{orig}").toString()'
                fj.append(f'    if (obj.contains("{orig}"))')
                fj.append(f"        result._{field} = {extract};")
            else:
                fj.append(f'    if (obj.contains("{orig}"))')
                fj.append(f'        result._{field} = co_await fromJson<{val_type}>(obj.value("{orig}"));')
        fj.append(f"    co_return result;")
        fj.append(f"}}")
        lines.extend(finalize_from_json(fj))
        lines.append(f"")

        # toJson
        if not _read_only:
            lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
            lines.append(f"    QJsonObject obj;")
            for field, orig in pairs:
                if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                    lines.append(f'    if (data._{field}.has_value())')
                    lines.append(f'        obj.insert("{orig}", QJsonValue(*data._{field}));')
                else:
                    lines.append(f'    if (data._{field}.has_value())')
                    lines.append(f'        obj.insert("{orig}", toJson(*data._{field}));')
            lines.append(f"    return obj;")
            lines.append(f"}}")

        code.append("\n".join(lines) + "\n")
    elif is_typed_map(spec) is not None:
        val_type = is_typed_map(spec)
        map_type = f"QMap<QString, {val_type}>"
        alias_lines = [f"using {name} = {map_type};"]
        # fromJson
        fj = []
        fj.append(f"template<>")
        fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
        fj.append(f"    if (!val.isObject())")
        fj.append(f'        co_return Utils::ResultError("Expected JSON object for {name}");')
        fj.append(f"    const QJsonObject obj = val.toObject();")
        fj.append(f"    {name} result;")
        if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
            extract = _json_extract_expr(val_type, "it.value()") or "it.value().toString()"
            fj.append(f"    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)")
            fj.append(f"        result.insert(it.key(), {extract});")
        else:
            fj.append(f"    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)")
            fj.append(f"        result.insert(it.key(), co_await fromJson<{val_type}>(it.value()));")
        fj.append(f"    co_return result;")
        fj.append(f"}}")
        alias_lines.extend(finalize_from_json(fj))
        # toJson
        if not _read_only:
            alias_lines.append(f"")
            alias_lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
            alias_lines.append(f"    QJsonObject obj;")
            if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                alias_lines.append(f"    for (auto it = data.constBegin(); it != data.constEnd(); ++it)")
                alias_lines.append(f"        obj.insert(it.key(), QJsonValue(it.value()));")
            else:
                alias_lines.append(f"    for (auto it = data.constBegin(); it != data.constEnd(); ++it)")
                alias_lines.append(f"        obj.insert(it.key(), toJson(it.value()));")
            alias_lines.append(f"    return obj;")
            alias_lines.append(f"}}")
        code.append("\n".join(alias_lines) + "\n")
    elif is_open_map(spec):
        map_type = "QMap<QString, QJsonValue>"
        alias_lines = [f"using {name} = {map_type};"]
        # fromJson
        fj = []
        fj.append(f"template<>")
        fj.append(f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
        fj.append(f"    if (!val.isObject())")
        fj.append(f'        return Utils::ResultError("Expected JSON object for {name}");')
        fj.append(f"    const QJsonObject obj = val.toObject();")
        fj.append(f"    {name} result;")
        fj.append(f"    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)")
        fj.append(f"        result.insert(it.key(), it.value());")
        fj.append(f"    return result;")
        fj.append(f"}}")
        alias_lines.extend(fj)
        # toJson
        if not _read_only:
            alias_lines.append(f"")
            alias_lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
            alias_lines.append(f"    QJsonObject obj;")
            alias_lines.append(f"    for (auto it = data.constBegin(); it != data.constEnd(); ++it)")
            alias_lines.append(f"        obj.insert(it.key(), it.value());")
            alias_lines.append(f"    return obj;")
            alias_lines.append(f"}}")
        code.append("\n".join(alias_lines) + "\n")
    elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
        item_type = ref_cpp_type(_extract_ref(spec["items"]), types)
        list_t = list_type(item_type)
        alias_lines = [doc_comment(spec.get('description', '')).rstrip('\n')] if _emit_comments \
            and spec.get('description') else []
        alias_lines.append(f"using {name} = {list_t};")
        _canonical_alias[name] = f"QList<{_canonical_alias.get(item_type, item_type)}>"
        # Aliases are typedefs, so two aliases of the same list type would
        # define the same specialization twice.
        if list_t not in alias_fromjson_emitted:
            alias_fromjson_emitted.add(list_t)
            fj = [
                "template<>",
                f"inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{",
                "    if (!val.isArray())",
                f'        co_return Utils::ResultError("Expected JSON array for {name}");',
                f"    {name} result;",
                "    for (const QJsonValue &v : val.toArray())",
                f"        result.append(co_await fromJson<{item_type}>(v));",
                "    co_return result;",
                "}",
            ]
            alias_lines.append("")
            alias_lines.extend(finalize_from_json(fj))
            if not _read_only:
                item_fn = "" if not needs_to_json(item_type, types) else (
                    "toJsonValue" if is_enum_type(item_type, types)
                    or is_union_type_name(item_type, types) else "toJson")
                alias_lines.append("")
                alias_lines.append(f"inline QJsonArray toJson(const {name} &data) {{")
                alias_lines.append("    QJsonArray arr;")
                alias_lines.append(f"    for (const auto &v : data) arr.append({json_call(item_fn, 'v')});")
                alias_lines.append("    return arr;")
                alias_lines.append("}")
        code.append("\n".join(alias_lines) + "\n")
    else:
        # Handle $ref-only type aliases (e.g. "EmptyResult": {"$ref": "#/$defs/Result"})
        if '$ref' in spec:
            target_type = ref_type(spec['$ref'])
        else:
            primitive_map = {
                'string': 'QString',
                'integer': 'int',
                'number': 'double',
                'boolean': 'bool',
                'object': 'QJsonObject',
                'array': 'QJsonArray',
            }
            target_type = primitive_map.get(spec.get('type'), spec.get('type'))
        if target_type:
            alias_lines = [f"using {name} = {target_type};"]
            # Generate fromJson specialization so that fromJson<AliasName>
            # works (the base template is = delete).
            # Since type aliases are just typedefs, fromJson<A> and fromJson<B>
            # are the same specialization when A and B alias the same type.
            # Only emit the first one to avoid C2766 duplicate definition.
            # Since type aliases are typedefs, fromJson<Alias> and fromJson<Target>
            # are the same specialization.  Skip when the target type already has
            # a fromJson (emitted by parse_struct/parse_enum/parse_union or a
            # prior alias to the same type) to avoid C2766 on MSVC.
            skip_fromjson = target_type in alias_fromjson_emitted or target_type in emitted
            json_type = spec.get('type')
            if skip_fromjson:
                pass
            elif json_type == 'string':
                alias_lines.append(f"template<> inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
                alias_lines.append(f"    if (!val.isString()) return Utils::ResultError(\"Expected string\");")
                alias_lines.append(f"    return val.toString();")
                alias_lines.append("}")
            elif json_type == 'integer':
                alias_lines.append(f"template<> inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
                alias_lines.append(f"    if (!val.isDouble()) return Utils::ResultError(\"Expected number\");")
                alias_lines.append(f"    return static_cast<int>(val.toDouble());")
                alias_lines.append("}")
            elif json_type == 'number':
                alias_lines.append(f"template<> inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
                alias_lines.append(f"    if (!val.isDouble()) return Utils::ResultError(\"Expected number\");")
                alias_lines.append(f"    return val.toDouble();")
                alias_lines.append("}")
            elif json_type == 'boolean':
                alias_lines.append(f"template<> inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
                alias_lines.append(f"    if (!val.isBool()) return Utils::ResultError(\"Expected boolean\");")
                alias_lines.append(f"    return val.toBool();")
                alias_lines.append("}")
            elif '$ref' in spec:
                alias_lines.append(f"template<> inline Utils::Result<{name}> fromJson<{name}>(const QJsonValue &val) {{")
                alias_lines.append(f"    return fromJson<{target_type}>(val);")
                alias_lines.append("}")
            if not skip_fromjson:
                alias_fromjson_emitted.add(target_type)
            _canonical_alias[name] = _canonical_alias.get(target_type, target_type)
            code.append("\n".join(alias_lines) + "\n")
        else:
            code.append(f"// Skipped unknown type alias: {name}\n")
    emitted.add(name)

if __name__ == "__main__":
    main()
