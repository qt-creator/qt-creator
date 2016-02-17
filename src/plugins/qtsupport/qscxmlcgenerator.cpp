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

#include "qscxmlcgenerator.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QUuid>
#include <QDateTime>

namespace QtSupport {

static QLoggingCategory log("qtc.qsxmlcgenerator");
static const char TaskCategory[] = "Task.Category.ExtraCompiler.QScxmlc";

QScxmlcGenerator::QScxmlcGenerator(const ProjectExplorer::Project *project,
                                   const Utils::FileName &source,
                                   const Utils::FileNameList &targets, QObject *parent) :
    ProjectExplorer::ExtraCompiler(project, source, targets, parent)
{
    connect(&m_process, static_cast<void(QProcess::*)(int)>(&QProcess::finished),
            this, &QScxmlcGenerator::finishProcess);
}

void QScxmlcGenerator::parseIssues(const QByteArray &processStderr)
{
    QList<ProjectExplorer::Task> issues;
    foreach (const QByteArray &line, processStderr.split('\n')) {
        QByteArrayList tokens = line.split(':');

        if (tokens.length() > 4) {
            Utils::FileName file = Utils::FileName::fromUtf8(tokens[0]);
            int line = tokens[1].toInt();
            // int column = tokens[2].toInt(); <- nice, but not needed for now.
            ProjectExplorer::Task::TaskType type = tokens[3].trimmed() == "error" ?
                        ProjectExplorer::Task::Error : ProjectExplorer::Task::Warning;
            QString message = QString::fromUtf8(tokens.mid(4).join(':').trimmed());
            issues.append(ProjectExplorer::Task(type, message, file, line, TaskCategory));
        }
    }
    setCompileIssues(issues);
}

void QScxmlcGenerator::finishProcess()
{
    parseIssues(m_process.readAllStandardError());

    setCompileTime(QDateTime::currentDateTime());
    foreach (const Utils::FileName &target, targets()) {
        QFile generated(m_tmpdir.path() + QLatin1Char('/') + target.fileName());
        if (!generated.open(QIODevice::ReadOnly))
            continue;
        setContent(target, generated.readAll());
    }
}

void QScxmlcGenerator::run(const QByteArray &sourceContent)
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(3000);
    }

    QtSupport::BaseQtVersion *version = 0;
    ProjectExplorer::Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitInformation::qtVersion(target->kit());
    else
        version = QtSupport::QtKitInformation::qtVersion(ProjectExplorer::KitManager::defaultKit());

    if (!version)
        return;

    const QString generator = version->qscxmlcCommand();
    if (!QFileInfo(generator).isExecutable())
        return;

    m_process.setProcessEnvironment(buildEnvironment().toProcessEnvironment());
    m_process.setWorkingDirectory(m_tmpdir.path());

    QFile input(m_tmpdir.path() + QLatin1Char('/') + source().fileName());
    if (!input.open(QIODevice::WriteOnly))
        return;
    input.write(sourceContent);
    input.close();

    qCDebug(log) << "  QScxmlcGenerator::run " << generator << " on "
                 << sourceContent.size() << " bytes";

    m_process.start(generator, QStringList({
            QLatin1String("-oh"), m_tmpdir.path() + QLatin1Char('/') + targets()[0].fileName(),
            QLatin1String("-ocpp"), m_tmpdir.path() + QLatin1Char('/') + targets()[1].fileName(),
            input.fileName()}));
}

ProjectExplorer::FileType QScxmlcGeneratorFactory::sourceType() const
{
    return ProjectExplorer::StateChartType;
}

QString QScxmlcGeneratorFactory::sourceTag() const
{
    return QStringLiteral("scxml");
}

ProjectExplorer::ExtraCompiler *QScxmlcGeneratorFactory::create(
        const ProjectExplorer::Project *project, const Utils::FileName &source,
        const Utils::FileNameList &targets)
{
    return new QScxmlcGenerator(project, source, targets, this);
}

} // namespace QtSupport
