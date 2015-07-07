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

#ifndef CPPPROJECTPART_H
#define CPPPROJECTPART_H

#include "cpptools_global.h"

#include "cppprojectfile.h"

#include <projectexplorer/toolchain.h>

#include <cplusplus/Token.h>

#include <QPointer>
#include <QSet>

#include <functional>

namespace ProjectExplorer { class Project; }

namespace Utils { class FileName; }

namespace CppTools {
class ProjectFile;

class CPPTOOLS_EXPORT ProjectPart
{
public: // Types
    enum LanguageVersion {
        C89,
        C99,
        C11,
        CXX98,
        CXX03,
        CXX11,
        CXX14,
        CXX17
    };

    enum LanguageExtension {
        NoExtensions         = 0,
        GnuExtensions        = 1 << 0,
        MicrosoftExtensions  = 1 << 1,
        BorlandExtensions    = 1 << 2,
        OpenMPExtensions     = 1 << 3,
        ObjectiveCExtensions = 1 << 4,

        AllExtensions = GnuExtensions | MicrosoftExtensions | BorlandExtensions | OpenMPExtensions
                      | ObjectiveCExtensions
    };
    Q_DECLARE_FLAGS(LanguageExtensions, LanguageExtension)

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
                           const QStringList &commandLineFlags,
                           const Utils::FileName &sysRoot);

    void updateLanguageFeatures();
    Ptr copy() const;

    QString id() const;

    static QByteArray readProjectConfigFile(const ProjectPart::Ptr &part);

public: // fields
    QString displayName;
    QString projectFile;
    ProjectExplorer::Project *project;
    QList<ProjectFile> files;
    QString projectConfigFile; // currently only used by the Generic Project Manager
    QByteArray projectDefines;
    QByteArray toolchainDefines;
    Core::Id toolchainType;
    QList<HeaderPath> headerPaths;
    QStringList precompiledHeaders;
    LanguageVersion languageVersion;
    LanguageExtensions languageExtensions;
    CPlusPlus::LanguageFeatures languageFeatures;
    QtVersion qtVersion;
    ProjectExplorer::ToolChain::WarningFlags warningFlags;
    bool selectedForBuilding;
};

inline uint qHash(const ProjectPart::HeaderPath &key, uint seed = 0)
{ return ((qHash(key.path) << 2) | key.type) ^ seed; }

class CPPTOOLS_EXPORT ProjectInfo
{
public:
    ProjectInfo();
    ProjectInfo(QPointer<ProjectExplorer::Project> project);

    bool isValid() const;

    bool operator ==(const ProjectInfo &other) const;
    bool operator !=(const ProjectInfo &other) const;
    bool definesChanged(const ProjectInfo &other) const;
    bool configurationChanged(const ProjectInfo &other) const;
    bool configurationOrFilesChanged(const ProjectInfo &other) const;

    QPointer<ProjectExplorer::Project> project() const;
    const QList<ProjectPart::Ptr> projectParts() const;

    void appendProjectPart(const ProjectPart::Ptr &part);
    void finish();

    const ProjectPart::HeaderPaths headerPaths() const;
    const QSet<QString> sourceFiles() const;
    const QByteArray defines() const;

    // Source file --> List of compiler calls
    typedef QHash<QString, QList<QStringList>> CompilerCallData;
    void setCompilerCallData(const CompilerCallData &data);
    CompilerCallData compilerCallData() const;

private:
    QPointer<ProjectExplorer::Project> m_project;
    QList<ProjectPart::Ptr> m_projectParts;
    CompilerCallData m_compilerCallData;
    // The members below are (re)calculated from the project parts with finish()
    ProjectPart::HeaderPaths m_headerPaths;
    QSet<QString> m_sourceFiles;
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
                           ProjectPart::LanguageVersion languageVersion,
                           ProjectPart::LanguageExtensions languageExtensions);

private:
    ProjectPart::Ptr m_templatePart;
    ProjectInfo &m_pInfo;
    QStringList m_cFlags, m_cxxFlags;
};

class CPPTOOLS_EXPORT CompilerOptionsBuilder
{
public:
    CompilerOptionsBuilder(const ProjectPart::Ptr &projectPart);
    virtual ~CompilerOptionsBuilder() {}

    QStringList options() const;

    // Add custom options
    void add(const QString &option);
    void addDefine(const QByteArray &defineLine);

    // Add options based on project part
    void addHeaderPathOptions();
    void addToolchainAndProjectDefines();
    virtual void addLanguageOption(ProjectFile::Kind fileKind);
    virtual void addOptionsForLanguage(bool checkForBorlandExtensions = true);

protected:
    virtual bool excludeDefineLine(const QByteArray &defineLine) const;
    virtual bool excludeHeaderPath(const QString &headerPath) const;

    virtual QString defineOption() const;
    virtual QString includeOption() const;

    const ProjectPart::Ptr m_projectPart;

private:
    QString defineLineToDefineOption(const QByteArray &defineLine);

    QStringList m_options;
};

} // namespace CppTools

#endif // CPPPROJECTPART_H
