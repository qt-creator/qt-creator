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

#include "cppprojectfile.h"

#include "cpptoolsconstants.h"

#include <coreplugin/icore.h>
#include <utils/mimetypes/mimedatabase.h>
#
#include <QDebug>

namespace CppTools {

ProjectFile::ProjectFile()
    : kind(CHeader)
{
}

ProjectFile::ProjectFile(const QString &file, Kind kind)
    : path(file)
    , kind(kind)
{
}

ProjectFile::Kind ProjectFile::classify(const QString &file)
{
    Utils::MimeDatabase mdb;
    const Utils::MimeType mimeType = mdb.mimeTypeForFile(file);
    if (!mimeType.isValid())
        return Unclassified;
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
    return Unclassified;
}

/// @return True if file is header or cannot be classified (i.e has no file extension)
bool ProjectFile::isHeader(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CHeader:
    case ProjectFile::CXXHeader:
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
    case ProjectFile::Unclassified:
        return true;
    default:
        return false;
    }
}

/// @return True if file is correctly classified source
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

QDebug operator<<(QDebug stream, const CppTools::ProjectFile &cxxFile)
{
    const char *kind;
    switch (cxxFile.kind) {
    case CppTools::ProjectFile::CHeader: kind = "CHeader"; break;
    case CppTools::ProjectFile::CSource: kind = "CSource"; break;
    case CppTools::ProjectFile::CXXHeader: kind = "CXXHeader"; break;
    case CppTools::ProjectFile::CXXSource: kind = "CXXSource"; break;
    case CppTools::ProjectFile::ObjCHeader: kind = "ObjCHeader"; break;
    case CppTools::ProjectFile::ObjCSource: kind = "ObjCSource"; break;
    case CppTools::ProjectFile::ObjCXXHeader: kind = "ObjCXXHeader"; break;
    case CppTools::ProjectFile::ObjCXXSource: kind = "ObjCXXSource"; break;
    case CppTools::ProjectFile::CudaSource: kind = "CudaSource"; break;
    case CppTools::ProjectFile::OpenCLSource: kind = "OpenCLSource"; break;
    default: kind = "INVALID"; break;
    }
    stream << cxxFile.path << QLatin1String(", ") << kind;
    return stream;
}

namespace Internal {

ProjectFileAdder::ProjectFileAdder(QList<ProjectFile> &files)
    : m_files(files)
{
    addMapping(CppTools::Constants::C_SOURCE_MIMETYPE, ProjectFile::CSource);
    addMapping(CppTools::Constants::C_HEADER_MIMETYPE, ProjectFile::CHeader);
    addMapping(CppTools::Constants::CPP_SOURCE_MIMETYPE, ProjectFile::CXXSource);
    addMapping(CppTools::Constants::CPP_HEADER_MIMETYPE, ProjectFile::CXXHeader);
    addMapping(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE, ProjectFile::ObjCSource);
    addMapping(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE, ProjectFile::ObjCXXSource);
}

ProjectFileAdder::~ProjectFileAdder()
{
}

bool ProjectFileAdder::maybeAdd(const QString &path)
{
    Utils::MimeDatabase mdb;
    const Utils::MimeType mt = mdb.mimeTypeForFile(path);
    if (m_mimeNameMapping.contains(mt.name())) {
        m_files << ProjectFile(path, m_mimeNameMapping.value(mt.name()));
        return true;
    }
    return false;
}

void ProjectFileAdder::addMapping(const char *mimeName, ProjectFile::Kind kind)
{
    Utils::MimeDatabase mdb;
    Utils::MimeType mimeType = mdb.mimeTypeForName(QLatin1String(mimeName));
    if (mimeType.isValid())
        m_mimeNameMapping.insert(mimeType.name(), kind);
}

} // namespace Internal
} // namespace CppTools

