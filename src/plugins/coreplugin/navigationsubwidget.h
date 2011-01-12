/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef NAVIGATIONSSUBWIDGET_H
#define NAVIGATIONSSUBWIDGET_H

#include <QtGui/QComboBox>

#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class StyledBar;
}

namespace Core {
class INavigationWidgetFactory;
class IMode;
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

    Core::Command *command(const QString &title) const;

signals:
    void splitMe();
    void closeMe();

private slots:
    void comboBoxIndexChanged(int);

private:
    NavigationWidget *m_parentWidget;
    QComboBox *m_navigationComboBox;
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
    virtual const Core::Command *command(const QString &text) const = 0;
};


class NavComboBox : public CommandComboBox
{
    Q_OBJECT

public:
    explicit NavComboBox(NavigationSubWidget *navSubWidget) :
        m_navSubWidget(navSubWidget) {}

private:
    virtual const Core::Command *command(const QString &text) const
        { return m_navSubWidget->command(text); }

    NavigationSubWidget *m_navSubWidget;
};

} // namespace Internal
} // namespace Core

#endif // NAVIGATIONSSUBWIDGET_H
