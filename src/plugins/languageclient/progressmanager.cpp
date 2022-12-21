// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressmanager.h"

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <languageserverprotocol/progresssupport.h>

#include <QTime>

using namespace LanguageServerProtocol;

namespace LanguageClient {

static Q_LOGGING_CATEGORY(LOGPROGRESS, "qtc.languageclient.progress", QtWarningMsg);

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
    if (auto begin = std::get_if<WorkDoneProgressBegin>(&value))
        beginProgress(token, *begin);
    else if (auto report = std::get_if<WorkDoneProgressReport>(&value))
        reportProgress(token, *report);
    else if (auto end = std::get_if<WorkDoneProgressEnd>(&value))
        endProgress(token, *end);
}

void ProgressManager::setTitleForToken(const LanguageServerProtocol::ProgressToken &token,
                                         const QString &message)
{
    m_titles.insert(token, message);
}

void ProgressManager::setClickHandlerForToken(const LanguageServerProtocol::ProgressToken &token,
                                              const std::function<void()> &handler)
{
    m_clickHandlers.insert(token, handler);
}

void ProgressManager::reset()
{
    const QList<ProgressToken> &tokens = m_progress.keys();
    for (const ProgressToken &token : tokens)
        endProgress(token);
}

bool ProgressManager::isProgressEndMessage(const LanguageServerProtocol::ProgressParams &params)
{
    return std::holds_alternative<WorkDoneProgressEnd>(params.value());
}

Utils::Id languageClientProgressId(const ProgressToken &token)
{
    constexpr char k_LanguageClientProgressId[] = "LanguageClient.ProgressId.";
    auto toString = [](const ProgressToken &token){
        if (std::holds_alternative<int>(token))
            return QString::number(std::get<int>(token));
        return std::get<QString>(token);
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
    progress->setCancelEnabled(false);
    const std::function<void()> clickHandler = m_clickHandlers.value(token);
    if (clickHandler)
        QObject::connect(progress, &Core::FutureProgress::clicked, clickHandler);
    m_progress[token] = {progress, interface};
    if (LOGPROGRESS().isDebugEnabled())
        m_timer[token].start();
    reportProgress(token, begin);
}

void ProgressManager::reportProgress(const ProgressToken &token,
                                     const WorkDoneProgressReport &report)
{
    const LanguageClientProgress &progress = m_progress.value(token);
    if (progress.progressInterface) {
        const std::optional<QString> &message = report.message();
        if (message.has_value()) {
            progress.progressInterface->setSubtitle(*message);
            const bool showSubtitle = !message->isEmpty();
            progress.progressInterface->setSubtitleVisibleInStatusBar(showSubtitle);
        }
    }
    if (progress.futureInterface) {
        if (const std::optional<double> &percentage = report.percentage(); percentage.has_value())
            progress.futureInterface->setProgressValue(*percentage);
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
        auto timer = m_timer.take(token);
        if (timer.isValid()) {
            qCDebug(LOGPROGRESS) << QString("%1 took %2")
                                        .arg(progress.progressInterface->title())
                                        .arg(QTime::fromMSecsSinceStartOfDay(timer.elapsed())
                                                 .toString(Qt::ISODateWithMs));
        }
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
