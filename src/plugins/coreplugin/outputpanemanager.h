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

#include <coreplugin/id.h>

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QStackedWidget;
class QTimeLine;
QT_END_NAMESPACE

namespace Core {

class IOutputPane;

namespace Internal {

class MainWindow;
class OutputPaneToggleButton;
class OutputPaneManageButton;

class OutputPaneManager : public QWidget
{
    Q_OBJECT

public:
    static OutputPaneManager *instance();
    void updateStatusButtons(bool visible);
    static void updateMaximizeButton(bool maximized);

    static int outputPaneHeightSetting();
    static void setOutputPaneHeightSetting(int value);

public slots:
    void slotHide();
    void slotNext();
    void slotPrev();
    void toggleMaximized();

protected:
    void focusInEvent(QFocusEvent *e) override;

private:
    // the only class that is allowed to create and destroy
    friend class MainWindow;
    friend class OutputPaneManageButton;

    static void create();
    static void destroy();

    explicit OutputPaneManager(QWidget *parent = nullptr);
    ~OutputPaneManager() override;

    void shortcutTriggered(int idx);
    void clearPage();
    void popupMenu();
    void saveSettings() const;
    void showPage(int idx, int flags);
    void ensurePageVisible(int idx);
    int currentIndex() const;
    void setCurrentIndex(int idx);
    void buttonTriggered(int idx);
    void readSettings();

    QLabel *m_titleLabel = nullptr;
    OutputPaneManageButton *m_manageButton = nullptr;
    QAction *m_clearAction = nullptr;
    QToolButton *m_clearButton = nullptr;
    QToolButton *m_closeButton = nullptr;

    QAction *m_minMaxAction = nullptr;
    QToolButton *m_minMaxButton = nullptr;

    QAction *m_nextAction = nullptr;
    QAction *m_prevAction = nullptr;
    QToolButton *m_prevToolButton = nullptr;
    QToolButton *m_nextToolButton = nullptr;
    QWidget *m_toolBar = nullptr;

    QStackedWidget *m_outputWidgetPane = nullptr;
    QStackedWidget *m_opToolBarWidgets = nullptr;
    QWidget *m_buttonsWidget = nullptr;
    QIcon m_minimizeIcon;
    QIcon m_maximizeIcon;
    int m_outputPaneHeightSetting = 0;
};

class BadgeLabel
{
public:
    BadgeLabel();
    void paint(QPainter *p, int x, int y, bool isChecked);
    void setText(const QString &text);
    QString text() const;
    QSize sizeHint() const;

private:
    void calculateSize();

    QSize m_size;
    QString m_text;
    QFont m_font;
    static const int m_padding = 6;
};

class OutputPaneToggleButton : public QToolButton
{
    Q_OBJECT
public:
    OutputPaneToggleButton(int number, const QString &text, QAction *action,
                           QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent*) override;
    void flash(int count = 3);
    void setIconBadgeNumber(int number);

private:
    void updateToolTip();
    void checkStateSet() override;

    QString m_number;
    QString m_text;
    QAction *m_action;
    QTimeLine *m_flashTimer;
    BadgeLabel m_badgeNumberLabel;
};

class OutputPaneManageButton : public QToolButton
{
    Q_OBJECT
public:
    OutputPaneManageButton();
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent*) override;
};

} // namespace Internal
} // namespace Core
