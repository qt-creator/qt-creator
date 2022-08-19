// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_finddialog.h"
#include "findplugin.h"

#include <QList>

QT_FORWARD_DECLARE_CLASS(QCompleter)

namespace Core {
class IFindFilter;

namespace Internal {

class FindToolWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FindToolWindow(QWidget *parent = nullptr);
    ~FindToolWindow() override;
    static FindToolWindow *instance();

    void setFindFilters(const QList<IFindFilter *> &filters);
    QList<IFindFilter *> findFilters() const;

    void setFindText(const QString &text);
    void setCurrentFilter(IFindFilter *filter);
    void readSettings();
    void writeSettings();

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void search();
    void replace();
    void setCurrentFilter(int index);
    void updateButtonStates();
    void updateFindFlags();
    void updateFindFilterName(IFindFilter *filter);
    static void findCompleterActivated(const QModelIndex &index);

    void acceptAndGetParameters(QString *term, IFindFilter **filter);

    Ui::FindDialog m_ui;
    QList<IFindFilter *> m_filters;
    QCompleter *m_findCompleter;
    QWidgetList m_configWidgets;
    IFindFilter *m_currentFilter;
    QWidget *m_configWidget;
};

} // namespace Internal
} // namespace Core
