// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "executableinfo.h"

#include <cppeditor/clangdiagnosticconfigswidget.h>

#include <memory>

namespace ClangTools {
namespace Internal {
class Diagnostic;

// Not UI-related, but requires the tree model (or else a huge refactoring or code duplication).
QString removeClangTidyCheck(const QString &checks, const QString &check);
QString removeClazyCheck(const QString &checks, const QString &check);
void disableChecks(const QList<Diagnostic> &diagnostics);

namespace Ui {
class ClazyChecks;
class TidyChecks;
}

class TidyChecksTreeModel;
class ClazyChecksTreeModel;
class ClazyChecksSortFilterModel;

// Like CppEditor::ClangDiagnosticConfigsWidget, but with tabs/widgets for clang-tidy and clazy
class DiagnosticConfigsWidget : public CppEditor::ClangDiagnosticConfigsWidget
{
    Q_OBJECT

public:
    DiagnosticConfigsWidget(const CppEditor::ClangDiagnosticConfigs &configs,
                            const Utils::Id &configToSelect,
                            const ClangTidyInfo &tidyInfo,
                            const ClazyStandaloneInfo &clazyInfo);
    ~DiagnosticConfigsWidget();

private:
    void syncExtraWidgets(const CppEditor::ClangDiagnosticConfig &config) override;

    void syncClangTidyWidgets(const CppEditor::ClangDiagnosticConfig &config);
    void syncTidyChecksToTree(const CppEditor::ClangDiagnosticConfig &config);

    void syncClazyWidgets(const CppEditor::ClangDiagnosticConfig &config);
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
