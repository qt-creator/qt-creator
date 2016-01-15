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

#ifndef CPPTOOLS_CPPPROJECTFILE_H
#define CPPTOOLS_CPPPROJECTFILE_H

#include "cpptools_global.h"

#include <QHash>
#include <QList>
#include <QString>

namespace CppTools {

class CPPTOOLS_EXPORT ProjectFile
{
public:
    // enums and types
    enum Kind {
        Unclassified = 0,
        CHeader = 1,
        CSource = 2,
        CXXHeader = 3,
        CXXSource = 4,
        ObjCHeader = 5,
        ObjCSource = 6,
        ObjCXXHeader = 7,
        ObjCXXSource = 8,
        CudaSource = 9,
        OpenCLSource = 10
    };

    ProjectFile();
    ProjectFile(const QString &file, Kind kind);

    static Kind classify(const QString &file);
    static bool isHeader(Kind kind);
    static bool isSource(Kind kind);

    QString path;
    Kind kind;
};

QDebug operator<<(QDebug stream, const CppTools::ProjectFile &cxxFile);

} // namespace CppTools

#endif // CPPTOOLS_CPPPROJECTFILE_H
