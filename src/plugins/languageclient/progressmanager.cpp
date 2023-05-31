// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressmanager.h"

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <languageserverprotocol/progresssupport.h>

#include <QTime>
#include <QTimer>

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

void ProgressManager::setCancelHandlerForToken(const LanguageServerProtocol::ProgressToken &token,
                                               const std::function<void ()> &handler)
{
    m_cancelHandlers.insert(token, handler);
}

void ProgressManager::reset()
{
    const QList<ProgressToken> &tokens = m_progress.keys();
    for (const ProgressToken &token : tokens)
        endProgressReport(token);
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
    ProgressItem progressItem;
    progressItem.futureInterface = interface;
    progressItem.title = m_titles.value(token, begin.title());
    if (LOGPROGRESS().isDebugEnabled())
        progressItem.timer.start();
    progressItem.showBarTimer = new QTimer();
    progressItem.showBarTimer->setSingleShot(true);
    progressItem.showBarTimer->setInterval(750);
    progressItem.showBarTimer->callOnTimeout([this, token]() { spawnProgressBar(token); });
    progressItem.showBarTimer->start();
    m_progress[token] = progressItem;
    reportProgress(token, begin);
}

void ProgressManager::spawnProgressBar(const LanguageServerProtocol::ProgressToken &token)
{
    ProgressItem &progressItem = m_progress[token];
    QTC_ASSERT(progressItem.futureInterface, return);
    Core::FutureProgress *progress
        = Core::ProgressManager::addTask(progressItem.futureInterface->future(),
                                         progressItem.title,
                                         languageClientProgressId(token));
    const std::function<void()> clickHandler = m_clickHandlers.value(token);
    if (clickHandler)
        QObject::connect(progress, &Core::FutureProgress::clicked, clickHandler);
    const std::function<void()> cancelHandler = m_cancelHandlers.value(token);
    if (cancelHandler)
        QObject::connect(progress, &Core::FutureProgress::canceled, cancelHandler);
    else
        progress->setCancelEnabled(false);
    if (!progressItem.message.isEmpty()) {
        progress->setSubtitle(progressItem.message);
        progress->setSubtitleVisibleInStatusBar(true);
    }
    progressItem.progressInterface = progress;
}

void ProgressManager::reportProgress(const ProgressToken &token,
                                     const WorkDoneProgressReport &report)
{
    ProgressItem &progress = m_progress[token];
    const std::optional<QString> &message = report.message();
    if (progress.progressInterface) {
        if (message.has_value()) {
            progress.progressInterface->setSubtitle(*message);
            const bool showSubtitle = !message->isEmpty();
            progress.progressInterface->setSubtitleVisibleInStatusBar(showSubtitle);
        }
    } else if (message.has_value()) {
        progress.message = *message;
    }
    if (progress.futureInterface) {
        if (const std::optional<double> &percentage = report.percentage(); percentage.has_value())
            progress.futureInterface->setProgressValue(*percentage);
    }
}

void ProgressManager::endProgress(const ProgressToken &token, const WorkDoneProgressEnd &end)
{
    const ProgressItem &progress = m_progress.value(token);
    const QString &message = end.message().value_or(QString());
    if (progress.progressInterface) {
        if (!message.isEmpty()) {
            progress.progressInterface->setKeepOnFinish(
                Core::FutureProgress::KeepOnFinishTillUserInteraction);
        }
        progress.progressInterface->setSubtitle(message);
        progress.progressInterface->setSubtitleVisibleInStatusBar(!message.isEmpty());
        if (progress.timer.isValid()) {
            qCDebug(LOGPROGRESS) << QString("%1 took %2")
                                        .arg(progress.progressInterface->title())
                                        .arg(QTime::fromMSecsSinceStartOfDay(
                                                 progress.timer.elapsed())
                                                 .toString(Qt::ISODateWithMs));
        }
    }
    endProgressReport(token);
}

void ProgressManager::endProgressReport(const ProgressToken &token)
{
    ProgressItem progress = m_progress.take(token);
    delete progress.showBarTimer;
    if (progress.futureInterface)
        progress.futureInterface->reportFinished();
    delete progress.futureInterface;
}

} // namespace LanguageClient
