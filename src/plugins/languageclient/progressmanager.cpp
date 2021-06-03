/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "progressmanager.h"

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <languageserverprotocol/progresssupport.h>

using namespace LanguageServerProtocol;

namespace LanguageClient {

ProgressManager::ProgressManager()
{}

ProgressManager::~ProgressManager()
{
    reset();
}

void ProgressManager::handleProgress(const LanguageServerProtocol::ProgressParams &params)
{
    const ProgressToken &token = params.token();
    ProgressParams::ProgressType value = params.value();
    if (auto begin = Utils::get_if<WorkDoneProgressBegin>(&value))
        beginProgress(token, *begin);
    else if (auto report = Utils::get_if<WorkDoneProgressReport>(&value))
        reportProgress(token, *report);
    else if (auto end = Utils::get_if<WorkDoneProgressEnd>(&value))
        endProgress(token, *end);
}

void ProgressManager::setTitleForToken(const LanguageServerProtocol::ProgressToken &token,
                                         const QString &message)
{
    m_titles.insert(token, message);
}

void ProgressManager::reset()
{
    const QList<ProgressToken> &tokens = m_progress.keys();
    for (const ProgressToken &token : tokens)
        endProgress(token);
}

bool ProgressManager::isProgressEndMessage(const LanguageServerProtocol::ProgressParams &params)
{
    return Utils::holds_alternative<WorkDoneProgressEnd>(params.value());
}

Utils::Id languageClientProgressId(const ProgressToken &token)
{
    constexpr char k_LanguageClientProgressId[] = "LanguageClient.ProgressId.";
    auto toString = [](const ProgressToken &token){
        if (Utils::holds_alternative<int>(token))
            return QString::number(Utils::get<int>(token));
        return Utils::get<QString>(token);
    };
    return Utils::Id(k_LanguageClientProgressId).withSuffix(toString(token));
}

void ProgressManager::beginProgress(const ProgressToken &token, const WorkDoneProgressBegin &begin)
{
    auto interface = new QFutureInterface<void>();
    interface->reportStarted();
    interface->setProgressRange(0, 100); // LSP always reports percentage of the task
    const QString title = m_titles.value(token, begin.title());
    Core::FutureProgress *progress = Core::ProgressManager::addTask(
            interface->future(), title, languageClientProgressId(token));
    m_progress[token] = {progress, interface};
    reportProgress(token, begin);
}

void ProgressManager::reportProgress(const ProgressToken &token,
                                     const WorkDoneProgressReport &report)
{
    const LanguageClientProgress &progress = m_progress.value(token);
    if (progress.progressInterface) {
        const Utils::optional<QString> &message = report.message();
        if (message.has_value()) {
            progress.progressInterface->setSubtitle(*message);
            const bool showSubtitle = !message->isEmpty();
            progress.progressInterface->setSubtitleVisibleInStatusBar(showSubtitle);
        }
    }
    if (progress.futureInterface) {
        const Utils::optional<int> &progressValue = report.percentage();
        if (progressValue.has_value())
            progress.futureInterface->setProgressValue(*progressValue);
    }
}

void ProgressManager::endProgress(const ProgressToken &token, const WorkDoneProgressEnd &end)
{
    const LanguageClientProgress &progress = m_progress.value(token);
    const QString &message = end.message().value_or(QString());
    if (progress.progressInterface) {
        if (!message.isEmpty()) {
            progress.progressInterface->setKeepOnFinish(
                Core::FutureProgress::KeepOnFinishTillUserInteraction);
        }
        progress.progressInterface->setSubtitle(message);
        progress.progressInterface->setSubtitleVisibleInStatusBar(!message.isEmpty());
    }
    endProgress(token);
}

void ProgressManager::endProgress(const ProgressToken &token)
{
    const LanguageClientProgress &progress = m_progress.take(token);
    if (progress.futureInterface)
        progress.futureInterface->reportFinished();
    delete progress.futureInterface;
}

} // namespace LanguageClient
