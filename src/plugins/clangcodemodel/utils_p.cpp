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

#include "unit.h"
#include "utils_p.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace ClangCodeModel {
namespace Internal {

QString getQString(const CXString &cxString, bool disposeCXString)
{
    QString s = QString::fromUtf8(clang_getCString(cxString));
    if (disposeCXString)
        clang_disposeString(cxString);
    return s;
}

namespace {

SourceLocation getLocation(const CXSourceLocation &loc,
                           void (*clangFunction)(CXSourceLocation,
                                                 CXFile *,
                                                 unsigned *,
                                                 unsigned *,
                                                 unsigned *))
{
    CXFile file;
    unsigned line, column, offset;
    (*clangFunction)(loc, &file, &line, &column, &offset);
    return SourceLocation(normalizeFileName(getQString(clang_getFileName(file))),
                          line,
                          column,
                          offset);
}

} // Anonymous

SourceLocation getInstantiationLocation(const CXSourceLocation &loc)
{
    return getLocation(loc, &clang_getInstantiationLocation);
}

SourceLocation getSpellingLocation(const CXSourceLocation &loc)
{
    return getLocation(loc, &clang_getSpellingLocation);
}

SourceLocation getExpansionLocation(const CXSourceLocation &loc)
{
//    return getLocation(loc, &clang_getExpansionLocation);
    return getLocation(loc, &clang_getInstantiationLocation);
}

QString normalizeFileName(const QString &fileName)
{
    if (fileName.isEmpty())
        return fileName;

    return normalizeFileName(QFileInfo(fileName));
}

QString normalizeFileName(const QFileInfo &fileInfo)
{
    if (!fileInfo.isFile())
        return QString();

    return QDir::cleanPath(fileInfo.absoluteFilePath());
}

QStringList formattedDiagnostics(const Unit::Ptr &unit)
{
    QStringList diags;
    if (!unit->isLoaded())
        return diags;

    const unsigned count = unit->getNumDiagnostics();
    for (unsigned i = 0; i < count; ++i) {
        CXDiagnostic diag = unit->getDiagnostic(i);

        unsigned opt = CXDiagnostic_DisplaySourceLocation
                | CXDiagnostic_DisplayColumn
                | CXDiagnostic_DisplaySourceRanges
                | CXDiagnostic_DisplayOption
                | CXDiagnostic_DisplayCategoryId
                | CXDiagnostic_DisplayCategoryName
                ;
        diags << getQString(clang_formatDiagnostic(diag, opt));
        clang_disposeDiagnostic(diag);
    }

    return diags;
}

} // Internal
} // ClangCodeModel
