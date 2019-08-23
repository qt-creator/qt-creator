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

#include "clangtool.h"

#include <debugger/debuggermainwindow.h>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace ClangTools {
namespace Internal {

class DiagnosticFilterModel;

const char ClangTidyClazyPerspectiveId[] = "ClangTidyClazy.Perspective";

class ClangTidyClazyTool final : public ClangTool
{
    Q_OBJECT

public:
    ClangTidyClazyTool();

    static ClangTidyClazyTool *instance();

    void startTool(FileSelection fileSelection) final;

    Diagnostics read(OutputFileFormat outputFileFormat,
                     const QString &logFilePath,
                     const QString &mainFilePath,
                     const QSet<Utils::FilePath> &projectFiles,
                     QString *errorMessage) const final;

    void onNewDiagnosticsAvailable(const Diagnostics &diagnostics) override;

private:
    void handleStateUpdate() final;

    void updateRunActions();
    void loadDiagnosticsFromFiles();

    DiagnosticFilterModel *m_diagnosticFilterModel = nullptr;

    Utils::FancyLineEdit *m_filterLineEdit = nullptr;
    QToolButton *m_applyFixitsButton = nullptr;

    QAction *m_goBack = nullptr;
    QAction *m_goNext = nullptr;
    QAction *m_loadExported = nullptr;
    QAction *m_clear = nullptr;
    QAction *m_expandCollapse = nullptr;

    Utils::Perspective m_perspective{ClangTidyClazyPerspectiveId, tr("Clang-Tidy and Clazy")};
};

} // namespace Internal
} // namespace ClangTools
