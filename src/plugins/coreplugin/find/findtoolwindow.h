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
    explicit FindToolWindow(QWidget *parent = 0);
    ~FindToolWindow();
    static FindToolWindow *instance();

    void setFindFilters(const QList<IFindFilter *> &filters);
    QList<IFindFilter *> findFilters() const;

    void setFindText(const QString &text);
    void setCurrentFilter(IFindFilter *filter);
    void readSettings();
    void writeSettings();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void search();
    void replace();
    void setCurrentFilter(int index);
    void updateButtonStates();
    void updateFindFlags();
    void updateFindFilterName(IFindFilter *filter);

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
