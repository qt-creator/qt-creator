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

#ifndef CLANGBACKEND_FILECONTAINER_H
#define CLANGBACKEND_FILECONTAINER_H

#include <clangbackendipc_global.h>

#include <utf8string.h>

#include <QMetaType>

namespace ClangBackEnd {

class CMBIPC_EXPORT FileContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const FileContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, FileContainer &container);
    friend CMBIPC_EXPORT bool operator==(const FileContainer &first, const FileContainer &second);
    friend CMBIPC_EXPORT bool operator<(const FileContainer &first, const FileContainer &second);
public:
    FileContainer() = default;
    FileContainer(const Utf8String &filePath,
                  const Utf8String &projectPartId,
                  const Utf8String &unsavedFileContent = Utf8String(),
                  bool hasUnsavedFileContent = false);

    const Utf8String &filePath() const;
    const Utf8String &projectPartId() const;
    const Utf8String &unsavedFileContent() const;
    bool hasUnsavedFileContent() const;

private:
    Utf8String filePath_;
    Utf8String projectPartId_;
    Utf8String unsavedFileContent_;
    bool hasUnsavedFileContent_ = false;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const FileContainer &container);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, FileContainer &container);
CMBIPC_EXPORT bool operator==(const FileContainer &first, const FileContainer &second);
CMBIPC_EXPORT bool operator<(const FileContainer &first, const FileContainer &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FileContainer &container);
void PrintTo(const FileContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::FileContainer)

#endif // CLANGBACKEND_FILECONTAINER_H
