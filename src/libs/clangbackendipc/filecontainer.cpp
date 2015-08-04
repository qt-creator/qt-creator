/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "filecontainer.h"

#include "clangbackendipcdebugutils.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

FileContainer::FileContainer(const Utf8String &fileName,
                             const Utf8String &projectPartId,
                             const Utf8String &unsavedFileContent,
                             bool hasUnsavedFileContent)
    : filePath_(fileName),
      projectPartId_(projectPartId),
      unsavedFileContent_(unsavedFileContent),
      hasUnsavedFileContent_(hasUnsavedFileContent)
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

const Utf8String &FileContainer::unsavedFileContent() const
{
    return unsavedFileContent_;
}

bool FileContainer::hasUnsavedFileContent() const
{
    return hasUnsavedFileContent_;
}

QDataStream &operator<<(QDataStream &out, const FileContainer &container)
{
    out << container.filePath_;
    out << container.projectPartId_;
    out << container.unsavedFileContent_;
    out << container.hasUnsavedFileContent_;

    return out;
}

QDataStream &operator>>(QDataStream &in, FileContainer &container)
{
    in >> container.filePath_;
    in >> container.projectPartId_;
    in >> container.unsavedFileContent_;
    in >> container.hasUnsavedFileContent_;

    return in;
}

bool operator==(const FileContainer &first, const FileContainer &second)
{
    return first.filePath_ == second.filePath_ && first.projectPartId_ == second.projectPartId_;
}

bool operator<(const FileContainer &first, const FileContainer &second)
{
    if (first.filePath_ == second.filePath_)
        return first.projectPartId_ < second.projectPartId_;

    return first.filePath_ < second.filePath_;
}

QDebug operator<<(QDebug debug, const FileContainer &container)
{
    debug.nospace() << "FileContainer("
                    << container.filePath()
                    << ", "
                    << container.projectPartId();

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
        << container.filePath().constData()
        << ", "
        << container.projectPartId().constData();

    if (container.hasUnsavedFileContent())
        *os << ", "
            << container.unsavedFileContent().constData();

    *os << ")";
}

} // namespace ClangBackEnd

