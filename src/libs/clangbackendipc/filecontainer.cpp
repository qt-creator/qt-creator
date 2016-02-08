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

#include "filecontainer.h"

#include "clangbackendipcdebugutils.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

FileContainer::FileContainer(const Utf8String &filePath,
                             const Utf8String &projectPartId,
                             const Utf8String &unsavedFileContent,
                             bool hasUnsavedFileContent,
                             quint32 documentRevision)
    : filePath_(filePath),
      projectPartId_(projectPartId),
      unsavedFileContent_(unsavedFileContent),
      documentRevision_(documentRevision),
      hasUnsavedFileContent_(hasUnsavedFileContent)
{
}

FileContainer::FileContainer(const Utf8String &filePath,
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

const Utf8String &FileContainer::filePath() const
{
    return filePath_;
}

const Utf8String &FileContainer::projectPartId() const
{
    return projectPartId_;
}

const Utf8StringVector &FileContainer::fileArguments() const
{
    return fileArguments_;
}

const Utf8String &FileContainer::unsavedFileContent() const
{
    return unsavedFileContent_;
}

bool FileContainer::hasUnsavedFileContent() const
{
    return hasUnsavedFileContent_;
}

quint32 FileContainer::documentRevision() const
{
    return documentRevision_;
}

QDataStream &operator<<(QDataStream &out, const FileContainer &container)
{
    out << container.filePath_;
    out << container.projectPartId_;
    out << container.fileArguments_;
    out << container.unsavedFileContent_;
    out << container.documentRevision_;
    out << container.hasUnsavedFileContent_;

    return out;
}

QDataStream &operator>>(QDataStream &in, FileContainer &container)
{
    in >> container.filePath_;
    in >> container.projectPartId_;
    in >> container.fileArguments_;
    in >> container.unsavedFileContent_;
    in >> container.documentRevision_;
    in >> container.hasUnsavedFileContent_;

    return in;
}

bool operator==(const FileContainer &first, const FileContainer &second)
{
    return first.filePath_ == second.filePath_ && first.projectPartId_ == second.projectPartId_;
}

QDebug operator<<(QDebug debug, const FileContainer &container)
{
    debug.nospace() << "FileContainer("
                    << container.filePath() << ", "
                    << container.projectPartId() << ", "
                    << container.fileArguments() << ", "
                    << container.documentRevision();

    if (container.hasUnsavedFileContent()) {
        const Utf8String fileWithContent = debugWriteFileForInspection(
                    container.unsavedFileContent(),
                    debugId(container));
        debug.nospace() << ", "
                        << "<" << fileWithContent << ">";
    }

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const FileContainer &container, ::std::ostream* os)
{
    *os << "FileContainer("
        << container.filePath().constData() << ", "
        << container.projectPartId().constData() << ", "
        << container.fileArguments().constData() << ", "
        << container.documentRevision();

    if (container.hasUnsavedFileContent())
        *os << ", "
            << container.unsavedFileContent().constData();

    *os << ")";
}

} // namespace ClangBackEnd

