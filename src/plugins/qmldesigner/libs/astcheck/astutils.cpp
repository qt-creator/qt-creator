// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "astutils.h"

#include <utils/algorithm.h>
#include <utils/array.h>
#include <utils/expected.h>
#include <utils/ranges.h>

#include <array>

#include <QRegularExpression>

namespace QmlDesigner::AstUtils {

constexpr auto qmlKeywords = Utils::to_sorted_array<std::u16string_view>(u"abstract",
                                                                         u"alias",
                                                                         u"as",
                                                                         u"boolean",
                                                                         u"break",
                                                                         u"byte",
                                                                         u"case",
                                                                         u"catch",
                                                                         u"char",
                                                                         u"class",
                                                                         u"component",
                                                                         u"console",
                                                                         u"const",
                                                                         u"continue",
                                                                         u"debugger",
                                                                         u"default",
                                                                         u"delete",
                                                                         u"do",
                                                                         u"double",
                                                                         u"else",
                                                                         u"enum",
                                                                         u"export",
                                                                         u"extends",
                                                                         u"false",
                                                                         u"final",
                                                                         u"finally",
                                                                         u"float",
                                                                         u"for",
                                                                         u"from",
                                                                         u"function",
                                                                         u"get",
                                                                         u"goto",
                                                                         u"if",
                                                                         u"implements",
                                                                         u"import",
                                                                         u"in",
                                                                         u"instanceof",
                                                                         u"int",
                                                                         u"interface",
                                                                         u"let",
                                                                         u"long",
                                                                         u"native",
                                                                         u"new",
                                                                         u"null",
                                                                         u"of",
                                                                         u"on",
                                                                         u"package",
                                                                         u"pragma",
                                                                         u"print",
                                                                         u"private",
                                                                         u"property",
                                                                         u"protected",
                                                                         u"public",
                                                                         u"readonly",
                                                                         u"required",
                                                                         u"return",
                                                                         u"set",
                                                                         u"short",
                                                                         u"signal",
                                                                         u"static",
                                                                         u"super",
                                                                         u"switch",
                                                                         u"synchronized",
                                                                         u"this",
                                                                         u"throw",
                                                                         u"throws",
                                                                         u"transient",
                                                                         u"true",
                                                                         u"try",
                                                                         u"typeof",
                                                                         u"var",
                                                                         u"void",
                                                                         u"volatile",
                                                                         u"while",
                                                                         u"with",
                                                                         u"yield");

constexpr auto qmlDiscouragedIds = Utils::to_sorted_array<std::u16string_view>(u"action",
                                                                               u"anchors",
                                                                               u"baseState",
                                                                               u"border",
                                                                               u"bottom",
                                                                               u"clip",
                                                                               u"data",
                                                                               u"enabled",
                                                                               u"flow",
                                                                               u"focus",
                                                                               u"font",
                                                                               u"height",
                                                                               u"id",
                                                                               u"item",
                                                                               u"layer",
                                                                               u"left",
                                                                               u"margin",
                                                                               u"opacity",
                                                                               u"padding",
                                                                               u"parent",
                                                                               u"right",
                                                                               u"scale",
                                                                               u"shaderInfo",
                                                                               u"source",
                                                                               u"sprite",
                                                                               u"spriteSequence",
                                                                               u"state",
                                                                               u"stateGroup",
                                                                               u"text",
                                                                               u"texture",
                                                                               u"time",
                                                                               u"top",
                                                                               u"visible",
                                                                               u"width",
                                                                               u"x",
                                                                               u"y",
                                                                               u"z");

constexpr auto qmlBuiltinTypes = Utils::to_sorted_array<std::u16string_view>(u"bool",
                                                                             u"color",
                                                                             u"date",
                                                                             u"double",
                                                                             u"enumeration",
                                                                             u"font",
                                                                             u"int",
                                                                             u"list",
                                                                             u"matrix4x4",
                                                                             u"point",
                                                                             u"quaternion",
                                                                             u"real",
                                                                             u"rect",
                                                                             u"size",
                                                                             u"string",
                                                                             u"url",
                                                                             u"var",
                                                                             u"variant",
                                                                             u"vector",
                                                                             u"vector2d",
                                                                             u"vector3d",
                                                                             u"vector4d");

constexpr auto createBannedQmlIds()
{
    std::array<std::u16string_view, qmlKeywords.size() + qmlDiscouragedIds.size() + qmlBuiltinTypes.size()> ids;

    auto idsEnd = std::ranges::copy(qmlKeywords, ids.begin()).out;
    idsEnd = std::ranges::copy(qmlDiscouragedIds, idsEnd).out;
    std::ranges::copy(qmlBuiltinTypes, idsEnd);

    std::ranges::sort(ids);

    return ids;
}

constexpr std::u16string_view toStdStringView(QStringView view)
{
    return {view.utf16(), Utils::usize(view)};
}

bool isQmlKeyword(QStringView id)
{
    return std::ranges::binary_search(qmlKeywords, toStdStringView(id));
}

bool isDiscouragedQmlId(QStringView id)
{
    return std::ranges::binary_search(qmlDiscouragedIds, toStdStringView(id));
}

bool isQmlBuiltinType(QStringView id)
{
    return std::ranges::binary_search(qmlBuiltinTypes, toStdStringView(id));
}

bool isBannedQmlId(QStringView id)
{
    static constexpr auto invalidIds = createBannedQmlIds();
    return std::ranges::binary_search(invalidIds, toStdStringView(id));
}

} // namespace QmlDesigner::AstUtils
