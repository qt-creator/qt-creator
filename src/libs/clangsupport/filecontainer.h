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

#include <utf8string.h>
#include <utf8stringvector.h>

#include <QDataStream>

namespace ClangBackEnd {

class FileContainer
{
public:
    FileContainer() = default;
    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false,
                  quint32 documentRevision = 0)
        : m_filePath(filePath),
          m_projectPartId(projectPartId),
          m_unsavedFileContent(unsavedFileContent),
          m_documentRevision(documentRevision),
          m_hasUnsavedFileContent(hasUnsavedFileContent)
    {
    }

    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8StringVector &fileArguments,
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false,
                  quint32 documentRevision = 0)
        : m_filePath(filePath),
          m_projectPartId(projectPartId),
          m_fileArguments(fileArguments),
          m_unsavedFileContent(unsavedFileContent),
          m_documentRevision(documentRevision),
          m_hasUnsavedFileContent(hasUnsavedFileContent)
    {
    }

    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8StringVector &fileArguments,
                  quint32 documentRevision)
        : m_filePath(filePath),
          m_projectPartId(projectPartId),
          m_fileArguments(fileArguments),
          m_documentRevision(documentRevision),
          m_hasUnsavedFileContent(false)
    {
    }

    const Utf8String &filePath() const
    {
        return m_filePath;
    }

    const Utf8String &projectPartId() const
    {
        return m_projectPartId;
    }

    const Utf8StringVector &fileArguments() const
    {
        return m_fileArguments;
    }

    const Utf8String &unsavedFileContent() const
    {
        return m_unsavedFileContent;
    }

    bool hasUnsavedFileContent() const
    {
        return m_hasUnsavedFileContent;
    }

    quint32 documentRevision() const
    {
        return m_documentRevision;
    }

    friend QDataStream &operator<<(QDataStream &out, const FileContainer &container)
    {
        out << container.m_filePath;
        out << container.m_projectPartId;
        out << container.m_fileArguments;
        out << container.m_unsavedFileContent;
        out << container.m_documentRevision;
        out << container.m_hasUnsavedFileContent;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FileContainer &container)
    {
        in >> container.m_filePath;
        in >> container.m_projectPartId;
        in >> container.m_fileArguments;
        in >> container.m_unsavedFileContent;
        in >> container.m_documentRevision;
        in >> container.m_hasUnsavedFileContent;

        return in;
    }

    friend bool operator==(const FileContainer &first, const FileContainer &second)
    {
        return first.m_filePath == second.m_filePath && first.m_projectPartId == second.m_projectPartId;
    }

private:
    Utf8String m_filePath;
    Utf8String m_projectPartId;
    Utf8StringVector m_fileArguments;
    Utf8String m_unsavedFileContent;
    quint32 m_documentRevision = 0;
    bool m_hasUnsavedFileContent = false;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);
std::ostream &operator<<(std::ostream &os, const FileContainer &container);

} // namespace ClangBackEnd
