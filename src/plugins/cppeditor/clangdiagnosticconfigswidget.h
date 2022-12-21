// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "clangdiagnosticconfig.h"

#include <QHash>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTabWidget;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class InfoLabel; }

namespace CppEditor {

class ClangDiagnosticConfig;
class ClangBaseChecksWidget;
class ConfigsModel;

class CPPEDITOR_EXPORT ClangDiagnosticConfigsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigsWidget(const ClangDiagnosticConfigs &configs,
                                          const Utils::Id &configToSelect,
                                          QWidget *parent = nullptr);
    ~ClangDiagnosticConfigsWidget() override;

    void sync();

    ClangDiagnosticConfigs configs() const;
    const ClangDiagnosticConfig currentConfig() const;

protected:
    void updateConfig(const ClangDiagnosticConfig &config);
    virtual void syncExtraWidgets(const ClangDiagnosticConfig &) {}
    QTabWidget *tabWidget() const;

private:
    void onCopyButtonClicked();
    void onRenameButtonClicked();
    void onRemoveButtonClicked();

    void onClangOnlyOptionsChanged();

    void setDiagnosticOptions(const QString &options);
    void updateValidityWidgets(const QString &errorMessage);

    void connectClangOnlyOptionsChanged();
    void disconnectClangOnlyOptionsChanged();

private:
    ConfigsModel *m_configsModel = nullptr;
    QHash<Utils::Id, QString> m_notAcceptedOptions;

    ClangBaseChecksWidget *m_clangBaseChecks = nullptr;
    QTreeView *m_configsView;
    Utils::InfoLabel *m_infoLabel;
    QTabWidget *m_tabWidget;
    QPushButton *m_renameButton;
    QPushButton *m_removeButton;
};

} // CppEditor namespace
