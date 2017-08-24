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

#include "filecontainer.h"

namespace ClangBackEnd {

class TranslationUnitDoesNotExistMessage
{
public:
    TranslationUnitDoesNotExistMessage() = default;
    TranslationUnitDoesNotExistMessage(const FileContainer &fileContainer)
        : m_fileContainer(fileContainer)
    {
    }

    TranslationUnitDoesNotExistMessage(const Utf8String &filePath, const Utf8String &projectPartId)
        : m_fileContainer(filePath, projectPartId)
    {
    }

    const FileContainer &fileContainer() const
    {
        return m_fileContainer;
    }

    const Utf8String &filePath() const
    {
        return m_fileContainer.filePath();
    }

    const Utf8String &projectPartId() const
    {
        return m_fileContainer.projectPartId();
    }

    friend QDataStream &operator<<(QDataStream &out, const TranslationUnitDoesNotExistMessage &message)
    {
        out << message.m_fileContainer;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TranslationUnitDoesNotExistMessage &message)
    {
        in >> message.m_fileContainer;

        return in;
    }

    friend bool operator==(const TranslationUnitDoesNotExistMessage &first, const TranslationUnitDoesNotExistMessage &second)
    {
        return first.m_fileContainer == second.m_fileContainer;
    }

private:
    FileContainer m_fileContainer;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const TranslationUnitDoesNotExistMessage &message);
std::ostream &operator<<(std::ostream &os, const TranslationUnitDoesNotExistMessage &message);

DECLARE_MESSAGE(TranslationUnitDoesNotExistMessage)
} // namespace ClangBackEnd
