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

#include "clangbackendipc_global.h"

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
        : filePath_(filePath),
          projectPartId_(projectPartId),
          unsavedFileContent_(unsavedFileContent),
          documentRevision_(documentRevision),
          hasUnsavedFileContent_(hasUnsavedFileContent)
    {
    }

    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8StringVector &fileArguments,
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false,
                  quint32 documentRevision = 0)
        : filePath_(filePath),
          projectPartId_(projectPartId),
          fileArguments_(fileArguments),
          unsavedFileContent_(unsavedFileContent),
          documentRevision_(documentRevision),
          hasUnsavedFileContent_(hasUnsavedFileContent)
    {
    }

    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8StringVector &fileArguments,
                  quint32 documentRevision)
        : filePath_(filePath),
          projectPartId_(projectPartId),
          fileArguments_(fileArguments),
          documentRevision_(documentRevision),
          hasUnsavedFileContent_(false)
    {
    }

    const Utf8String &filePath() const
    {
        return filePath_;
    }

    const Utf8String &projectPartId() const
    {
        return projectPartId_;
    }

    const Utf8StringVector &fileArguments() const
    {
        return fileArguments_;
    }

    const Utf8String &unsavedFileContent() const
    {
        return unsavedFileContent_;
    }

    bool hasUnsavedFileContent() const
    {
        return hasUnsavedFileContent_;
    }

    quint32 documentRevision() const
    {
        return documentRevision_;
    }

    friend QDataStream &operator<<(QDataStream &out, const FileContainer &container)
    {
        out << container.filePath_;
        out << container.projectPartId_;
        out << container.fileArguments_;
        out << container.unsavedFileContent_;
        out << container.documentRevision_;
        out << container.hasUnsavedFileContent_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FileContainer &container)
    {
        in >> container.filePath_;
        in >> container.projectPartId_;
        in >> container.fileArguments_;
        in >> container.unsavedFileContent_;
        in >> container.documentRevision_;
        in >> container.hasUnsavedFileContent_;

        return in;
    }

    friend bool operator==(const FileContainer &first, const FileContainer &second)
    {
        return first.filePath_ == second.filePath_ && first.projectPartId_ == second.projectPartId_;
    }

private:
    Utf8String filePath_;
    Utf8String projectPartId_;
    Utf8StringVector fileArguments_;
    Utf8String unsavedFileContent_;
    quint32 documentRevision_;
    bool hasUnsavedFileContent_ = false;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);
void PrintTo(const FileContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd
