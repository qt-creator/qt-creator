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

#include "environment.h"
#include "pchgeneratorinterface.h"
#include "pchgeneratornotifierinterface.h"

#include <projectpartpch.h>

#include <QProcess>

#include <queue>

namespace ClangBackEnd {

template <typename Process>
class PchGenerator final : public PchGeneratorInterface
{
public:
    PchGenerator(Environment &environment,
                 PchGeneratorNotifierInterface *notifier=nullptr)
        : m_environment(environment),
          m_notifier(notifier)
    {
    }

    ~PchGenerator()
    {
        cleanupAllProcesses();
    }

    void startTask(Utils::SmallStringVector &&compilerArguments, ProjectPartPch &&projectPartPch) override
    {
        addTask(std::move(compilerArguments), std::move(projectPartPch));
    }

    void setNotifier(PchGeneratorNotifierInterface *notifier)
    {
        m_notifier = notifier;
    }

unittest_public:
    Process *addTask(Utils::SmallStringVector &&compilerArguments, ProjectPartPch &&projectPartPch)
    {
        auto process = std::make_unique<Process>();
        Process *processPointer = process.get();

        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->setArguments(QStringList(compilerArguments));
        process->setProgram(QString(m_environment.clangCompilerPath()));

        connectProcess(processPointer, std::move(projectPartPch));

        if (!deferProcess())
            startProcess(std::move(process));
        else
            m_deferredProcesses.push(std::move(process));

        return processPointer;
    }

    void connectProcess(Process *process, ProjectPartPch &&projectPartPch)
    {
        auto finishedCallback = [=,projectPartPch=std::move(projectPartPch)] (int exitCode, QProcess::ExitStatus exitStatus) {
            deleteProcess(process);
            activateNextDeferredProcess();
            m_notifier->taskFinished(generateTaskFinishStatus(exitCode, exitStatus), projectPartPch);
        };

        QObject::connect(process,
                         static_cast<void (Process::*)(int, QProcess::ExitStatus)>(&Process::finished),
                         std::move(finishedCallback));
    }

    void startProcess(std::unique_ptr<Process> &&process)
    {
        process->start();
        m_runningProcesses.push_back(std::move(process));
    }

    const std::vector<std::unique_ptr<Process>> &runningProcesses() const
    {
        return m_runningProcesses;
    }

    const std::queue<std::unique_ptr<Process>> &deferredProcesses() const
    {
        return m_deferredProcesses;
    }

    void deleteProcess(Process *process)
    {
        auto found = std::find_if(m_runningProcesses.begin(),
                     m_runningProcesses.end(),
                     [=] (const std::unique_ptr<Process> &entry) {
            return entry.get() == process;
        });

        if (found != m_runningProcesses.end()) {
            std::unique_ptr<Process> avoidDoubleDeletedProcess = std::move(*found);
            m_runningProcesses.erase(found);
        }
    }

    void cleanupAllProcesses()
    {
        std::vector<std::unique_ptr<Process>> runningProcesses = std::move(m_runningProcesses);
        std::queue<std::unique_ptr<Process>> deferredProcesses = std::move(m_deferredProcesses);
    }

    static TaskFinishStatus generateTaskFinishStatus(int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (exitCode != 0 || exitStatus != QProcess::NormalExit)
            return TaskFinishStatus::Unsuccessfully;
         else
            return TaskFinishStatus::Successfully;
    }

    bool deferProcess() const
    {
        return m_environment.hardwareConcurrency() <= m_runningProcesses.size();
    }

    void activateNextDeferredProcess()
    {
        if (!m_deferredProcesses.empty()) {
            std::unique_ptr<Process> process = std::move(m_deferredProcesses.front());
            m_deferredProcesses.pop();

            startProcess(std::move(process));
        }
    }

private:
    std::vector<std::unique_ptr<Process>> m_runningProcesses;
    std::queue<std::unique_ptr<Process>> m_deferredProcesses;
    Environment &m_environment;
    PchGeneratorNotifierInterface *m_notifier=nullptr;
};

} // namespace ClangBackEnd
