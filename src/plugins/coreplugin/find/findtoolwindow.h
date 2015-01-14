/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FINDTOOLWINDOW_H
#define FINDTOOLWINDOW_H

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
    explicit FindToolWindow(FindPlugin *plugin, QWidget *parent = 0);
    ~FindToolWindow();
    static FindToolWindow *instance();

    void setFindFilters(const QList<IFindFilter *> &filters);

    void setFindText(const QString &text);
    void setCurrentFilter(IFindFilter *filter);
    void readSettings();
    void writeSettings();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void search();
    void replace();
    void setCurrentFilter(int index);
    void updateButtonStates();
    void updateFindFlags();

private:
    void acceptAndGetParameters(QString *term, IFindFilter **filter);

    Ui::FindDialog m_ui;
    FindPlugin *m_plugin;
    QList<IFindFilter *> m_filters;
    QCompleter *m_findCompleter;
    QWidgetList m_configWidgets;
    IFindFilter *m_currentFilter;
    QWidget *m_configWidget;
};

} // namespace Internal
} // namespace Core

#endif // FINDTOOLWINDOW_H
