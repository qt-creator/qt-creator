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

#include "clangdiagnosticconfigsmodel.h"

#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace CppTools {

class ClangDiagnosticConfigsWidget;

class CPPTOOLS_EXPORT ClangDiagnosticConfigsSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigsSelectionWidget(QWidget *parent = nullptr);

    using CreateEditWidget
        = std::function<ClangDiagnosticConfigsWidget *(const ClangDiagnosticConfigs &configs,
                                                       const Utils::Id &configToSelect)>;

    void refresh(const ClangDiagnosticConfigsModel &model,
                 const Utils::Id &configToSelect,
                 const CreateEditWidget &createEditWidget);

    Utils::Id currentConfigId() const;
    ClangDiagnosticConfigs customConfigs() const;

signals:
    void changed();

private:
    void onButtonClicked();

    ClangDiagnosticConfigsModel m_diagnosticConfigsModel;
    Utils::Id m_currentConfigId;
    bool m_showTidyClazyUi = true;

    QLabel *m_label = nullptr;
    QPushButton *m_button = nullptr;

    CreateEditWidget m_createEditWidget;
};

} // CppTools namespace
