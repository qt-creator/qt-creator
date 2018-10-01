/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include <QHash>
#include <QTimer>

namespace Utils {
class QtcProcess;
class FileName;
using FileNameList = QList<FileName>;
}

namespace Cppcheck {
namespace Internal {

class CppcheckTool;

class CppcheckRunner final : public QObject
{
    Q_OBJECT

public:
    explicit CppcheckRunner(CppcheckTool &tool);
    ~CppcheckRunner() override;

    void reconfigure(const QString &binary, const QString &arguments);
    void addToQueue(const Utils::FileNameList &files,
                    const QString &additionalArguments = {});
    void removeFromQueue(const Utils::FileNameList &files);
    void stop(const Utils::FileNameList &files = {});

    const Utils::FileNameList &currentFiles() const;
    QString currentCommand() const;

private:
    void checkQueued();
    void readOutput();
    void readError();
    void handleStarted();
    void handleFinished(int);

    CppcheckTool &m_tool;
    Utils::QtcProcess *m_process = nullptr;
    QString m_binary;
    QString m_arguments;
    QHash<QString, Utils::FileNameList> m_queue;
    Utils::FileNameList m_currentFiles;
    QTimer m_queueTimer;
    int m_maxArgumentsLength = 32767;
    bool m_isRunning = false;
};

} // namespace Internal
} // namespace Cppcheck
