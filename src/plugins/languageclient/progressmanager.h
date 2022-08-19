// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/progressmanager/futureprogress.h>

#include <QFutureInterface>
#include <QPointer>

namespace LanguageServerProtocol {
class ProgressParams;
class ProgressToken;
class WorkDoneProgressBegin;
class WorkDoneProgressReport;
class WorkDoneProgressEnd;
} // namespace LanguageServerProtocol

namespace LanguageClient {

class ProgressManager
{
public:
    ProgressManager();
    ~ProgressManager();
    void handleProgress(const LanguageServerProtocol::ProgressParams &params);
    void setTitleForToken(const LanguageServerProtocol::ProgressToken &token,
                          const QString &message);
    void reset();

    static bool isProgressEndMessage(const LanguageServerProtocol::ProgressParams &params);

private:
    void beginProgress(const LanguageServerProtocol::ProgressToken &token,
                       const LanguageServerProtocol::WorkDoneProgressBegin &begin);
    void reportProgress(const LanguageServerProtocol::ProgressToken &token,
                        const LanguageServerProtocol::WorkDoneProgressReport &report);
    void endProgress(const LanguageServerProtocol::ProgressToken &token,
                     const LanguageServerProtocol::WorkDoneProgressEnd &end);
    void endProgress(const LanguageServerProtocol::ProgressToken &token);

    struct LanguageClientProgress {
        QPointer<Core::FutureProgress> progressInterface = nullptr;
        QFutureInterface<void> *futureInterface = nullptr;
    };

    QMap<LanguageServerProtocol::ProgressToken, LanguageClientProgress> m_progress;
    QMap<LanguageServerProtocol::ProgressToken, QString> m_titles;
};

} // namespace LanguageClient
