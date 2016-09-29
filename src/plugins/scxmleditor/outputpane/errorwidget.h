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
