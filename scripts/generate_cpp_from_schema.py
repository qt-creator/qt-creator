# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import argparse
import json
import re
import sys
from pathlib import Path

# Controls whether doc/inline comments are emitted. Set via --no-comments.
_emit_comments: bool = True

# Maps variant_type_str -> alias_name for inline union aliases already emitted.
# Prevents redefinition of fromJson/toJsonValue for equivalent variant types.
_emitted_variant_sigs: dict = {}

def make_header(namespace: str, export_header: str = None) -> str:
    export_include = f'\n#include "{export_header}"\n' if export_header else ''
    return f'''/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 {sys.executable} \\
  {sys.argv[0]} \\
  {' '.join(sys.argv[1:])}
*/
#pragma once
{export_include}
#include <utils/result.h>
#include <utils/co_result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <variant>

namespace {namespace} {{

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;
'''

def make_footer(namespace: str) -> str:
    return f'''
}} // namespace {namespace}
'''

def make_cpp_preamble(namespace: str, header_filename: str) -> str:
    return f'''// This file is auto-generated. Do not edit manually.
#include "{header_filename}"

namespace {namespace} {{
'''

def use_return_if_no_co_await(lines):
    """Replace co_return with return in lines where co_await is never used."""
    if not any('co_await' in line for line in lines):
        return [line.replace('co_return', 'return') for line in lines]
    return lines


def doc_comment(text, indent=''):
    """Format a description as a /** ... */ Doxygen block comment."""
    if not _emit_comments:
        return ''
    if not text:
        return ''
    text = text.strip()
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
            fj.append(f"    const QString str = val.toString();")
            for s, orig in pairs:
                fj.append(f'    if (str == "{orig}") co_return {name}::{s};')
            fj.append(f'    co_return Utils::ResultError("Invalid {name} value: " + str);')
            fj.append("}")
            lines.extend(use_return_if_no_co_await(fj))
            lines.append("")
            # For serialization to JSON, use toString
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

def is_simple_type_alias(type_name, types):
    """Check if a type is a simple primitive alias (e.g. using Foo = QString).
    These types convert directly to QJsonValue without needing toJson()."""
    if type_name in types:
        spec = types[type_name]
        return spec.get("type") in ("string", "integer", "number", "boolean") and "enum" not in spec and "properties" not in spec
    return False

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
    for item in items:
        ref = _extract_ref(item)
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

    # Check if we need a wrapper struct (duplicates exist or inline-only variants)
    if has_duplicates or has_inline_only:
        return _gen_discriminated_wrapper_struct(name, disc_field, variants, unique_cpp_types, spec, types)
    else:
        # No duplicates — generate a using alias with discriminator-based dispatch
        return _gen_discriminated_alias(name, disc_field, variants, unique_cpp_types, spec, types)


def _gen_discriminated_wrapper_struct(name, disc_field, variants, unique_cpp_types, spec, types):
    """Generate a wrapper struct for discriminated unions with duplicate types or inline-only variants."""
    prefix = doc_comment(spec.get('description', ''))
    lines = []

    # Build the variant type
    # Include std::monostate if there are inline-only (no-payload) variants
    has_inline_only = any(v[0] is None for v in variants)
    variant_members = list(unique_cpp_types)
    if has_inline_only:
        variant_members.insert(0, "std::monostate")
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

    fj.append(f"    else")
    fj.append(f'        co_return Utils::ResultError("Invalid {name}: unknown {disc_field} \\"" + kind + "\\"");')
    fj.append(f"    co_return result;")
    fj.append(f"}}")
    lines.extend(use_return_if_no_co_await(fj))
    lines.append(f"")

    # toJson
    lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
    lines.append(f"    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {{")
    if has_inline_only:
        lines.append(f"        using T = std::decay_t<decltype(v)>;")
        lines.append(f"        if constexpr (std::is_same_v<T, std::monostate>) return {{}};")
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


def _gen_discriminated_alias(name, disc_field, variants, unique_cpp_types, spec, types):
    """Generate a using alias with discriminator-based dispatch for unions without duplicate types."""
    prefix = doc_comment(spec.get('description', ''))
    lines = []
    variant_type_str = ", ".join(unique_cpp_types)

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

    fj.append(f'    co_return Utils::ResultError("Invalid {name}: unknown {disc_field} \\"" + dispatchValue + "\\"");')
    fj.append(f"}}")
    lines.extend(use_return_if_no_co_await(fj))
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
    lines.append(f"        return {{}};")
    lines.append(f"    }}, val);")
    lines.append(f"}}")
    lines.append(f"")

    # toJson — re-insert the discriminator field
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

    # Shared field accessors
    ref_names = unique_cpp_types
    if types:
        for field, ret_type in find_shared_fields(ref_names, types):
            lines.append(f"")
            if _emit_comments:
                lines.append(f"/** Returns the '{field}' field from the active variant. */")
            lines.append(f"inline {ret_type} {field}(const {name} &val) {{")
            lines.append(f"    return std::visit([](const auto &v) -> {ret_type} {{ return v._{field}; }}, val);")
            lines.append(f"}}")

    return prefix + "\n".join(lines), variant_type_str


def parse_union(name, spec, skip_to_json=False, skip_from_json=False, types=None):
    """Generate code for union types (std::variant)
    skip_to_json: if True, skip generating toJsonValue function (for duplicate signatures)
    skip_from_json: if True, skip generating fromJson specialization (for duplicate variant signatures)
    """
    prefix = doc_comment(spec.get('description', ''))
    lines = []
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
                variant_types.append(ref_type(ref))
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
        for i, json_type in enumerate(spec["type"]):
            cpp_t = cpp_type(json_type)
            if json_type == "string":
                fj.append(f"    if (val.isString()) {{")
                fj.append(f"        co_return {name}(val.toString());")
                fj.append(f"    }}")
            elif json_type == "integer":
                fj.append(f"    if (val.isDouble()) {{")
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
        ref_names = [ref_type(r) for item in items if (r := _extract_ref(item))]
        # Collect plain-type items (non-$ref items with a "type" field)
        plain_items = [item for item in items if not _extract_ref(item) and "type" in item]

        # If all items are plain types (no $refs), generate primitive-style fromJson
        if plain_items and not ref_names:
            for item in plain_items:
                json_type = item["type"]
                if json_type == "string":
                    fj.append(f"    if (val.isString())")
                    fj.append(f"        co_return {name}(val.toString());")
                elif json_type == "integer":
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
                        fj.append(f"    if (val.isArray())")
                        fj.append(f"        co_return {name}(val.toArray());")

        elif ref_names:
            dispatch_field, dispatch_map = find_dispatch_field(ref_names, types) if types else (None, None)
            if dispatch_field:
                fj.append(f"    if (!val.isObject())")
                fj.append(f'        co_return Utils::ResultError("Invalid {name}: expected object");')
                fj.append(f"    const QString dispatchValue = val.toObject().value(\"{dispatch_field}\").toString();")
                first = True
                for ref_name, const_val in dispatch_map.items():
                    kw = "if" if first else "else if"
                    first = False
                    fj.append(f"    {kw} (dispatchValue == \"{const_val}\")")
                    fj.append(f"        co_return {name}(co_await fromJson<{ref_name}>(val));")
                fj.append(f"    co_return Utils::ResultError(\"Invalid {name}: unknown {dispatch_field} \\\"\" + dispatchValue + \"\\\"\");")
                fj.append("}")  # close fromJson function
                if not skip_from_json:
                    lines.extend(use_return_if_no_co_await(fj))
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
                lines.append("        return {};")
                lines.append("    }, val);")
                lines.append("}")
                for field, ret_type in find_shared_fields(ref_names, types):
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
                    fj.append(f"    if (!val.isObject())")
                    fj.append(f'        co_return Utils::ResultError("Invalid {name}: expected object");')
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
                    for ref_name in ref_names:
                        fj.append(f"    if (val.isObject()) {{")
                        fj.append(f"        auto result = fromJson<{ref_name}>(val);")
                        fj.append(f"        if (result) co_return {name}(*result);")
                        fj.append(f"    }}")

    fj.append(f'    co_return Utils::ResultError("Invalid {name}");')
    fj.append("}")
    if not skip_from_json:
        lines.extend(use_return_if_no_co_await(fj))

    # Emit shared-field getters for presence/try-each unions too
    if "anyOf" in spec or "oneOf" in spec:
        items = spec.get("anyOf", spec.get("oneOf", []))
        ref_names = [ref_type(r) for item in items if (r := _extract_ref(item))]
        # Only for pure $ref unions (not already handled by the const-dispatch branch above)
        if ref_names and types:
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

        if not is_primitive_union:
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
                        lines.append(f"        if constexpr (std::is_same_v<T, {vt}>) {{")
                        lines.append(f"            QJsonArray arr;")
                        lines.append(f"            for (const auto &elem : v)")
                        lines.append(f"                arr.append(toJson(elem));")
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
    return False

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
                    "throw", "sizeof", "alignof", "decltype", "typeid"}
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

def is_typed_map(spec):
    """Return the C++ value type if spec is a typed string-keyed map
    (type: object, additionalProperties with a specific type, no named properties).
    Returns None otherwise.
    """
    if spec.get("type") != "object":
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
    }
    return _map.get(cpp_t)

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
    lines.extend(use_return_if_no_co_await(fj))
    lines.append("")

    # toJsonValue
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
        names = [ref_type(r) for item in any_of if (r := _extract_ref(item))]
        return names if len(names) == len(any_of) else []

    def field_anyof_ref_names(spec):
        """Return ref type name list if spec is a non-array field with anyOf/oneOf containing
        only $refs or arrays-of-$refs.  Array items are returned as QList<RefType>."""
        if spec.get("type") == "array":
            return []
        any_of = spec.get("anyOf", spec.get("oneOf", []))
        if not any_of:
            return []
        names = []
        for item in any_of:
            ref = _extract_ref(item)
            if ref:
                names.append(ref_type(ref))
            elif item.get("type") == "array" and "$ref" in item.get("items", {}):
                names.append(list_type(ref_type(item["items"]["$ref"])))
            else:
                return []  # unrecognised item shape — bail out
        return names

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
        child_full = parse_struct(short_name, child_props_n, types, child_required_n, child_desc_n, nested_children=grandchildren, children_of=children_of, original_name=child_name)
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
                                    original_name=sub_name)
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
                                    original_name=sub_name)
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
            ser_lines.append(f"    const QString str = val.toString();")
            for v in values:
                ser_lines.append(f"    if (str == \"{v}\") return {qname}::{sanitize_identifier(v)};")
            ser_lines.append(f"    return Utils::ResultError(\"Invalid {qname} value: \" + str);")
            ser_lines.append("}")
            ser_lines.append("")
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
                if variant_str in _emitted_variant_sigs:
                    # Reuse the already-emitted alias to avoid fromJson redefinition
                    union_alias = _emitted_variant_sigs[variant_str]
                else:
                    union_alias = name + prop[0].upper() + prop[1:] + "Item"
                    sub_struct_blocks.append("\n".join(_build_inline_union_code(union_alias, ref_names, types)))
                    _emitted_variant_sigs[variant_str] = union_alias
                array_item_union_names[prop] = union_alias
            else:
                fref_names = field_anyof_ref_names(spec)
                if fref_names:
                    variant_str = ", ".join(fref_names)
                    if variant_str in _emitted_variant_sigs:
                        # Reuse the already-emitted alias to avoid fromJson redefinition
                        union_alias = _emitted_variant_sigs[variant_str]
                    else:
                        union_alias = name + prop[0].upper() + prop[1:]
                        sub_struct_blocks.append("\n".join(_build_inline_union_code(union_alias, fref_names, types)))
                        _emitted_variant_sigs[variant_str] = union_alias
                    field_union_names[prop] = union_alias
                elif _map_anyof_info(spec):
                    # Map with anyOf-typed additionalProperties values
                    variant_types, val_alias = _map_anyof_info(spec)
                    val_alias = name + prop[0].upper() + prop[1:] + "Value"
                    variant_str = ", ".join(variant_types)
                    if variant_str not in _emitted_variant_sigs:
                        block = _build_map_value_variant_code(val_alias, variant_types)
                        sub_struct_blocks.append("\n".join(block))
                        _emitted_variant_sigs[variant_str] = val_alias
                    else:
                        val_alias = _emitted_variant_sigs[variant_str]
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
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};")
        elif prop in sub_struct_names:
            t = sub_struct_names[prop]
            decl_type = f"std::optional<{t}>" if is_optional else t
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        # Handle $ref (direct or allOf-wrapped)
        elif _extract_ref(spec):
            t = ref_type(_extract_ref(spec))
            if is_integer_const_namespace(t, types):
                t = "int"  # namespace of int constants, not a C++ type
            else:
                t = nested_short_names.get(t, t)  # use short name if nested
            decl_type = f"std::optional<{t}>" if is_optional else t
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        # Handle nullable $ref (anyOf with $ref + null)
        elif _extract_nullable_ref(spec):
            t = _extract_nullable_ref(spec)
            if is_integer_const_namespace(t, types):
                t = "int"
            else:
                t = nested_short_names.get(t, t)
            decl_type = f"std::optional<{t}>"  # always optional (nullable)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            item_type = ref_type(_extract_ref(spec.get("items", {})))
            item_type = nested_short_names.get(item_type, item_type)  # use short name if nested
            decl_type = list_type(item_type, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif prop in array_item_struct_names:
            t = array_item_struct_names[prop]
            decl_type = list_type(t, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif prop in array_item_union_names:
            t = array_item_union_names[prop]
            decl_type = list_type(t, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif prop in field_union_names:
            t = field_union_names[prop]
            decl_type = f"std::optional<{t}>" if is_optional else t
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif prop in map_value_union_names:
            val_alias, full_map_type = map_value_union_names[prop]
            decl_type = f"std::optional<{full_map_type}>" if is_optional else full_map_type
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif spec.get("type") == "array":
            item_type = cpp_type(spec["items"].get("type", "string"))
            decl_type = list_type(item_type, is_optional)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif is_open_map(spec):
            inner = "QMap<QString, QJsonValue>"
            decl_type = f"std::optional<{inner}>" if is_optional else inner
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            inner = f"QMap<QString, {val_type}>"
            decl_type = f"std::optional<{inner}>" if is_optional else inner
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        elif is_const_string(spec):
            pass  # const string fields are not stored in the struct
        else:
            base_t, is_nullable = _nullable_type(spec)
            t = base_t if base_t else spec.get("type", "string")
            decl_type = f"std::optional<{cpp_type(t)}>" if (is_optional or is_nullable) else cpp_type(t)
            lines.extend(pre_lines)
            lines.append(f"    {decl_type} _{sanitize_identifier(prop)};{inline_comment}")
        if not is_const_string(spec):
            prop_decl_types[prop] = decl_type

    if has_additional_props:
        lines.append(f"    QJsonObject _additionalProperties;  //!< additional properties")

    # Builder methods — named after the field (keyword-escaped), return *this by reference.
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
            list_item_type = nested_short_names.get(list_item_type, list_item_type)  # use short name if nested
            inner_type = list_type(list_item_type)
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
            list_item_type = cpp_type(spec["items"].get("type", "string"))
            inner_type = list_type(list_item_type)
        elif is_open_map(spec):
            inner_type = "QMap<QString, QJsonValue>"
        elif is_typed_map(spec) is not None:
            val_type = is_typed_map(spec)
            inner_type = f"QMap<QString, {val_type}>"
        else:
            base_t, is_nullable = _nullable_type(spec)
            inner_type = cpp_type(base_t if base_t else spec.get("type", "string"))
            if is_nullable and not is_optional:
                inner_type = f"std::optional<{inner_type}>"
        setter_type = f"std::optional<{inner_type}>" if is_optional else inner_type
        lines.append(f"    {name}& {prop_name}({_param_type(setter_type)} v) {{ _{sanitize_identifier(prop)} = v; return *this; }}")
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

    if has_additional_props:
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
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isString())")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
        elif prop in sub_struct_names:
            t = sub_struct_names[prop]
            t_fj = f"{name}::{t}"  # inline sub-structs are always nested inside this struct
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject())")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
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
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
                elif is_enum:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isString())")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
                elif is_union:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
                else:
                    fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isObject())")
                    fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
        elif _extract_nullable_ref(spec):
            t = _extract_nullable_ref(spec)
            if is_integer_const_namespace(t, types):
                fj_lines.append(f"    if (obj.contains(\"{prop}\") && !obj[\"{prop}\"].isNull())")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = obj[\"{prop}\"].toInt();")
            else:
                t_fj = f"{name}::{nested_short_names[t]}" if t in nested_short_names else t
                # Nullable ref: only parse when present and non-null
                fj_lines.append(f"    if (obj.contains(\"{prop}\") && !obj[\"{prop}\"].isNull())")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t_fj}>(obj[\"{prop}\"]);")
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            item_type = ref_type(_extract_ref(spec.get("items", {})))
            # Use qualified short-name when the item type is nested inside this struct
            item_type_fj = f"{name}::{nested_short_names[item_type]}" if item_type in nested_short_names else item_type
            is_enum = is_enum_type(item_type, types)
            is_union = is_union_type_name(item_type, types)
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(item_type_fj)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            list_{prop_name}.append(co_await fromJson<{item_type_fj}>(v));")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            result._{sanitize_identifier(prop)}.append(co_await fromJson<{item_type_fj}>(v));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif prop in array_item_struct_names:
            t = array_item_struct_names[prop]
            t_fj = f"{name}::{t}"  # inline sub-structs are always nested
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(t_fj)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            list_{prop_name}.append(co_await fromJson<{t_fj}>(v));")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            result._{sanitize_identifier(prop)}.append(co_await fromJson<{t_fj}>(v));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif prop in field_union_names:
            t = field_union_names[prop]
            fj_lines.append(f"    if (obj.contains(\"{prop}\"))")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = co_await fromJson<{t}>(obj[\"{prop}\"]);")
        elif prop in array_item_union_names:
            t = array_item_union_names[prop]
            fj_lines.append(f"    if (obj.contains(\"{prop}\") && obj[\"{prop}\"].isArray()) {{")
            fj_lines.append(f"        const QJsonArray arr = obj[\"{prop}\"].toArray();")
            if is_optional:
                fj_lines.append(f"        {list_type(t)} list_{prop_name};")
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            list_{prop_name}.append(co_await fromJson<{t}>(v));")
                fj_lines.append(f"        }}")
                fj_lines.append(f"        result._{sanitize_identifier(prop)} = list_{prop_name};")
            else:
                fj_lines.append(f"        for (const QJsonValue &v : arr) {{")
                fj_lines.append(f"            result._{sanitize_identifier(prop)}.append(co_await fromJson<{t}>(v));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"    }}")
        elif spec.get("type") == "array":
            item_type = cpp_type(spec["items"].get("type", "string"))
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
            fj_lines.append(f"            map_{prop_name}.insert(it.key(), co_await fromJson<{val_alias}>(it.value()));")
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
            if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                extract = _json_extract_expr(val_type, "it.value()") or "it.value().toString()"
                fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it)")
                fj_lines.append(f"            map_{prop_name}.insert(it.key(), {extract});")
            else:
                fj_lines.append(f"        for (auto it = mapObj_{prop_name}.constBegin(); it != mapObj_{prop_name}.constEnd(); ++it) {{")
                fj_lines.append(f"            map_{prop_name}.insert(it.key(), co_await fromJson<{val_type}>(it.value()));")
                fj_lines.append(f"        }}")
            fj_lines.append(f"        result._{sanitize_identifier(prop)} = map_{prop_name};")
            fj_lines.append(f"    }}")
        elif is_const_string(spec):
            const_value = spec["const"]
            fj_lines.append(f"    if (obj.value(\"{prop}\").toString() != \"{const_value}\")")
            fj_lines.append(f"        co_return Utils::ResultError(\"Field '{prop}' must be '{const_value}', got: \" + obj.value(\"{prop}\").toString());")
        else:
            base_t, is_nullable = _nullable_type(spec)
            t = base_t if base_t else spec.get("type", "string")
            ct = cpp_type(t)
            _scalar_expr = _json_extract_expr(ct, f'obj.value("{prop}")')
            if is_optional:
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
                fj_lines.append(f"{indent}}}")
            elif _scalar_expr:
                fj_lines.append(f"{indent}result._{sanitize_identifier(prop)} = {_scalar_expr};")
            else:
                fj_lines.append(f"{indent}// Unknown property type: {ct}")
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
    lines.extend(use_return_if_no_co_await(fj_lines))
    lines.append("")
    # toJson function — two-pass:
    # Pass 1: collect required single-value fields for an initializer list.
    # Pass 2: emit optional and array fields as post-init insert calls.
    lines.append(f"inline QJsonObject toJson(const {name} &data) {{")
    init_entries = []  # (key, value_expr) for the initializer list
    post_lines   = []  # lines emitted after the QJsonObject declaration

    for prop, spec in props.items():
        prop_name = sanitize_identifier(prop)
        is_optional = prop not in required
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
                # Integer const namespace — serialize as plain int
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
                    # Simple primitive aliases (e.g. QString) convert directly to QJsonValue
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
                # Integer const namespace — serialize as plain int
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                if prop in required:
                    post_lines.append(f"    else")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
            else:
                is_enum = is_enum_type(t, types)
                is_union = is_union_type_name(t, types)
                is_simple = is_simple_type_alias(t, types)
                val_fn = "toJsonValue" if (is_enum or is_union) else ("" if is_simple else "toJson")
                # Always optional (nullable) — emit when has_value
                if val_fn:
                    post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                    post_lines.append(f"        obj.insert(\"{prop}\", {val_fn}(*data._{sanitize_identifier(prop)}));")
                else:
                    post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                    post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                if prop in required:
                    # Required but nullable: emit null when empty
                    post_lines.append(f"    else")
                    post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
        elif spec.get("type") == "array" and _extract_ref(spec.get("items", {})):
            item_type = ref_type(_extract_ref(spec.get("items", {})))
            is_enum = is_enum_type(item_type, types)
            is_union = is_union_type_name(item_type, types)
            arr_fn = "toJsonValue" if (is_enum or is_union) else "toJson"
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonArray arr_{prop_name};")
                post_lines.append(f"        for (const auto &v : *data._{sanitize_identifier(prop)}) arr_{prop_name}.append({arr_fn}(v));")
                post_lines.append(f"        obj.insert(\"{prop}\", arr_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonArray arr_{prop_name};")
                post_lines.append(f"    for (const auto &v : data._{sanitize_identifier(prop)}) arr_{prop_name}.append({arr_fn}(v));")
                post_lines.append(f"    obj.insert(\"{prop}\", arr_{prop_name});")
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
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value()) {{")
                post_lines.append(f"        QJsonObject map_{prop_name};")
                if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                    post_lines.append(f"        for (auto it = data._{sanitize_identifier(prop)}->constBegin(); it != data._{sanitize_identifier(prop)}->constEnd(); ++it)")
                    post_lines.append(f"            map_{prop_name}.insert(it.key(), QJsonValue(it.value()));")
                else:
                    post_lines.append(f"        for (auto it = data._{sanitize_identifier(prop)}->constBegin(); it != data._{sanitize_identifier(prop)}->constEnd(); ++it)")
                    post_lines.append(f"            map_{prop_name}.insert(it.key(), toJsonValue(it.value()));")
                post_lines.append(f"        obj.insert(\"{prop}\", map_{prop_name});")
                post_lines.append(f"    }}")
            else:
                post_lines.append(f"    QJsonObject map_{prop_name};")
                if val_type in ("QString", "QJsonObject", "int", "double", "bool"):
                    post_lines.append(f"    for (auto it = data._{sanitize_identifier(prop)}.constBegin(); it != data._{sanitize_identifier(prop)}.constEnd(); ++it)")
                    post_lines.append(f"        map_{prop_name}.insert(it.key(), QJsonValue(it.value()));")
                else:
                    post_lines.append(f"    for (auto it = data._{sanitize_identifier(prop)}.constBegin(); it != data._{sanitize_identifier(prop)}.constEnd(); ++it)")
                    post_lines.append(f"        map_{prop_name}.insert(it.key(), toJsonValue(it.value()));")
                post_lines.append(f"    obj.insert(\"{prop}\", map_{prop_name});")
        elif is_const_string(spec):
            const_value = spec["const"]
            init_entries.append((prop, f"QString(\"{const_value}\")"))
        else:
            _, is_nullable = _nullable_type(spec)
            if is_optional:
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
            elif is_nullable:
                # Required but nullable: always emit the key; use QJsonValue::Null when empty
                post_lines.append(f"    if (data._{sanitize_identifier(prop)}.has_value())")
                post_lines.append(f"        obj.insert(\"{prop}\", *data._{sanitize_identifier(prop)});")
                post_lines.append(f"    else")
                post_lines.append(f"        obj.insert(\"{prop}\", QJsonValue::Null);")
            else:
                init_entries.append((prop, f"data._{sanitize_identifier(prop)}"))

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
    obj_names = [rn for rn in ref_names if not rn.startswith("QList<")]

    fj = [
        "template<>",
        f"inline Utils::Result<{union_alias}> fromJson<{union_alias}>(const QJsonValue &val) {{",
    ]

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
        fj.append(f'    co_return Utils::ResultError("Invalid {union_alias}");')
    fj.append("}")
    union_lines.extend(use_return_if_no_co_await(fj))

    # toJsonValue — custom if list types present, otherwise default
    if list_types:
        union_lines.append("")
        union_lines.append(f"inline QJsonValue toJsonValue(const {union_alias} &val) {{")
        union_lines.append("    return std::visit([](const auto &v) -> QJsonValue {")
        union_lines.append("        using T = std::decay_t<decltype(v)>;")
        first = True
        for list_t, inner_t in list_types:
            kw = "if" if first else "} else if"
            first = False
            union_lines.append(f"        {kw} constexpr (std::is_same_v<T, {list_t}>) {{")
            union_lines.append(f"            QJsonArray arr;")
            union_lines.append(f"            for (const auto &item : v) arr.append(toJson(item));")
            union_lines.append(f"            return arr;")
        union_lines.append("        } else if constexpr (std::is_same_v<T, QJsonObject>) {")
        union_lines.append("            return v;")
        union_lines.append("        } else {")
        union_lines.append("            return toJson(v);")
        union_lines.append("        }")
        union_lines.append("    }, val);")
        union_lines.append("}")
        union_lines.append("")
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

    def _remove_inline(func_lines):
        """Remove 'inline' keyword from function definition lines."""
        result = []
        for fl in func_lines:
            result.append(re.sub(r'\binline\s+', '', fl))
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
            cpp_lines.extend(_remove_inline(func_lines))
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
                    cpp_lines.extend(_remove_inline(func_lines))
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
                    cpp_lines.extend(_remove_inline(func_lines))
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
            cpp_lines.extend(_remove_inline(func_lines))
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
                cpp_lines.extend(_remove_inline(lines[j:end + 1]))
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
    args = parser.parse_args()
    global _emit_comments, _emitted_variant_sigs
    _emit_comments = not args.no_comments
    _emitted_variant_sigs = {}  # reset per run
    schema_path = Path(args.schema)
    output_path = Path(args.output)
    namespace = args.namespace
    with open(schema_path) as f:
        schema = json.load(f)
    # Support 'definitions', 'components.schemas', or '$defs'
    if "definitions" in schema:
        types = schema["definitions"]
    elif "components" in schema and "schemas" in schema["components"]:
        types = schema["components"]["schemas"]
    elif "$defs" in schema:
        types = schema["$defs"]
    else:
        print("Schema format not recognized. Needs 'definitions', 'components.schemas', or '$defs'.")
        sys.exit(1)
    # --- BEGIN FULL REFACTOR: Robust dependency-graph-based emission ---
    export_header = args.export_header if args.cpp_output else None
    code = [make_header(namespace, export_header=export_header)]
    order = topo_sort_types(types)

    # Build dependency graph for all types (structs, enums, aliases, unions)
    emitted = set()
    variant_signatures = {}
    alias_fromjson_emitted = set()  # track underlying types that already have fromJson

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
                has_additional_props = spec.get("additionalProperties") in ({}, True)
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
        with open(output_path, "w") as f:
            f.write(h_output)

        cpp_output = "\n".join(cpp_blocks)
        cpp_output = re.sub(r'\n{3,}', '\n\n', cpp_output)
        with open(cpp_output_path, "w") as f:
            f.write(cpp_output)

        print(f"Generated C++ header at {output_path}")
        print(f"Generated C++ implementation at {cpp_output_path}")
    else:
        output = "\n".join(code)
        output = re.sub(r'\n{3,}', '\n\n', output)
        with open(output_path, "w") as f:
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
        _, signature = parse_union(name, spec, skip_to_json=True, skip_from_json=True, types=types)
        if signature in variant_signatures:
            result, _ = parse_union(name, spec, skip_to_json=True, skip_from_json=True, types=types)
            code.append(result)
        else:
            result, _ = parse_union(name, spec, skip_to_json=False, skip_from_json=False, types=types)
            code.append(result)
            variant_signatures[signature] = name
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
            code.append("\n".join(alias_lines) + "\n")
        else:
            code.append(f"// Skipped unknown type alias: {name}\n")
    emitted.add(name)

if __name__ == "__main__":
    main()
