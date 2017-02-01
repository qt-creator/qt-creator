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

#include "cppprojectfile.h"
#include "cpptoolsconstants.h"

#include <coreplugin/icore.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QDebug>

namespace CppTools {

ProjectFile::ProjectFile(const QString &filePath, Kind kind)
    : path(filePath)
    , kind(kind)
{
}

bool ProjectFile::operator==(const ProjectFile &other) const
{
    return path == other.path
        && kind == other.kind;
}

ProjectFile::Kind ProjectFile::classify(const QString &filePath)
{
    if (isAmbiguousHeader(filePath))
        return AmbiguousHeader;

    Utils::MimeDatabase mdb;
    const Utils::MimeType mimeType = mdb.mimeTypeForFile(filePath);
    if (!mimeType.isValid())
        return Unsupported;
    const QString mt = mimeType.name();
    if (mt == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE))
        return CSource;
    if (mt == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE))
        return CHeader;
    if (mt == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE))
        return CXXSource;
    if (mt == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE))
        return CXXHeader;
    if (mt == QLatin1String(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE))
        return ObjCSource;
    if (mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return ObjCXXSource;
    if (mt == QLatin1String(CppTools::Constants::QDOC_MIMETYPE))
        return CXXSource;
    if (mt == QLatin1String(CppTools::Constants::MOC_MIMETYPE))
        return CXXSource;
    return Unsupported;
}

bool ProjectFile::isAmbiguousHeader(const QString &filePath)
{
    return filePath.endsWith(".h");
}

bool ProjectFile::isHeader(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CHeader:
    case ProjectFile::CXXHeader:
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
    case ProjectFile::Unsupported: // no file extension, e.g. stl headers
    case ProjectFile::AmbiguousHeader:
        return true;
    default:
        return false;
    }
}

bool ProjectFile::isSource(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CSource:
    case ProjectFile::CXXSource:
    case ProjectFile::ObjCSource:
    case ProjectFile::ObjCXXSource:
    case ProjectFile::CudaSource:
    case ProjectFile::OpenCLSource:
        return true;
    default:
        return false;
    }
}

bool ProjectFile::isHeader() const
{
    return isHeader(kind);
}

bool ProjectFile::isSource() const
{
    return isSource(kind);
}

#define RETURN_TEXT_FOR_CASE(enumValue) case ProjectFile::enumValue: return #enumValue
const char *projectFileKindToText(ProjectFile::Kind kind)
{
    switch (kind) {
        RETURN_TEXT_FOR_CASE(Unclassified);
        RETURN_TEXT_FOR_CASE(Unsupported);
        RETURN_TEXT_FOR_CASE(AmbiguousHeader);
        RETURN_TEXT_FOR_CASE(CHeader);
        RETURN_TEXT_FOR_CASE(CSource);
        RETURN_TEXT_FOR_CASE(CXXHeader);
        RETURN_TEXT_FOR_CASE(CXXSource);
        RETURN_TEXT_FOR_CASE(ObjCHeader);
        RETURN_TEXT_FOR_CASE(ObjCSource);
        RETURN_TEXT_FOR_CASE(ObjCXXHeader);
        RETURN_TEXT_FOR_CASE(ObjCXXSource);
        RETURN_TEXT_FOR_CASE(CudaSource);
        RETURN_TEXT_FOR_CASE(OpenCLSource);
    }

    return "UnhandledProjectFileKind";
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug stream, const CppTools::ProjectFile &projectFile)
{
    stream << projectFile.path << QLatin1String(", ") << projectFileKindToText(projectFile.kind);
    return stream;
}

} // namespace CppTools
