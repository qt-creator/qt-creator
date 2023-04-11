// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "executableinfo.h"

#include <cppeditor/clangdiagnosticconfigswidget.h>

#include <memory>

namespace ClangTools::Internal {

class ClazyChecksSortFilterModel;
class ClazyChecksTreeModel;
class ClazyChecksWidget;
class Diagnostic;
class TidyChecksTreeModel;
class TidyChecksWidget;

void disableChecks(const QList<Diagnostic> &diagnostics);

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

    void onClangTidyTreeChanged();
    void onClazyTreeChanged();

    void connectClangTidyItemChanged();
    void disconnectClangTidyItemChanged();

    void connectClazyItemChanged();
    void disconnectClazyItemChanged();

private:
    // Clang-Tidy
    TidyChecksWidget *m_tidyChecks = nullptr;
    std::unique_ptr<TidyChecksTreeModel> m_tidyTreeModel;
    ClangTidyInfo m_tidyInfo;

    // Clazy
    ClazyChecksWidget *m_clazyChecks = nullptr;
    ClazyChecksSortFilterModel *m_clazySortFilterProxyModel = nullptr;
    std::unique_ptr<ClazyChecksTreeModel> m_clazyTreeModel;
    ClazyStandaloneInfo m_clazyInfo;
};

} // ClangTools::Internal
