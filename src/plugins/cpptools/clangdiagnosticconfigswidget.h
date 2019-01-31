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

#include "cpptools_global.h"

#include "clangdiagnosticconfig.h"
#include "clangdiagnosticconfigsmodel.h"

#include <QHash>
#include <QWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
class QPushButton;
QT_END_NAMESPACE

namespace CppTools {

namespace Ui {
class ClangDiagnosticConfigsWidget;
class ClangBaseChecks;
class ClazyChecks;
class TidyChecks;
}

class TidyChecksTreeModel;
class ClazyChecksTreeModel;
class ClazyChecksSortFilterModel;

class CPPTOOLS_EXPORT ClangDiagnosticConfigsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigsWidget(const Core::Id &configToSelect, QWidget *parent = nullptr);
    ~ClangDiagnosticConfigsWidget() override;

    ClangDiagnosticConfigs customConfigs() const;

signals:
    void customConfigsChanged(const CppTools::ClangDiagnosticConfigs &customConfigs);

private:
    void setupTabs();

    void onCurrentConfigChanged(int index);
    void onCopyButtonClicked();
    void onRemoveButtonClicked();
    void onClangTidyModeChanged(int index);
    void onClangTidyTreeChanged();
    void onClazyTreeChanged();
    void onClangTidyTreeItemClicked(const QModelIndex &index);

    void onClangOnlyOptionsChanged();

    void syncWidgetsToModel(const Core::Id &configToSelect = Core::Id());
    void syncConfigChooserToModel(const Core::Id &configToSelect = Core::Id());
    void syncOtherWidgetsToComboBox();
    void syncClangTidyWidgets(const ClangDiagnosticConfig &config);
    void syncClazyWidgets(const ClangDiagnosticConfig &config);
    void syncClazyChecksGroupBox();
    void syncTidyChecksToTree(const ClangDiagnosticConfig &config);

    void updateConfig(const CppTools::ClangDiagnosticConfig &config);

    bool isConfigChooserEmpty() const;
    const ClangDiagnosticConfig &selectedConfig() const;
    Core::Id selectedConfigId() const;

    void setDiagnosticOptions(const QString &options);
    void updateValidityWidgets(const QString &errorMessage);

    void connectClangTidyItemChanged();
    void disconnectClangTidyItemChanged();

    void connectClazyItemChanged();
    void disconnectClazyItemChanged();

    void connectConfigChooserCurrentIndex();
    void disconnectConfigChooserCurrentIndex();
    void connectClangOnlyOptionsChanged();
    void disconnectClangOnlyOptionsChanged();

private:
    Ui::ClangDiagnosticConfigsWidget *m_ui;
    ClangDiagnosticConfigsModel m_diagnosticConfigsModel;
    QHash<Core::Id, QString> m_notAcceptedOptions;

    std::unique_ptr<CppTools::Ui::ClangBaseChecks> m_clangBaseChecks;
    QWidget *m_clangBaseChecksWidget = nullptr;

    std::unique_ptr<CppTools::Ui::ClazyChecks> m_clazyChecks;
    QWidget *m_clazyChecksWidget = nullptr;
    std::unique_ptr<ClazyChecksTreeModel> m_clazyTreeModel;
    ClazyChecksSortFilterModel *m_clazySortFilterProxyModel = nullptr;

    std::unique_ptr<CppTools::Ui::TidyChecks> m_tidyChecks;
    QWidget *m_tidyChecksWidget = nullptr;
    std::unique_ptr<TidyChecksTreeModel> m_tidyTreeModel;

    int m_selectedConfigIndex = 0;
};

} // CppTools namespace
