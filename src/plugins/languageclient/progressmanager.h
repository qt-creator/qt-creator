// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <coreplugin/progressmanager/futureprogress.h>

#include <QElapsedTimer>
#include <QFutureInterface>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace LanguageServerProtocol {
class ProgressParams;
class ProgressToken;
class WorkDoneProgressBegin;
class WorkDoneProgressReport;
class WorkDoneProgressEnd;
} // namespace LanguageServerProtocol

namespace LanguageClient {

class LANGUAGECLIENT_EXPORT ProgressManager
{
public:
    ProgressManager();
    ~ProgressManager();
    void handleProgress(const LanguageServerProtocol::ProgressParams &params);
    void setTitleForToken(const LanguageServerProtocol::ProgressToken &token,
                          const QString &message);
    void setClickHandlerForToken(const LanguageServerProtocol::ProgressToken &token,
                                 const std::function<void()> &handler);
    void setCancelHandlerForToken(const LanguageServerProtocol::ProgressToken &token,
                                  const std::function<void()> &handler);
    void endProgressReport(const LanguageServerProtocol::ProgressToken &token);

    void reset();

    static bool isProgressEndMessage(const LanguageServerProtocol::ProgressParams &params);

private:
    void beginProgress(const LanguageServerProtocol::ProgressToken &token,
                       const LanguageServerProtocol::WorkDoneProgressBegin &begin);
    void reportProgress(const LanguageServerProtocol::ProgressToken &token,
                        const LanguageServerProtocol::WorkDoneProgressReport &report);
    void endProgress(const LanguageServerProtocol::ProgressToken &token,
                     const LanguageServerProtocol::WorkDoneProgressEnd &end);
    void spawnProgressBar(const LanguageServerProtocol::ProgressToken &token);

    struct ProgressItem
    {
        QPointer<Core::FutureProgress> progressInterface = nullptr;
        QFutureInterface<void> *futureInterface = nullptr;
        QElapsedTimer timer;
        QTimer *showBarTimer = nullptr;
        QString message;
        QString title;
    };

    QMap<LanguageServerProtocol::ProgressToken, ProgressItem> m_progress;
    QMap<LanguageServerProtocol::ProgressToken, QString> m_titles;
    QMap<LanguageServerProtocol::ProgressToken, std::function<void()>> m_clickHandlers;
    QMap<LanguageServerProtocol::ProgressToken, std::function<void()>> m_cancelHandlers;
};

} // namespace LanguageClient
