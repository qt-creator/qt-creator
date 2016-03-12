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

#include <QWidget>

namespace CppTools {

namespace Ui { class ClangDiagnosticConfigsWidget; }

class CPPTOOLS_EXPORT ClangDiagnosticConfigsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigsWidget(
            const ClangDiagnosticConfigsModel &diagnosticConfigsModel = ClangDiagnosticConfigsModel(),
            const Core::Id &configToSelect = Core::Id(),
            QWidget *parent = 0);
    ~ClangDiagnosticConfigsWidget();

    Core::Id currentConfigId() const;
    ClangDiagnosticConfigs customConfigs() const;

    void setConfigWithUndecoratedDisplayName(const Core::Id &id);
    void refresh(const ClangDiagnosticConfigsModel &diagnosticConfigsModel,
                 const Core::Id &configToSelect);

signals:
    void currentConfigChanged(const Core::Id &currentConfigId);
    void customConfigsChanged(const CppTools::ClangDiagnosticConfigs &customConfigs);

private:
    void onCurrentConfigChanged(int);
    void onCopyButtonClicked();
    void onRemoveButtonClicked();

    void onDiagnosticOptionsEdited();

    void syncWidgetsToModel(const Core::Id &configToSelect = Core::Id());
    void syncConfigChooserToModel(const Core::Id &configToSelect = Core::Id());
    void syncOtherWidgetsToComboBox();

    bool isConfigChooserEmpty() const;
    const ClangDiagnosticConfig &currentConfig() const;

    void setDiagnosticOptions(const QString &options);

    void connectConfigChooserCurrentIndex();
    void disconnectConfigChooserCurrentIndex();
    void connectDiagnosticOptionsChanged();
    void disconnectDiagnosticOptionsChanged();

private:
    Ui::ClangDiagnosticConfigsWidget *m_ui;
    ClangDiagnosticConfigsModel m_diagnosticConfigsModel;
    Core::Id m_configWithUndecoratedDisplayName;
};

} // CppTools namespace
