/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLS_CPPPROJECTFILE_H
#define CPPTOOLS_CPPPROJECTFILE_H

#include "cpptools_global.h"

#include <coreplugin/mimedatabase.h>

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

class CPPTOOLS_EXPORT ProjectFileAdder
{
public:
    ProjectFileAdder(QList<ProjectFile> &files);
    ~ProjectFileAdder();

    bool maybeAdd(const QString &path);

private:
    typedef QPair<Core::MimeType, ProjectFile::Kind> Pair;

    void addMapping(const char *mimeName, ProjectFile::Kind kind);

    QList<ProjectFile> &m_files;
    QList<Pair> m_mapping;
    QFileInfo m_fileInfo;
};

QDebug operator<<(QDebug stream, const CppTools::ProjectFile &cxxFile);

} // namespace CppTools

#endif // CPPTOOLS_CPPPROJECTFILE_H
