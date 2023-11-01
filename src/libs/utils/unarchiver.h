// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "commandline.h"
#include "process.h"

#include <solutions/tasking/tasktree.h>

#include <QObject>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Unarchiver : public QObject
{
    Q_OBJECT
public:
    class SourceAndCommand
    {
    private:
        friend class Unarchiver;
        SourceAndCommand(const FilePath &sourceFile, const CommandLine &commandTemplate)
            : m_sourceFile(sourceFile), m_commandTemplate(commandTemplate) {}
        FilePath m_sourceFile;
        CommandLine m_commandTemplate;
    };

    static expected_str<SourceAndCommand> sourceAndCommand(const FilePath &sourceFile);

    void setSourceAndCommand(const SourceAndCommand &data) { m_sourceAndCommand = data; }
    void setDestDir(const FilePath &destDir) { m_destDir = destDir; }

    void start();

signals:
    void outputReceived(const QString &output);
    void done(bool success);

private:
    std::optional<SourceAndCommand> m_sourceAndCommand;
    FilePath m_destDir;
    std::unique_ptr<Process> m_process;
};

class QTCREATOR_UTILS_EXPORT UnarchiverTaskAdapter : public Tasking::TaskAdapter<Unarchiver>
{
public:
    UnarchiverTaskAdapter();
    void start() final;
};

using UnarchiverTask = Tasking::CustomTask<UnarchiverTaskAdapter>;

} // namespace Utils
