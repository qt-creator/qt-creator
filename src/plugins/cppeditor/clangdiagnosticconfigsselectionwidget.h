// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "clangdiagnosticconfigsmodel.h"

#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QPushButton;
QT_END_NAMESPACE

namespace CppEditor {

class ClangDiagnosticConfigsWidget;

class CPPEDITOR_EXPORT ClangDiagnosticConfigsSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigsSelectionWidget(QWidget *parent = nullptr);
    explicit ClangDiagnosticConfigsSelectionWidget(QFormLayout *parentLayout);

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
    QString label() const;
    void setUpUi(bool withLabel);
    void onButtonClicked();

    ClangDiagnosticConfigsModel m_diagnosticConfigsModel;
    Utils::Id m_currentConfigId;
    bool m_showTidyClazyUi = true;

    QPushButton *m_button = nullptr;

    CreateEditWidget m_createEditWidget;
};

} // CppEditor namespace
