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

#include "clangdiagnosticconfigsselectionwidget.h"

#include "cppcodemodelsettings.h"
#include "cpptoolsreuse.h"

namespace CppTools {

ClangDiagnosticConfigsSelectionWidget::ClangDiagnosticConfigsSelectionWidget(QWidget *parent)
    : QComboBox(parent)
{
    refresh(codeModelSettings()->clangDiagnosticConfigId());

    connectToCurrentIndexChanged();
}

Core::Id ClangDiagnosticConfigsSelectionWidget::currentConfigId() const
{
    return Core::Id::fromSetting(currentData());
}

void ClangDiagnosticConfigsSelectionWidget::connectToCurrentIndexChanged()
{
    m_currentIndexChangedConnection
            = connect(this,
                      static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                      this,
                      [this]() { emit currentConfigChanged(currentConfigId()); });
}

void ClangDiagnosticConfigsSelectionWidget::disconnectFromCurrentIndexChanged()
{
    disconnect(m_currentIndexChangedConnection);
}

void ClangDiagnosticConfigsSelectionWidget::refresh(Core::Id id)
{
    disconnectFromCurrentIndexChanged();

    int configToSelectIndex = -1;
    clear();
    m_diagnosticConfigsModel = ClangDiagnosticConfigsModel(
                codeModelSettings()->clangCustomDiagnosticConfigs());
    const int size = m_diagnosticConfigsModel.size();
    for (int i = 0; i < size; ++i) {
        const ClangDiagnosticConfig &config = m_diagnosticConfigsModel.at(i);
        const QString displayName
                = ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(config);
        addItem(displayName, config.id().toSetting());

        if (id == config.id())
            configToSelectIndex = i;
    }

    if (configToSelectIndex != -1)
        setCurrentIndex(configToSelectIndex);
    else
        emit currentConfigChanged(currentConfigId());

    connectToCurrentIndexChanged();
}

} // CppTools namespace
