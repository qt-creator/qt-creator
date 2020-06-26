/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "executableinfo.h"

#include <cpptools/clangdiagnosticconfigswidget.h>

#include <memory>

namespace ClangTools {
namespace Internal {

namespace Ui {
class ClazyChecks;
class TidyChecks;
}

class TidyChecksTreeModel;
class ClazyChecksTreeModel;
class ClazyChecksSortFilterModel;

// Like CppTools::ClangDiagnosticConfigsWidget, but with tabs/widgets for clang-tidy and clazy
class DiagnosticConfigsWidget : public CppTools::ClangDiagnosticConfigsWidget
{
    Q_OBJECT

public:
    DiagnosticConfigsWidget(const CppTools::ClangDiagnosticConfigs &configs,
                            const Utils::Id &configToSelect,
                            const ClangTidyInfo &tidyInfo,
                            const ClazyStandaloneInfo &clazyInfo);
    ~DiagnosticConfigsWidget();

private:
    void syncExtraWidgets(const CppTools::ClangDiagnosticConfig &config) override;

    void syncClangTidyWidgets(const CppTools::ClangDiagnosticConfig &config);
    void syncTidyChecksToTree(const CppTools::ClangDiagnosticConfig &config);

    void syncClazyWidgets(const CppTools::ClangDiagnosticConfig &config);
    void syncClazyChecksGroupBox();

    void onClangTidyModeChanged(int index);
    void onClangTidyTreeChanged();
    void onClazyTreeChanged();

    void connectClangTidyItemChanged();
    void disconnectClangTidyItemChanged();

    void connectClazyItemChanged();
    void disconnectClazyItemChanged();

private:
    // Clang-Tidy
    std::unique_ptr<Ui::TidyChecks> m_tidyChecks;
    QWidget *m_tidyChecksWidget = nullptr;
    std::unique_ptr<TidyChecksTreeModel> m_tidyTreeModel;
    ClangTidyInfo m_tidyInfo;

    // Clazy
    std::unique_ptr<Ui::ClazyChecks> m_clazyChecks;
    QWidget *m_clazyChecksWidget = nullptr;
    std::unique_ptr<ClazyChecksTreeModel> m_clazyTreeModel;
    ClazyChecksSortFilterModel *m_clazySortFilterProxyModel = nullptr;
    ClazyStandaloneInfo m_clazyInfo;
};

} // namespace Internal
} // namespace ClangTools
