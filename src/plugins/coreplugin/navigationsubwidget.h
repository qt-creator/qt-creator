// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
    NavigationSubWidget(NavigationWidget *parentWidget, int position, int factoryIndex);
    ~NavigationSubWidget() override;

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
    void factoryIndexChanged(int factoryIndex);

private:
    void comboBoxIndexChanged(int);
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
    explicit CommandComboBox(QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;

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
    const Command *command(const QString &text) const override
        { return m_navSubWidget->command(text); }

    NavigationSubWidget *m_navSubWidget;
};

} // namespace Internal
} // namespace Core
