// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qscxmlcgenerator.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDateTime>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

static QLoggingCategory log("qtc.qscxmlcgenerator", QtWarningMsg);

const char TaskCategory[] = "Task.Category.ExtraCompiler.QScxmlc";

class QScxmlcGenerator final : public ProcessExtraCompiler
{
public:
    QScxmlcGenerator(const Project *project, const FilePath &source,
                     const FilePaths &targets, QObject *parent)
        : ProcessExtraCompiler(project, source, targets, parent)
        , m_tmpdir("qscxmlgenerator")
    {
        QTC_ASSERT(targets.count() == 2, return);
        m_header = m_tmpdir.filePath(targets[0].fileName()).toString();
        QTC_ASSERT(!m_header.isEmpty(), return);
        m_impl = m_tmpdir.filePath(targets[1].fileName()).toString();
    }

private:
    FilePath command() const override;

    QStringList arguments() const override
    {
        return {"--header", m_header, "--impl", m_impl, tmpFile().fileName()};
    }

    FilePath workingDirectory() const override
    {
        return m_tmpdir.path();
    }

    FilePath tmpFile() const
    {
        return workingDirectory().pathAppended(source().fileName());
    }

    FileNameToContentsHash handleProcessFinished(Process *process) override;
    bool prepareToRun(const QByteArray &sourceContents) override;
    Tasks parseIssues(const QByteArray &processStderr) override;

    TemporaryDirectory m_tmpdir;
    QString m_header;
    QString m_impl;
};

Tasks QScxmlcGenerator::parseIssues(const QByteArray &processStderr)
{
    Tasks issues;
    const QList<QByteArray> lines = processStderr.split('\n');
    for (const QByteArray &line : lines) {
        QByteArrayList tokens = line.split(':');

        if (tokens.length() > 4) {
            FilePath file = FilePath::fromUtf8(tokens[0]);
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

FilePath QScxmlcGenerator::command() const
{
    QtSupport::QtVersion *version = nullptr;
    Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitAspect::qtVersion(target->kit());
    else
        version = QtSupport::QtKitAspect::qtVersion(KitManager::defaultKit());

    if (!version)
        return {};

    return version->qscxmlcFilePath();
}

bool QScxmlcGenerator::prepareToRun(const QByteArray &sourceContents)
{
    const FilePath fn = tmpFile();
    QFile input(fn.toString());
    if (!input.open(QIODevice::WriteOnly))
        return false;
    input.write(sourceContents);
    input.close();

    return true;
}

FileNameToContentsHash QScxmlcGenerator::handleProcessFinished(Process *process)
{
    Q_UNUSED(process)
    const FilePath wd = workingDirectory();
    FileNameToContentsHash result;
    forEachTarget([&](const FilePath &target) {
        const FilePath file = wd.pathAppended(target.fileName());
        QFile generated(file.toString());
        if (!generated.open(QIODevice::ReadOnly))
            return;
        result[target] = generated.readAll();
    });
    return result;
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
        const Project *project, const FilePath &source,
        const FilePaths &targets)
{
    return new QScxmlcGenerator(project, source, targets, this);
}

} // namespace QtSupport
