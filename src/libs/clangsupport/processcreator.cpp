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

#include "processcreator.h"

#include "processexception.h"
#include "processstartedevent.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTemporaryDir>

namespace ClangBackEnd {

using namespace std::chrono_literals;

ProcessCreator::ProcessCreator()
{
}

void ProcessCreator::setTemporaryDirectoryPattern(const QString &temporaryDirectoryPattern)
{
    m_temporaryDirectoryPattern = temporaryDirectoryPattern;
    resetTemporaryDirectory();
}

void ProcessCreator::setProcessPath(const QString &processPath)
{
    m_processPath = processPath;
}

void ProcessCreator::setArguments(const QStringList &arguments)
{
    m_arguments = arguments;
}

void ProcessCreator::setEnvironment(const Utils::Environment &environment)
{
    m_environment = environment;
}

std::future<QProcessUniquePointer> ProcessCreator::createProcess() const
{
    return std::async(std::launch::async, [&] {
        checkIfProcessPathExists();
        auto process = QProcessUniquePointer(new QProcess);
        process->setProcessChannelMode(QProcess::QProcess::ForwardedChannels);
        process->setProcessEnvironment(processEnvironment());
        process->start(m_processPath, m_arguments);
        process->waitForStarted(5000);

        checkIfProcessWasStartingSuccessful(process.get());

        postProcessStartedEvent();

        process->moveToThread(QCoreApplication::instance()->thread());

        return process;
    });
}

void ProcessCreator::setObserver(QObject *observer)
{
    this->m_observer = observer;
}

void ProcessCreator::checkIfProcessPathExists() const
{
    if (!QFileInfo::exists(m_processPath)) {
        const QString messageTemplate = QCoreApplication::translate("ProcessCreator",
                                                                    "Executable does not exist: %1");
        throwProcessException(messageTemplate.arg(m_processPath));
    }
}

void ProcessCreator::checkIfProcessWasStartingSuccessful(QProcess *process) const
{
    if (process->exitStatus() == QProcess::CrashExit || process->exitCode() != 0)
        dispatchProcessError(process);
}

void ProcessCreator::dispatchProcessError(QProcess *process) const
{
    switch (process->error()) {
        case QProcess::UnknownError: {
            const QString message = QCoreApplication::translate("ProcessCreator",
                                                                "Unknown error occurred.");
            throwProcessException(message);
        };
        case QProcess::Crashed: {
            const QString message = QCoreApplication::translate("ProcessCreator",
                                                                "Process crashed.");
            throwProcessException(message);
        };
        case QProcess::FailedToStart: {
            const QString message = QCoreApplication::translate("ProcessCreator",
                                                                "Process failed at startup.");
            throwProcessException(message);
        };
        case QProcess::Timedout: {
            const QString message = QCoreApplication::translate("ProcessCreator",
                                                                "Process timed out.");
            throwProcessException(message);
        };
        case QProcess::WriteError: {
            const QString message = QCoreApplication::translate("ProcessCreator",
                                                                "Cannot write to process.");
            throwProcessException(message);
        };
        case QProcess::ReadError: {
            const QString message = QCoreApplication::translate("ProcessCreator",
                                                                "Cannot read from process.");
            throwProcessException(message);
        };
    }

    throwProcessException("Internal impossible error!");
}

void ProcessCreator::postProcessStartedEvent() const
{
    if (m_observer)
        QCoreApplication::postEvent(m_observer, new ProcessStartedEvent);
}

void ProcessCreator::throwProcessException(const QString &message) const
{
    postProcessStartedEvent();
    throw ProcessException(message);
}

const QTemporaryDir &ProcessCreator::temporaryDirectory() const
{
    return *m_temporaryDirectory.get();
}

void ProcessCreator::resetTemporaryDirectory()
{
    m_temporaryDirectory = std::make_unique<Utils::TemporaryDirectory>(m_temporaryDirectoryPattern);
}

QProcessEnvironment ProcessCreator::processEnvironment() const
{
    auto processEnvironment = QProcessEnvironment::systemEnvironment();

    if (temporaryDirectory().isValid()) {
        const QString temporaryDirectoryPath = temporaryDirectory().path();
        processEnvironment.insert("TMPDIR", temporaryDirectoryPath);
        processEnvironment.insert("TMP", temporaryDirectoryPath);
        processEnvironment.insert("TEMP", temporaryDirectoryPath);
    }

    const Utils::Environment &env = m_environment;
    for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
        if (env.isEnabled(it))
            processEnvironment.insert(env.key(it), env.expandedValueForKey(env.key(it)));
    }

    return processEnvironment;
}

} // namespace ClangBackEnd
