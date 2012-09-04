/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QTSUPPORT_QTKITCONFIGWIDGET_H
#define QTSUPPORT_QTKITCONFIGWIDGET_H

#include <projectexplorer/kitconfigwidget.h>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace ProjectExplorer { class Kit; }

namespace QtSupport {
class BaseQtVersion;

namespace Internal {

class QtKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    explicit QtKitConfigWidget(ProjectExplorer::Kit *k, QWidget *parent = 0);

    QString displayName() const;

    void makeReadOnly();

    void apply();
    void discard();
    bool isDirty() const;
    QWidget *buttonWidget() const;

private slots:
    void versionsChanged(const QList<int> &added, const QList<int> &removed, const QList<int> &changed);
    void kitUpdated(ProjectExplorer::Kit *k);
    void manageQtVersions();

private:
    int findQtVersion(const int id) const;

    ProjectExplorer::Kit *m_kit;
    QComboBox *m_combo;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace Debugger

#endif // QTSUPPORT_QTYSTEMCONFIGWIDGET_H
