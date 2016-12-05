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

#pragma once

#include "cpptools_global.h"

#include "cppprojectfile.h"
#include "projectpartheaderpath.h"

#include <projectexplorer/projectexplorer_global.h>

#include <coreplugin/id.h>

#include <cplusplus/Token.h>

#include <QString>
#include <QSharedPointer>

namespace ProjectExplorer {
class Project;
}

namespace CppTools {

class CPPTOOLS_EXPORT ProjectPart
{
public:
    enum LanguageVersion {
        C89,
        C99,
        C11,
        LatestCVersion = C11,
        CXX98,
        CXX03,
        CXX11,
        CXX14,
        CXX17,
        LatestCxxVersion = CXX17,
    };

    enum LanguageExtension {
        NoExtensions         = 0,
        GnuExtensions        = 1 << 0,
        MicrosoftExtensions  = 1 << 1,
        BorlandExtensions    = 1 << 2,
        OpenMPExtensions     = 1 << 3,
        ObjectiveCExtensions = 1 << 4,

        AllExtensions = GnuExtensions
                      | MicrosoftExtensions
                      | BorlandExtensions
                      | OpenMPExtensions
                      | ObjectiveCExtensions
    };
    Q_DECLARE_FLAGS(LanguageExtensions, LanguageExtension)

    enum QtVersion {
        UnknownQt = -1,
        NoQt,
        Qt4,
        Qt5
    };

    enum ToolChainWordWidth {
        WordWidth32Bit,
        WordWidth64Bit,
    };

    using Ptr = QSharedPointer<ProjectPart>;

public:
    QString id() const;

    Ptr copy() const;
    void updateLanguageFeatures();

    static QByteArray readProjectConfigFile(const Ptr &projectPart);

public:
    ProjectExplorer::Project *project = nullptr;

    QString displayName;

    QString projectFile;
    QString projectConfigFile; // currently only used by the Generic Project Manager

    ProjectFiles files;

    QStringList precompiledHeaders;
    ProjectPartHeaderPaths headerPaths;

    QByteArray projectDefines;

    LanguageVersion languageVersion = LatestCxxVersion;
    LanguageExtensions languageExtensions = NoExtensions;
    ProjectExplorer::WarningFlags warningFlags = ProjectExplorer::WarningFlags::Default;
    QtVersion qtVersion = UnknownQt;
    CPlusPlus::LanguageFeatures languageFeatures;

    bool selectedForBuilding = true;

    Core::Id toolchainType;
    bool isMsvc2015Toolchain = false;
    QByteArray toolchainDefines;
    ToolChainWordWidth toolChainWordWidth = WordWidth32Bit;
    QString toolChainTargetTriple;
};

} // namespace CppTools
