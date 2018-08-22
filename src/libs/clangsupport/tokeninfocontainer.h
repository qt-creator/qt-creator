/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "clangsupport_global.h"

#include "sourcerangecontainer.h"

#include <sqlite/utf8string.h>

#include <QDataStream>

#include <bitset>

namespace ClangBackEnd {

inline QDataStream &operator<<(QDataStream &out, HighlightingType highlightingType);
inline QDataStream &operator<<(QDataStream &out, HighlightingTypes highlightingTypes);
inline QDataStream &operator>>(QDataStream &in, HighlightingType &highlightingType);
inline QDataStream &operator>>(QDataStream &in, HighlightingTypes &highlightingTypes);
inline bool operator==(const MixinHighlightingTypes &first, const MixinHighlightingTypes &second);
inline bool operator==(const HighlightingTypes &first, const HighlightingTypes &second);

using ByteSizeBitset = std::bitset<8>;

inline QDataStream &operator<<(QDataStream &out, ByteSizeBitset bits);
inline QDataStream &operator>>(QDataStream &in, ByteSizeBitset &bits);

struct ExtraInfo
{
    ExtraInfo()
        : identifier(false)
        , includeDirectivePath(false)
        , declaration(false)
        , definition(false)
        , signal(false)
        , slot(false)
    {
    }
    ExtraInfo(Utf8String token, Utf8String typeSpelling,
              Utf8String semanticParentTypeSpelling, SourceRangeContainer cursorRange,
              int lexicalParentIndex, AccessSpecifier accessSpecifier, StorageClass storageClass,
              bool isIdentifier, bool isInclusion, bool isDeclaration, bool isDefinition,
              bool isSignal, bool isSlot)
        : token(token)
        , typeSpelling(typeSpelling)
        , semanticParentTypeSpelling(semanticParentTypeSpelling)
        , cursorRange(cursorRange)
        , lexicalParentIndex(lexicalParentIndex)
        , accessSpecifier(accessSpecifier)
        , storageClass(storageClass)
        , identifier(isIdentifier)
        , includeDirectivePath(isInclusion)
        , declaration(isDeclaration)
        , definition(isDefinition)
        , signal(isSignal)
        , slot(isSlot)
    {
    }
    Utf8String token;
    Utf8String typeSpelling;
    Utf8String semanticParentTypeSpelling;
    SourceRangeContainer cursorRange;
    int lexicalParentIndex = -1;
    AccessSpecifier accessSpecifier = AccessSpecifier::Invalid;
    StorageClass storageClass = StorageClass::Invalid;
    bool identifier : 1;
    bool includeDirectivePath : 1;
    bool declaration : 1;
    bool definition : 1;
    bool signal : 1;
    bool slot : 1;
};

inline QDataStream &operator<<(QDataStream &out, const ExtraInfo &extraInfo);
inline QDataStream &operator>>(QDataStream &in, ExtraInfo &extraInfo);
inline bool operator==(const ExtraInfo &first, const ExtraInfo &second);

class TokenInfoContainer
{
public:
    TokenInfoContainer() = default;
    TokenInfoContainer(uint line, uint column, uint length, HighlightingTypes types)
        : line(line)
        , column(column)
        , length(length)
        , types(types)
    {
    }

    TokenInfoContainer(uint line, uint column, uint length, HighlightingTypes types,
                       const ExtraInfo &extraInfo)
        : line(line)
        , column(column)
        , length(length)
        , types(types)
        , extraInfo(extraInfo)
        , noExtraInfo(false)
    {
    }

    bool isInvalid() const
    {
        return line == 0 && column == 0 && length == 0;
    }

    bool isGlobalDeclaration() const
    {
        if (types.mixinHighlightingTypes.contains(
                    ClangBackEnd::HighlightingType::TemplateTypeParameter)
                || types.mixinHighlightingTypes.contains(
                    ClangBackEnd::HighlightingType::TemplateTemplateParameter)) {
            return false;
        }

        return extraInfo.declaration
                && types.mainHighlightingType != HighlightingType::LocalVariable
                && (types.mixinHighlightingTypes.contains(HighlightingType::Operator)
                    == extraInfo.token.startsWith("operator"));
    }

    friend QDataStream &operator<<(QDataStream &out, const TokenInfoContainer &container)
    {
        out << container.line;
        out << container.column;
        out << container.length;
        out << container.types;
        out << container.noExtraInfo;

        if (container.noExtraInfo)
            return out;

        out << container.extraInfo;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TokenInfoContainer &container)
    {
        in >> container.line;
        in >> container.column;
        in >> container.length;
        in >> container.types;
        in >> container.noExtraInfo;

        if (container.noExtraInfo)
            return in;

        in >> container.extraInfo;

        return in;
    }

    friend bool operator==(const TokenInfoContainer &first, const TokenInfoContainer &second)
    {
        return first.line == second.line
            && first.column == second.column
            && first.length == second.length
            && first.types == second.types
            && first.noExtraInfo == second.noExtraInfo
            && first.extraInfo == second.extraInfo;
    }

    uint line = 0;
    uint column = 0;
    uint length = 0;
    HighlightingTypes types;
    ExtraInfo extraInfo;
    bool noExtraInfo = true;
};

inline QDataStream &operator<<(QDataStream &out, const ExtraInfo &extraInfo)
{
    out << extraInfo.token;
    out << extraInfo.typeSpelling;
    out << extraInfo.semanticParentTypeSpelling;
    out << extraInfo.cursorRange;
    out << extraInfo.lexicalParentIndex;
    out << static_cast<uint>(extraInfo.accessSpecifier);
    out << static_cast<uint>(extraInfo.storageClass);
    out << extraInfo.identifier;
    out << extraInfo.includeDirectivePath;
    out << extraInfo.declaration;
    out << extraInfo.definition;
    out << extraInfo.signal;
    out << extraInfo.slot;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, ExtraInfo &extraInfo)
{
    in >> extraInfo.token;
    in >> extraInfo.typeSpelling;
    in >> extraInfo.semanticParentTypeSpelling;
    in >> extraInfo.cursorRange;
    in >> extraInfo.lexicalParentIndex;

    uint accessSpecifier;
    uint storageClass;
    bool isIdentifier;
    bool isInclusion;
    bool isDeclaration;
    bool isDefinition;
    bool isSignal;
    bool isSlot;

    in >> accessSpecifier;
    in >> storageClass;
    in >> isIdentifier;
    in >> isInclusion;
    in >> isDeclaration;
    in >> isDefinition;
    in >> isSignal;
    in >> isSlot;

    extraInfo.accessSpecifier = static_cast<AccessSpecifier>(accessSpecifier);
    extraInfo.storageClass = static_cast<StorageClass>(storageClass);
    extraInfo.identifier = isIdentifier;
    extraInfo.includeDirectivePath = isInclusion;
    extraInfo.declaration = isDeclaration;
    extraInfo.definition = isDefinition;
    extraInfo.signal = isSignal;
    extraInfo.slot = isSlot;
    return in;
}

inline bool operator==(const ExtraInfo &first, const ExtraInfo &second)
{
    return first.token == second.token
            && first.typeSpelling == second.typeSpelling
            && first.semanticParentTypeSpelling == second.semanticParentTypeSpelling
            && first.cursorRange == second.cursorRange
            && first.lexicalParentIndex == second.lexicalParentIndex
            && first.accessSpecifier == second.accessSpecifier
            && first.storageClass == second.storageClass
            && first.identifier == second.identifier
            && first.includeDirectivePath == second.includeDirectivePath
            && first.declaration == second.declaration
            && first.definition == second.definition
            && first.signal == second.signal
            && first.slot == second.slot;
}

inline QDataStream &operator<<(QDataStream &out, HighlightingType highlightingType)
{
    out << static_cast<quint8>(highlightingType);

    return out;
}

inline QDataStream &operator<<(QDataStream &out, HighlightingTypes highlightingTypes)
{
    out << highlightingTypes.mainHighlightingType;

    out << highlightingTypes.mixinHighlightingTypes.size();

    for (HighlightingType type : highlightingTypes.mixinHighlightingTypes)
        out << type;

    return out;
}


inline QDataStream &operator>>(QDataStream &in, HighlightingType &highlightingType)
{
    quint8 highlightingTypeInt;

    in >> highlightingTypeInt;

    highlightingType = static_cast<HighlightingType>(highlightingTypeInt);

    return in;
}

inline QDataStream &operator>>(QDataStream &in, HighlightingTypes &highlightingTypes)
{
    in >> highlightingTypes.mainHighlightingType ;

    quint8 size;
    in >> size;

    for (int counter = 0; counter < size; ++counter) {
        HighlightingType type;
        in >> type;
        highlightingTypes.mixinHighlightingTypes.push_back(type);
    }

    return in;
}

inline bool operator==(const MixinHighlightingTypes &first, const MixinHighlightingTypes &second)
{
    return first.size() == second.size()
        && std::equal(first.begin(), first.end(), second.begin());
}

inline bool operator==(const HighlightingTypes &first, const HighlightingTypes &second)
{
    return first.mainHighlightingType == second.mainHighlightingType
        && first.mixinHighlightingTypes == second.mixinHighlightingTypes;
}

inline QDataStream &operator<<(QDataStream &out, ByteSizeBitset bits)
{
    // Narrow unsigned long to uint8_t
    out << static_cast<uint8_t>(bits.to_ulong());
    return out;
}

inline QDataStream &operator>>(QDataStream &in, ByteSizeBitset &bits)
{
    uint8_t byteValue;
    in >> byteValue;
    bits = ByteSizeBitset(byteValue);
    return in;
}

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const TokenInfoContainer &container);

} // namespace ClangBackEnd
