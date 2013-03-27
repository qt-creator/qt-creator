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

#include "cppprojectfile.h"

#include "cpptoolsconstants.h"

#include <coreplugin/icore.h>

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
    const Core::MimeDatabase *mimeDatabase = Core::ICore::mimeDatabase();
    const QFileInfo fi(file);
    const Core::MimeType mimeType = mimeDatabase->findByFile(fi);
    if (!mimeType)
        return Unclassified;
    const QString mt = mimeType.type();
    if (mt == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE))
        return CSource;
    if (mt == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE))
        return CHeader;
    if (mt == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE))
        return CXXSource;
    if (mt == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE))
        return CXXHeader;
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

ProjectFileAdder::ProjectFileAdder(QList<ProjectFile> &files)
    : m_files(files)
{
    addMapping(CppTools::Constants::C_SOURCE_MIMETYPE, ProjectFile::CSource);
    addMapping(CppTools::Constants::C_HEADER_MIMETYPE, ProjectFile::CHeader);
    addMapping(CppTools::Constants::CPP_SOURCE_MIMETYPE, ProjectFile::CXXSource);
    addMapping(CppTools::Constants::CPP_HEADER_MIMETYPE, ProjectFile::CXXHeader);
    addMapping(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE, ProjectFile::ObjCXXSource);
}

ProjectFileAdder::~ProjectFileAdder()
{
}

bool ProjectFileAdder::maybeAdd(const QString &path)
{
    m_fileInfo.setFile(path);
    foreach (const Pair &pair, m_mapping)
        if (pair.first.matchesFile(path)) {
            m_files << ProjectFile(path, pair.second);
            return true;
        }
    return false;
}

void ProjectFileAdder::addMapping(const char *mimeName, ProjectFile::Kind kind)
{
    const Core::MimeDatabase *mimeDatabase = Core::ICore::mimeDatabase();
    Core::MimeType mimeType = mimeDatabase->findByType(QLatin1String(mimeName));
    if (!mimeType.isNull())
        m_mapping.append(Pair(mimeType, kind));
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

} // namespace CppTools
