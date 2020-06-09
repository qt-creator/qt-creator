/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "projectexplorer/ioutputparser.h"
#include "utils/outputformatter.h"
#include <array>
#include <QObject>
#include <QRegularExpression>

namespace MesonProjectManager {
namespace Internal {
class MesonOutputParser final : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT
    const QRegularExpression m_errorFileLocRegex{R"((^.*meson.build):(\d+):(\d+): ERROR)"};
    const QRegularExpression m_errorOptionRegex{R"!(ERROR: Value)!"};
    int m_remainingLines = 0;
    QStringList m_pending;
    void pushLine(const QString &line);
    Result processErrors(const QString &line);
    Result processWarnings(const QString &line);
    void addTask(ProjectExplorer::Task::TaskType type, const QString &line);
    void addTask(ProjectExplorer::Task task);
    Utils::OutputLineParser::LinkSpecs addTask(ProjectExplorer::Task::TaskType type,
                                               const QString &line,
                                               const QRegularExpressionMatch &match,
                                               int fileCapIndex,
                                               int lineNumberCapIndex);

public:
    MesonOutputParser();
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void readStdo(const QByteArray &data);
    void setSourceDirectory(const Utils::FilePath &sourceDir);
};

} // namespace Internal
} // namespace MesonProjectManager
