// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "outputpane.h"
#include "warningmodel.h"

QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)

namespace ScxmlEditor {

namespace OutputPane {

class TableView;

class ErrorWidget : public OutputPane
{
    Q_OBJECT

public:
    explicit ErrorWidget(QWidget *parent = nullptr);
    ~ErrorWidget() override;

    WarningModel *warningModel() const
    {
        return m_warningModel;
    }

    QString title() const override
    {
        return m_title;
    }

    QIcon icon() const override
    {
        return m_icon;
    }

    QColor alertColor() const override;
    void setPaneFocus() override;

signals:
    void mouseExited();
    void warningEntered(Warning *w);
    void warningSelected(Warning *w);
    void warningDoubleClicked(Warning *w);

private:
    void createUi();
    void updateWarnings();
    void exportWarnings();
    void warningCountChanged(int c);
    QString modifyExportedValue(const QString &val);

    QIcon m_icon;
    QString m_title;
    WarningModel *m_warningModel;
    QSortFilterProxyModel *m_proxyModel;

    TableView *m_errorsTable = nullptr;
    QToolButton *m_clean = nullptr;
    QToolButton *m_exportWarnings = nullptr;
    QToolButton *m_showErrors = nullptr;
    QToolButton *m_showWarnings = nullptr;
    QToolButton *m_showInfos = nullptr;
};

} // namespace OutputPane
} // namespace ScxmlEditor
