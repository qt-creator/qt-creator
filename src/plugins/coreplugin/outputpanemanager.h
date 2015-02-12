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

#ifndef OUTPUTPANEMANAGER_H
#define OUTPUTPANEMANAGER_H

#include <coreplugin/id.h>

#include <QMap>
#include <QToolButton>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QLabel;
class QSplitter;
class QStackedWidget;
class QTimeLine;
class QLabel;
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
    void init();
    static OutputPaneManager *instance();
    QWidget *buttonsWidget();
    void updateStatusButtons(bool visible);

    bool isMaximized()const;

public slots:
    void slotHide();
    void slotNext();
    void slotPrev();
    void shortcutTriggered();
    void slotMinMax();

protected:
    void focusInEvent(QFocusEvent *e);
    void resizeEvent(QResizeEvent *e);

private slots:
    void showPage(int flags);
    void togglePage(int flags);
    void clearPage();
    void buttonTriggered();
    void updateNavigateState();
    void popupMenu();
    void saveSettings() const;
    void flashButton();
    void setBadgeNumber(int number);

private:
    // the only class that is allowed to create and destroy
    friend class MainWindow;
    friend class OutputPaneManageButton;

    static void create();
    static void destroy();

    explicit OutputPaneManager(QWidget *parent = 0);
    ~OutputPaneManager();

    void showPage(int idx, int flags);
    void ensurePageVisible(int idx);
    int findIndexForPage(IOutputPane *out);
    int currentIndex() const;
    void setCurrentIndex(int idx);
    void buttonTriggered(int idx);
    void readSettings();

    QLabel *m_titleLabel;
    OutputPaneManageButton *m_manageButton;
    QAction *m_clearAction;
    QToolButton *m_clearButton;
    QToolButton *m_closeButton;

    QAction *m_minMaxAction;
    QToolButton *m_minMaxButton;

    QAction *m_nextAction;
    QAction *m_prevAction;
    QToolButton *m_prevToolButton;
    QToolButton *m_nextToolButton;
    QWidget *m_toolBar;

    QList<IOutputPane *> m_panes;
    QVector<OutputPaneToggleButton *> m_buttons;
    QVector<QAction *> m_actions;
    QVector<Id> m_ids;
    QMap<Id, bool> m_buttonVisibility;

    QStackedWidget *m_outputWidgetPane;
    QStackedWidget *m_opToolBarWidgets;
    QWidget *m_buttonsWidget;
    QIcon m_minimizeIcon;
    QIcon m_maximizeIcon;
    bool m_maximised;
    int m_outputPaneHeight;
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
                           QWidget *parent = 0);
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
    void flash(int count = 3);
    void setIconBadgeNumber(int number);

private slots:
    void updateToolTip();

private:
    void checkStateSet();

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
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
};

} // namespace Internal
} // namespace Core

#endif // OUTPUTPANEMANAGER_H
