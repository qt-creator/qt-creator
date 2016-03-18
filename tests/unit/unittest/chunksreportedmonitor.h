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

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

#include <texteditor/semantichighlighter.h>

namespace ClangBackEnd {

class ChunksReportedMonitor : public QObject
{
    Q_OBJECT

public:
    ChunksReportedMonitor(const QFuture<TextEditor::HighlightingResult> &future);

    uint resultsReadyCounter();

private:
    bool waitUntilFinished(int timeoutInMs = 5000);
    void onResultsReadyAt(int beginIndex, int endIndex);

private:
    QFuture<TextEditor::HighlightingResult> m_future;
    QFutureWatcher<TextEditor::HighlightingResult> m_futureWatcher;
    uint m_resultsReadyCounter = 0;
};

} // namespace ClangBackEnd
