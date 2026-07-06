// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qscxmlcgenerator.h"

#include "baseqtversion.h"
#include "qtkitaspect.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDateTime>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport::Internal {

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
        m_header = m_tmpdir.filePath(targets[0].fileName()).toUrlishString();
        QTC_ASSERT(!m_header.isEmpty(), return);
        m_impl = m_tmpdir.filePath(targets[1].fileName()).toUrlishString();
    }

private:
    Parameters parameters() const override
    {
        Parameters params;

        params.workingDirectory = m_tmpdir.path();
        FilePath tmpFile = params.workingDirectory.pathAppended(source().fileName());
        Kit *kit = project()->activeKit();
        if (!kit)
            kit = KitManager::defaultKit();
        if (QtVersion *version = QtKitAspect::qtVersion(kit)) {
            params.command = {version->qscxmlcFilePath(),
                              {"--header", m_header, "--impl", m_impl, tmpFile.fileName()}};
        }
        params.preRunner = [tmpFile](const QByteArray &sourceContents) {
            QFile input(tmpFile.toUrlishString());
            if (!input.open(QIODevice::WriteOnly))
                return false;
            input.write(sourceContents);
            input.close();
            return true;
        };
        params.postRunner = [wd = params.workingDirectory, targets = this->targets()](Process *) {
            FileNameToContentsHash result;
            for (const FilePath &target : targets) {
                const FilePath file = wd.pathAppended(target.fileName());
                QFile generated(file.toUrlishString());
                if (generated.open(QIODevice::ReadOnly))
                    result[target] = generated.readAll();
            }
            return result;
        };

        return params;
    }

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

// QScxmlcGeneratorFactory

class QScxmlcGeneratorFactory final : public ExtraCompilerFactory
{
public:
    explicit QScxmlcGeneratorFactory(QObject *guard) : m_guard(guard) {}

private:
    FileType sourceType() const final { return FileType::StateChart; }

    QString sourceTag() const final { return QStringLiteral("scxml"); }

    ExtraCompiler *create(const Project *project,
                          const FilePath &source,
                          const FilePaths &targets) final
    {
        return new QScxmlcGenerator(project, source, targets, m_guard);
    }

    QObject *m_guard;
};

void setupQScxmlcGenerator(QObject *guard)
{
    static QScxmlcGeneratorFactory theQScxmlcGeneratorFactory(guard);
}

} // QtSupport::Internal
