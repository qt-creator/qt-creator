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
