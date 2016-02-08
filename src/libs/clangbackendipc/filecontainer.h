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

#ifndef CLANGBACKEND_FILECONTAINER_H
#define CLANGBACKEND_FILECONTAINER_H

#include <clangbackendipc_global.h>

#include <utf8string.h>
#include <utf8stringvector.h>

namespace ClangBackEnd {

class CMBIPC_EXPORT FileContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const FileContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, FileContainer &container);
    friend CMBIPC_EXPORT bool operator==(const FileContainer &first, const FileContainer &second);
public:
    FileContainer() = default;
    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false,
                  quint32 documentRevision = 0);
    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8StringVector &fileArguments,
                  quint32 documentRevision);

    const Utf8String &filePath() const;
    const Utf8String &projectPartId() const;
    const Utf8StringVector &fileArguments() const;
    const Utf8String &unsavedFileContent() const;
    bool hasUnsavedFileContent() const;
    quint32 documentRevision() const;

private:
    Utf8String filePath_;
    Utf8String projectPartId_;
    Utf8StringVector fileArguments_;
    Utf8String unsavedFileContent_;
    quint32 documentRevision_;
    bool hasUnsavedFileContent_ = false;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const FileContainer &container);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, FileContainer &container);
CMBIPC_EXPORT bool operator==(const FileContainer &first, const FileContainer &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);
void PrintTo(const FileContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd

#endif // CLANGBACKEND_FILECONTAINER_H
