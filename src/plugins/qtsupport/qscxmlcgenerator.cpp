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

#include <QDateTime>
#include <QLoggingCategory>
#include <QUuid>

using namespace ProjectExplorer;

namespace QtSupport {

static QLoggingCategory log("qtc.qsxmlcgenerator");
static const char TaskCategory[] = "Task.Category.ExtraCompiler.QScxmlc";

QScxmlcGenerator::QScxmlcGenerator(const Project *project,
                                   const Utils::FileName &source,
                                   const Utils::FileNameList &targets, QObject *parent) :
    ProcessExtraCompiler(project, source, targets, parent),
    m_tmpdir("qscxmlgenerator")
{
    QTC_ASSERT(targets.count() == 2, return);
    m_header = m_tmpdir.path() + QLatin1Char('/') + targets[0].fileName();
    m_impl = m_tmpdir.path() + QLatin1Char('/') + targets[1].fileName();
}

QList<Task> QScxmlcGenerator::parseIssues(const QByteArray &processStderr)
{
    QList<Task> issues;
    foreach (const QByteArray &line, processStderr.split('\n')) {
        QByteArrayList tokens = line.split(':');

        if (tokens.length() > 4) {
            Utils::FileName file = Utils::FileName::fromUtf8(tokens[0]);
            int line = tokens[1].toInt();
            // int column = tokens[2].toInt(); <- nice, but not needed for now.
            Task::TaskType type = tokens[3].trimmed() == "error" ?
                        Task::Error : Task::Warning;
            QString message = QString::fromUtf8(tokens.mid(4).join(':').trimmed());
            issues.append(Task(type, message, file, line, TaskCategory));
        }
    }
    return issues;
}


Utils::FileName QScxmlcGenerator::command() const
{
    QtSupport::BaseQtVersion *version = nullptr;
    Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitInformation::qtVersion(target->kit());
    else
        version = QtSupport::QtKitInformation::qtVersion(KitManager::defaultKit());

    if (!version)
        return Utils::FileName();

    return Utils::FileName::fromString(version->qscxmlcCommand());
}

QStringList QScxmlcGenerator::arguments() const
{
    QTC_ASSERT(!m_header.isEmpty(), return QStringList());

    return QStringList({QLatin1String("--header"), m_header, QLatin1String("--impl"), m_impl,
                        tmpFile().fileName()});
}

Utils::FileName QScxmlcGenerator::workingDirectory() const
{
    return Utils::FileName::fromString(m_tmpdir.path());
}

bool QScxmlcGenerator::prepareToRun(const QByteArray &sourceContents)
{
    const Utils::FileName fn = tmpFile();
    QFile input(fn.toString());
    if (!input.open(QIODevice::WriteOnly))
        return false;
    input.write(sourceContents);
    input.close();

    return true;
}

FileNameToContentsHash QScxmlcGenerator::handleProcessFinished(QProcess *process)
{
    Q_UNUSED(process);
    const Utils::FileName wd = workingDirectory();
    FileNameToContentsHash result;
    forEachTarget([&](const Utils::FileName &target) {
        Utils::FileName file = wd;
        file.appendPath(target.fileName());
        QFile generated(file.toString());
        if (!generated.open(QIODevice::ReadOnly))
            return;
        result[target] = generated.readAll();
    });
    return result;
}

Utils::FileName QScxmlcGenerator::tmpFile() const
{
    Utils::FileName wd = workingDirectory();
    wd.appendPath(source().fileName());
    return wd;
}

FileType QScxmlcGeneratorFactory::sourceType() const
{
    return FileType::StateChart;
}

QString QScxmlcGeneratorFactory::sourceTag() const
{
    return QStringLiteral("scxml");
}

ExtraCompiler *QScxmlcGeneratorFactory::create(
        const Project *project, const Utils::FileName &source,
        const Utils::FileNameList &targets)
{
    return new QScxmlcGenerator(project, source, targets, this);
}

} // namespace QtSupport
