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
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false,
                  quint32 documentRevision = 0,
                  const Utf8String &textCodecName = Utf8String())
        : filePath(filePath),
          unsavedFileContent(unsavedFileContent),
          textCodecName(textCodecName),
          documentRevision(documentRevision),
          hasUnsavedFileContent(hasUnsavedFileContent)
    {
    }

    FileContainer(const Utf8String &filePath,
                  const Utf8StringVector &compilationArguments,
                  const Utf8StringVector &headerPaths,
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false,
                  quint32 documentRevision = 0)
        : filePath(filePath),
          compilationArguments(compilationArguments),
          headerPaths(headerPaths),
          unsavedFileContent(unsavedFileContent),
          documentRevision(documentRevision),
          hasUnsavedFileContent(hasUnsavedFileContent)
    {
    }

    FileContainer(const Utf8String &filePath,
                  const Utf8StringVector &compilationArguments,
                  const Utf8StringVector &headerPaths,
                  quint32 documentRevision)
        : filePath(filePath),
          compilationArguments(compilationArguments),
          headerPaths(headerPaths),
          documentRevision(documentRevision),
          hasUnsavedFileContent(false)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const FileContainer &container)
    {
        out << container.filePath;
        out << container.compilationArguments;
        out << container.headerPaths;
        out << container.unsavedFileContent;
        out << container.textCodecName;
        out << container.documentRevision;
        out << container.hasUnsavedFileContent;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FileContainer &container)
    {
        in >> container.filePath;
        in >> container.compilationArguments;
        in >> container.headerPaths;
        in >> container.unsavedFileContent;
        in >> container.textCodecName;
        in >> container.documentRevision;
        in >> container.hasUnsavedFileContent;

        return in;
    }

    friend bool operator==(const FileContainer &first, const FileContainer &second)
    {
        return first.filePath == second.filePath;
    }

public:
    Utf8String filePath;
    Utf8StringVector compilationArguments;
    Utf8StringVector headerPaths;
    Utf8String unsavedFileContent;
    Utf8String textCodecName;
    quint32 documentRevision = 0;
    bool hasUnsavedFileContent = false;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);

} // namespace ClangBackEnd
