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
    ToolTipInfo(const Utf8String &text) : m_text(text) {}

    const Utf8String &text() const { return m_text; }
    void setText(const Utf8String &text) { m_text = text; }

    const Utf8String &briefComment() const { return m_briefComment; }
    void setBriefComment(const Utf8String &briefComment) { m_briefComment = briefComment; }

    const Utf8StringVector &qdocIdCandidates() const { return m_qdocIdCandidates; }
    void setQdocIdCandidates(const Utf8StringVector &qdocIdCandidates)
    { m_qdocIdCandidates = qdocIdCandidates; }

    const Utf8String &qdocMark() const { return m_qdocMark; }
    void setQdocMark(const Utf8String &qdocMark) { m_qdocMark = qdocMark; }

    const QdocCategory &qdocCategory() const { return m_qdocCategory; }
    void setQdocCategory(const QdocCategory &qdocCategory) { m_qdocCategory = qdocCategory; }

    const Utf8String &sizeInBytes() const { return m_sizeInBytes; }
    void setSizeInBytes(const Utf8String &sizeInBytes) { m_sizeInBytes = sizeInBytes; }

    friend QDataStream &operator<<(QDataStream &out, const ToolTipInfo &message)
    {
        out << message.m_text;
        out << message.m_briefComment;
        out << message.m_qdocIdCandidates;
        out << message.m_qdocMark;
        out << static_cast<quint8>(message.m_qdocCategory);
        out << message.m_sizeInBytes;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ToolTipInfo &message)
    {
        quint8 qdocCategory;

        in >> message.m_text;
        in >> message.m_briefComment;
        in >> message.m_qdocIdCandidates;
        in >> message.m_qdocMark;
        in >> qdocCategory;
        in >> message.m_sizeInBytes;

        message.m_qdocCategory = static_cast<QdocCategory>(qdocCategory);

        return in;
    }

    friend bool operator==(const ToolTipInfo &first, const ToolTipInfo &second)
    {
        return first.m_text == second.m_text
            && first.m_briefComment == second.m_briefComment
            && first.m_qdocIdCandidates == second.m_qdocIdCandidates
            && first.m_qdocMark == second.m_qdocMark
            && first.m_qdocCategory == second.m_qdocCategory
            && first.m_sizeInBytes == second.m_sizeInBytes;
    }

    friend QDebug operator<<(QDebug debug, const ToolTipInfo &message);
    friend std::ostream &operator<<(std::ostream &os, const ToolTipInfo &message);

private:
    Utf8String m_text;
    Utf8String m_briefComment;

    Utf8StringVector m_qdocIdCandidates;
    Utf8String m_qdocMark;
    QdocCategory m_qdocCategory = Unknown;

    // For class definition and for class fields.
    Utf8String m_sizeInBytes;
};

const char *qdocCategoryToString(ToolTipInfo::QdocCategory category);

} // namespace ClangBackEnd
