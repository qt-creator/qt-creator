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

#include "clangfileinfo.h"

#include <projectexplorer/runconfiguration.h>
#include <cpptools/projectinfo.h>

namespace Debugger { class DetailedErrorView; }

namespace ClangTools {
namespace Internal {

class ClangToolsDiagnosticModel;
class Diagnostic;

class ClangTool : public QObject
{
    Q_OBJECT

public:
    ClangTool(const QString &name);
    virtual ~ClangTool();

    virtual void startTool(bool askUserForFileSelection) = 0;

    virtual QList<Diagnostic> read(const QString &filePath,
                                   const QString &logFilePath,
                                   QString *errorMessage) const = 0;

    FileInfos collectFileInfos(ProjectExplorer::Project *project,
                               bool askUserForFileSelection) const;

    // For testing.
    QList<Diagnostic> diagnostics() const;

    const QString &name() const;

    void onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics);

signals:
    void finished(bool success); // For testing.

protected:
    virtual void handleStateUpdate() = 0;

    void setToolBusy(bool busy);
    void initDiagnosticView();

    ClangToolsDiagnosticModel *m_diagnosticModel = nullptr;
    QPointer<Debugger::DetailedErrorView> m_diagnosticView;

    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    bool m_running = false;
    bool m_toolBusy = false;

private:
    const QString m_name;
};

} // namespace Internal
} // namespace ClangTools
