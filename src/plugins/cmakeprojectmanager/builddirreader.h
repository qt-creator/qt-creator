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

#include "builddirparameters.h"
#include "cmakebuildtarget.h"
#include "cmakeconfigitem.h"
#include "cmaketool.h"

#include <projectexplorer/rawprojectpart.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>

#include <QFutureInterface>
#include <QObject>
#include <QVector>

namespace ProjectExplorer { class FileNode; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class CMakeProjectNode;

class BuildDirReader : public QObject
{
    Q_OBJECT

public:
    static std::unique_ptr<BuildDirReader> createReader(const BuildDirParameters &p);
    virtual void setParameters(const BuildDirParameters &p) = 0;

    virtual bool isCompatible(const BuildDirParameters &p) = 0;
    virtual void resetData() = 0;
    virtual void parse(bool forceCMakeRun, bool forceConfiguration) = 0;
    virtual void stop() = 0;

    virtual bool isParsing() const = 0;

    virtual QVector<Utils::FilePath> takeProjectFilesToWatch() = 0;
    virtual QList<CMakeBuildTarget> takeBuildTargets(QString &errorMessage) = 0;
    virtual CMakeConfig takeParsedConfiguration(QString &errorMessage) = 0;
    virtual std::unique_ptr<CMakeProjectNode> generateProjectTree(
        const QList<const ProjectExplorer::FileNode *> &allFiles, QString &errorMessage)
        = 0;
    virtual ProjectExplorer::RawProjectParts createRawProjectParts(QString &errorMessage) = 0;

signals:
    void isReadyNow() const;
    void configurationStarted() const;
    void dataAvailable() const;
    void dirty() const;
    void errorOccured(const QString &message) const;

protected:
    BuildDirParameters m_parameters;
};

} // namespace Internal
} // namespace CMakeProjectManager
