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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

namespace Internal {

class ProjectFileAdder
{
public:
    ProjectFileAdder(QList<ProjectFile> &files);
    ~ProjectFileAdder();

    bool maybeAdd(const QString &path);

private:

    void addMapping(const char *mimeName, ProjectFile::Kind kind);

    QList<ProjectFile> &m_files;
    QHash<QString, ProjectFile::Kind> m_mimeNameMapping;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLS_CPPPROJECTFILE_H
