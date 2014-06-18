/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPPROJECTPART_H
#define CPPPROJECTPART_H

#include "cpptools_global.h"

#include "cppprojectfile.h"

#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

#include <QPointer>

namespace CppTools {

class CPPTOOLS_EXPORT ProjectPart
{
public: // Types
    enum CVersion {
        C89,
        C99,
        C11
    };

    enum CXXVersion {
        CXX98,
        CXX11
    };

    enum CXXExtension {
        NoExtensions = 0x0,
        GnuExtensions = 0x1,
        MicrosoftExtensions = 0x2,
        BorlandExtensions = 0x4,
        OpenMPExtensions = 0x8,

        AllExtensions = GnuExtensions | MicrosoftExtensions | BorlandExtensions | OpenMPExtensions
    };
    Q_DECLARE_FLAGS(CXXExtensions, CXXExtension)

    enum QtVersion {
        UnknownQt = -1,
        NoQt = 0,
        Qt4 = 1,
        Qt5 = 2
    };

    typedef QSharedPointer<ProjectPart> Ptr;

    struct HeaderPath {
        enum Type { InvalidPath, IncludePath, FrameworkPath };

    public:
        QString path;
        Type type;

        HeaderPath(): type(InvalidPath) {}
        HeaderPath(const QString &path, Type type): path(path), type(type) {}

        bool isValid() const { return type != InvalidPath; }
        bool isFrameworkPath() const { return type == FrameworkPath; }

        bool operator==(const HeaderPath &other) const
        { return path == other.path && type == other.type; }

        bool operator!=(const HeaderPath &other) const
        { return !(*this == other); }
    };
    typedef QList<HeaderPath> HeaderPaths;

public: // methods
    ProjectPart();

    void evaluateToolchain(const ProjectExplorer::ToolChain *tc,
                           const QStringList &cxxflags,
                           const QStringList &cflags,
                           const Utils::FileName &sysRoot);

    Ptr copy() const;

    static QByteArray readProjectConfigFile(const ProjectPart::Ptr &part);

public: // fields
    QString displayName;
    QString projectFile;
    ProjectExplorer::Project *project;
    QList<ProjectFile> files;
    QString projectConfigFile; // currently only used by the Generic Project Manager
    QByteArray projectDefines;
    QByteArray toolchainDefines;
    QList<HeaderPath> headerPaths;
    QStringList precompiledHeaders;
    CVersion cVersion;
    CXXVersion cxxVersion;
    CXXExtensions cxxExtensions;
    QtVersion qtVersion;
    ProjectExplorer::ToolChain::WarningFlags cWarningFlags;
    ProjectExplorer::ToolChain::WarningFlags cxxWarningFlags;
};

inline uint qHash(const ProjectPart::HeaderPath &key, uint seed = 0)
{ return ((qHash(key.path) << 2) | key.type) ^ seed; }

class CPPTOOLS_EXPORT ProjectInfo
{
public:
    ProjectInfo();
    ProjectInfo(QPointer<ProjectExplorer::Project> project);

    operator bool() const;
    bool isValid() const;
    bool isNull() const;

    QPointer<ProjectExplorer::Project> project() const;
    const QList<ProjectPart::Ptr> projectParts() const;

    void appendProjectPart(const ProjectPart::Ptr &part);
    void clearProjectParts();

    const ProjectPart::HeaderPaths headerPaths() const;
    const QStringList sourceFiles() const;
    const QByteArray defines() const;

private:
    QPointer<ProjectExplorer::Project> m_project;
    QList<ProjectPart::Ptr> m_projectParts;
    // The members below are (re)calculated from the project parts once a part is appended.
    ProjectPart::HeaderPaths m_headerPaths;
    QStringList m_sourceFiles;
    QByteArray m_defines;
};

class CPPTOOLS_EXPORT ProjectPartBuilder
{
public:
    ProjectPartBuilder(ProjectInfo &m_pInfo);

    void setQtVersion(ProjectPart::QtVersion qtVersion);
    void setCFlags(const QStringList &flags);
    void setCxxFlags(const QStringList &flags);
    void setDefines(const QByteArray &defines);
    void setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths);
    void setIncludePaths(const QStringList &includePaths);
    void setPreCompiledHeaders(const QStringList &pchs);
    void setProjectFile(const QString &projectFile);
    void setDisplayName(const QString &displayName);
    void setConfigFileName(const QString &configFileName);

    QList<Core::Id> createProjectPartsForFiles(const QStringList &files);

private:
    void createProjectPart(const QStringList &theSources, const QString &partName,
                           ProjectPart::CVersion cVersion, ProjectPart::CXXVersion cxxVersion);

private:
    ProjectPart::Ptr m_templatePart;
    ProjectInfo &m_pInfo;
    QStringList m_cFlags, m_cxxFlags;
};

} // namespace CppTools

#endif // CPPPROJECTPART_H
