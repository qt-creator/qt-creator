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

#ifndef NAVIGATIONSUBWIDGET_H
#define NAVIGATIONSUBWIDGET_H

#include <QComboBox>

#include <QList>

QT_BEGIN_NAMESPACE
class QMenu;
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class StyledBar; }

namespace Core {
class INavigationWidgetFactory;
class Command;
class NavigationWidget;

namespace Internal {

class NavigationSubWidget : public QWidget
{
    Q_OBJECT
public:
    NavigationSubWidget(NavigationWidget *parentWidget, int position, int index);
    virtual ~NavigationSubWidget();

    INavigationWidgetFactory *factory();

    int factoryIndex() const;
    void setFactoryIndex(int i);

    void setFocusWidget();

    int position() const;
    void setPosition(int i);

    void saveSettings();
    void restoreSettings();

    Command *command(const QString &title) const;
    void setCloseIcon(const QIcon &icon);

    QWidget *widget();

signals:
    void splitMe(int factoryIndex);
    void closeMe();

private slots:
    void comboBoxIndexChanged(int);

private:
    void populateSplitMenu();

    NavigationWidget *m_parentWidget;
    QComboBox *m_navigationComboBox;
    QMenu *m_splitMenu;
    QToolButton *m_closeButton;
    QWidget *m_navigationWidget;
    INavigationWidgetFactory *m_navigationWidgetFactory;
    Utils::StyledBar *m_toolBar;
    QList<QToolButton *> m_additionalToolBarWidgets;
    int m_position;
};

// A combo associated with a command. Shows the command text
// and shortcut in the tooltip.
class CommandComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit CommandComboBox(QWidget *parent = 0);

protected:
    bool event(QEvent *event);

private:
    virtual const Command *command(const QString &text) const = 0;
};


class NavComboBox : public CommandComboBox
{
    Q_OBJECT

public:
    explicit NavComboBox(NavigationSubWidget *navSubWidget) :
        m_navSubWidget(navSubWidget) {}

private:
    virtual const Command *command(const QString &text) const
        { return m_navSubWidget->command(text); }

    NavigationSubWidget *m_navSubWidget;
};

} // namespace Internal
} // namespace Core

#endif // NAVIGATIONSUBWIDGET_H
