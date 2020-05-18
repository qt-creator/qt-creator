/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <utf8string.h>
#include <utf8stringvector.h>

#include <QVariant>

namespace ClangBackEnd {

class ToolTipInfo
{
public:
    enum QdocCategory : quint8 {
        Unknown,
        ClassOrNamespace,
        Enum,
        Typedef,
        Macro,
        Brief,
        Function,
    };

public:
    ToolTipInfo() = default;
    ToolTipInfo(const Utf8String &text) : text(text) {}

    friend QDataStream &operator<<(QDataStream &out, const ToolTipInfo &message)
    {
        out << message.text;
        out << message.briefComment;
        out << message.qdocIdCandidates;
        out << message.qdocMark;
        out << static_cast<quint8>(message.qdocCategory);
        out << message.value;
        out << message.sizeInBytes;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ToolTipInfo &message)
    {
        quint8 qdocCategory;

        in >> message.text;
        in >> message.briefComment;
        in >> message.qdocIdCandidates;
        in >> message.qdocMark;
        in >> qdocCategory;
        in >> message.value;
        in >> message.sizeInBytes;

        message.qdocCategory = static_cast<QdocCategory>(qdocCategory);

        return in;
    }

    friend bool operator==(const ToolTipInfo &first, const ToolTipInfo &second)
    {
        return first.text == second.text
            && first.briefComment == second.briefComment
            && first.qdocIdCandidates == second.qdocIdCandidates
            && first.qdocMark == second.qdocMark
            && first.qdocCategory == second.qdocCategory
            && first.value == second.value
            && first.sizeInBytes == second.sizeInBytes;
    }

public:
    Utf8String text;
    Utf8String briefComment;

    Utf8StringVector qdocIdCandidates;
    Utf8String qdocMark;
    QdocCategory qdocCategory = Unknown;
    QVariant value;

    // For class definition and for class fields.
    Utf8String sizeInBytes;
};

QDebug operator<<(QDebug debug, const ToolTipInfo &message);
std::ostream &operator<<(std::ostream &os, const ToolTipInfo &message);

const char *qdocCategoryToString(ToolTipInfo::QdocCategory category);

} // namespace ClangBackEnd
