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

#include <QHash>
#include <QWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

namespace CppTools {

class ClangDiagnosticConfig;

namespace Ui {
class ClangDiagnosticConfigsWidget;
class ClangBaseChecks;
}

class ConfigsModel;

class CPPTOOLS_EXPORT ClangDiagnosticConfigsWidget : public QWidget
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
    void updateConfig(const CppTools::ClangDiagnosticConfig &config);
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
    Ui::ClangDiagnosticConfigsWidget *m_ui;
    ConfigsModel *m_configsModel = nullptr;
    QHash<Utils::Id, QString> m_notAcceptedOptions;

    std::unique_ptr<CppTools::Ui::ClangBaseChecks> m_clangBaseChecks;
    QWidget *m_clangBaseChecksWidget = nullptr;
};

} // CppTools namespace
