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

#include "cmakeconfigitem.h"
#include "cmakeproject.h"
#include "cmaketool.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>

#include <QFutureInterface>
#include <QObject>

namespace CppTools { class ProjectPartBuilder; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class CMakeListsNode;

class BuildDirReader : public QObject
{
    Q_OBJECT

public:
    struct Parameters {
        Parameters();
        Parameters(const CMakeBuildConfiguration *bc);
        Parameters(const Parameters &other);

        QString projectName;

        Utils::FileName sourceDirectory;
        Utils::FileName buildDirectory;
        Utils::Environment environment;
        Utils::FileName cmakeExecutable;
        CMakeTool::Version cmakeVersion;
        bool cmakeHasServerMode = false;
        CMakeTool::PathMapper pathMapper;

        QByteArray cxxToolChainId;
        QByteArray cToolChainId;

        Utils::FileName sysRoot;

        Utils::MacroExpander *expander = nullptr;

        CMakeConfig configuration;

        QString generator;
        QString extraGenerator;
        QString platform;
        QString toolset;
        QStringList generatorArguments;
        bool isAutorun = false;
    };

    static BuildDirReader *createReader(const BuildDirReader::Parameters &p);
    virtual void setParameters(const Parameters &p);

    virtual bool isCompatible(const Parameters &p) = 0;
    virtual void resetData() = 0;
    virtual void parse(bool force) = 0;
    virtual void stop() = 0;

    virtual bool isReady() const { return true; }
    virtual bool isParsing() const = 0;
    virtual bool hasData() const = 0;

    virtual CMakeConfig takeParsedConfiguration() = 0;
    virtual QList<CMakeBuildTarget> buildTargets() const = 0;
    virtual void generateProjectTree(CMakeListsNode *root,
                                     const QList<const ProjectExplorer::FileNode *> &allFiles) = 0;
    virtual QSet<Core::Id> updateCodeModel(CppTools::ProjectPartBuilder &ppBuilder) = 0;

signals:
    void isReadyNow() const;
    void configurationStarted() const;
    void dataAvailable() const;
    void dirty() const;
    void errorOccured(const QString &message) const;

protected:
    Parameters m_parameters;
};

} // namespace Internal
} // namespace CMakeProjectManager
