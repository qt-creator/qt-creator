/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KITMANAGERWIDGET_H
#define KITMANAGERWIDGET_H

#include "kitconfigwidget.h"

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QGridLayout;
class QLineEdit;
class QToolButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;

namespace Internal {

class KitManagerConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    explicit KitManagerConfigWidget(Kit *k, QWidget *parent = 0);

    QString displayName() const;

    void apply();
    void discard();
    bool isDirty() const;
    void addConfigWidget(ProjectExplorer::KitConfigWidget *widget);
    void makeReadOnly();

private slots:
    void setIcon();

private:
    enum LayoutColumns {
        LabelColumn,
        WidgetColumn,
        ButtonColumn
    };

    void addToLayout(const QString &name, const QString &toolTip, QWidget *widget, QWidget *button = 0);


    void addLabel(const QString &name, const QString &toolTip, int row);
    void addButtonWidget(QWidget *button, const QString &toolTip, int row);

    QGridLayout *m_layout;
    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QList<KitConfigWidget *> m_widgets;
    Kit *m_kit;
    QString m_iconPath;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITMANAGERWIDGET_H
