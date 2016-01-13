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

#ifndef CPPTOOLS_PROJECTPART_H
#define CPPTOOLS_PROJECTPART_H

#include "cpptools_global.h"

#include "cppprojectfile.h"
#include "projectpartheaderpath.h"

#include <projectexplorer/toolchain.h>

#include <coreplugin/id.h>

#include <utils/fileutils.h>

#include <cplusplus/Token.h>

#include <QString>
#include <QSharedPointer>

namespace ProjectExplorer {
class Project;
}

namespace CppTools {

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

    using Ptr = QSharedPointer<ProjectPart>;


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
    QVector<ProjectFile> files;
    QString projectConfigFile; // currently only used by the Generic Project Manager
    QByteArray projectDefines;
    QByteArray toolchainDefines;
    Core::Id toolchainType;
    ProjectPartHeaderPaths headerPaths;
    QStringList precompiledHeaders;
    LanguageVersion languageVersion;
    LanguageExtensions languageExtensions;
    CPlusPlus::LanguageFeatures languageFeatures;
    QtVersion qtVersion;
    ProjectExplorer::ToolChain::WarningFlags warningFlags;
    bool selectedForBuilding;
};

} // namespace CppTools

#endif // CPPTOOLS_PROJECTPART_H
