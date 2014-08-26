/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "fancymainwindow.h"

#include "qtcassert.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>

static const char stateKeyC[] = "State";
static const int settingsVersion = 2;
static const char dockWidgetActiveState[] = "DockWidgetActiveState";

namespace Utils {

// Stolen from QDockWidgetTitleButton
class DockWidgetTitleButton : public QAbstractButton
{
public:
    DockWidgetTitleButton(QWidget *parent)
        : QAbstractButton(parent)
    {
        setFocusPolicy(Qt::NoFocus);
    }

    QSize sizeHint() const
    {
        ensurePolished();

        int size = 2*style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, 0, this);
        if (!icon().isNull()) {
            int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
            QSize sz = icon().actualSize(QSize(iconSize, iconSize));
            size += qMax(sz.width(), sz.height());
        }

        return QSize(size, size);
    }

    QSize minimumSizeHint() const { return sizeHint(); }

    void enterEvent(QEvent *event)
    {
        if (isEnabled()) update();
        QAbstractButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event)
    {
        if (isEnabled()) update();
        QAbstractButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event);
};

void DockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QStyleOptionToolButton opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;
    opt.icon = icon();
    opt.subControls = 0;
    opt.activeSubControls = 0;
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    opt.iconSize = QSize(size, size);
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
}


class TitleBarWidget : public QWidget
{
public:
    TitleBarWidget(QDockWidget *parent, const QStyleOptionDockWidget &opt)
      : QWidget(parent), q(parent), m_active(true)
    {
        m_titleLabel = new QLabel(this);

        m_floatButton = new DockWidgetTitleButton(this);
        m_floatButton->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarNormalButton, &opt, q));
        m_floatButton->setAccessibleName(QDockWidget::tr("Float"));
        m_floatButton->setAccessibleDescription(QDockWidget::tr("Undocks and re-attaches the dock widget"));

        m_closeButton = new DockWidgetTitleButton(this);
        m_closeButton->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, q));
        m_closeButton->setAccessibleName(QDockWidget::tr("Close"));
        m_closeButton->setAccessibleDescription(QDockWidget::tr("Closes the dock widget"));

        setActive(false);

        const int minWidth = 10;
        const int maxWidth = 10000;
        const int inactiveHeight = 0;
        const int activeHeight = m_closeButton->sizeHint().height() + 2;

        m_minimumInactiveSize = QSize(minWidth, inactiveHeight);
        m_maximumInactiveSize = QSize(maxWidth, inactiveHeight);
        m_minimumActiveSize   = QSize(minWidth, activeHeight);
        m_maximumActiveSize   = QSize(maxWidth, activeHeight);

        auto layout = new QHBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->setContentsMargins(4, 0, 0, 0);
        layout->addWidget(m_titleLabel);
        layout->addStretch();
        layout->addWidget(m_floatButton);
        layout->addWidget(m_closeButton);
        setLayout(layout);
    }

    void leaveEvent(QEvent *event)
    {
        if (!q->isFloating())
            setActive(false);
        QWidget::leaveEvent(event);
    }

    void setActive(bool on)
    {
        if (m_active == on)
            return;
        m_active = on;
        m_titleLabel->setVisible(on);
        m_floatButton->setVisible(on);
        m_closeButton->setVisible(on);
        update();
    }

    QSize sizeHint() const
    {
        ensurePolished();
        return m_active ? m_maximumActiveSize : m_maximumInactiveSize;
    }

    QSize minimumSizeHint() const
    {
        ensurePolished();
        return m_active ? m_minimumActiveSize : m_minimumInactiveSize;
    }

public:
    QDockWidget *q;
    bool m_active;
    QSize m_minimumActiveSize;
    QSize m_maximumActiveSize;
    QSize m_minimumInactiveSize;
    QSize m_maximumInactiveSize;

    QLabel *m_titleLabel;
    DockWidgetTitleButton *m_floatButton;
    DockWidgetTitleButton *m_closeButton;
};


class DockWidget : public QDockWidget
{
public:
    DockWidget(QWidget *inner, QWidget *parent)
        : QDockWidget(parent), m_inner(inner)
    {
        setWidget(inner);
        setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
        setObjectName(inner->objectName() + QLatin1String("DockWidget"));
        setWindowTitle(inner->windowTitle());
        setMouseTracking(true);

        QStyleOptionDockWidget opt;
        initStyleOption(&opt);
        m_titleBar = new TitleBarWidget(this, opt);
        m_titleBar->m_titleLabel->setText(inner->windowTitle());
        setTitleBarWidget(m_titleBar);

        m_timer.setSingleShot(true);
        m_timer.setInterval(500);

        connect(&m_timer, &QTimer::timeout, this, &DockWidget::handleMouseTimeout);

        connect(this, &QDockWidget::topLevelChanged, this, &DockWidget::handleToplevelChanged);

        connect(toggleViewAction(), &QAction::triggered,
                [this]() {
                    if (isVisible())
                        raise();
                });

        auto origFloatButton = findChild<QAbstractButton *>(QLatin1String("qt_dockwidget_floatbutton"));
        connect(m_titleBar->m_floatButton, &QAbstractButton::clicked,
                origFloatButton, &QAbstractButton::clicked);

        auto origCloseButton = findChild<QAbstractButton *>(QLatin1String("qt_dockwidget_closebutton"));
        connect(m_titleBar->m_closeButton, &QAbstractButton::clicked,
                origCloseButton, &QAbstractButton::clicked);
    }

    bool eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            int y = me->pos().y();
            int x = me->pos().x();
            int h = m_titleBar->m_floatButton->height();
            if (!isFloating() && 0 <= x && x < m_inner->width() && 0 <= y && y <= h) {
                m_timer.start();
                m_startPos = mapToGlobal(me->pos());
            }
        }
        return false;
    }

    void enterEvent(QEvent *event)
    {
        QApplication::instance()->installEventFilter(this);
        QDockWidget::leaveEvent(event);
    }

    void leaveEvent(QEvent *event)
    {
        QApplication::instance()->removeEventFilter(this);
        QDockWidget::leaveEvent(event);
    }

    void handleMouseTimeout()
    {
        QPoint dist = m_startPos - QCursor::pos();
        if (!isFloating() && dist.manhattanLength() < 4) {
            m_titleBar->setActive(true);
        }
    }

    void handleToplevelChanged(bool floating)
    {
        if (!floating)
            m_titleBar->setActive(false);
    }

private:
    QPoint m_startPos;
    QWidget *m_inner;
    TitleBarWidget *m_titleBar;
    QTimer m_timer;
};

/*! \class Utils::FancyMainWindow

    \brief The FancyMainWindow class is a MainWindow with dock widgets and
    additional "lock" functionality
    (locking the dock widgets in place) and "reset layout" functionality.

    The dock actions and the additional actions should be accessible
    in a Window-menu.
*/

class FancyMainWindowPrivate
{
public:
    FancyMainWindowPrivate();

    bool m_handleDockVisibilityChanges;

    QAction m_menuSeparator;
    QAction m_resetLayoutAction;
    QDockWidget *m_toolBarDockWidget;
};

FancyMainWindowPrivate::FancyMainWindowPrivate() :
    m_handleDockVisibilityChanges(true),
    m_menuSeparator(0),
    m_resetLayoutAction(FancyMainWindow::tr("Reset to Default Layout"), 0),
    m_toolBarDockWidget(0)
{
    m_menuSeparator.setSeparator(true);
}

FancyMainWindow::FancyMainWindow(QWidget *parent) :
    QMainWindow(parent), d(new FancyMainWindowPrivate)
{
    connect(&d->m_resetLayoutAction, &QAction::triggered,
            this, &FancyMainWindow::resetLayout);
}

FancyMainWindow::~FancyMainWindow()
{
    delete d;
}

QDockWidget *FancyMainWindow::addDockForWidget(QWidget *widget)
{
    QTC_ASSERT(widget, return 0);
    QTC_CHECK(widget->objectName().size());
    QTC_CHECK(widget->windowTitle().size());

    auto dockWidget = new DockWidget(widget, this);

    connect(dockWidget, &QDockWidget::visibilityChanged,
        [this, dockWidget](bool visible) {
            if (d->m_handleDockVisibilityChanges)
                dockWidget->setProperty(dockWidgetActiveState, visible);
        });


    dockWidget->setProperty(dockWidgetActiveState, true);

    return dockWidget;
}

void FancyMainWindow::setTrackingEnabled(bool enabled)
{
    if (enabled) {
        d->m_handleDockVisibilityChanges = true;
        foreach (QDockWidget *dockWidget, dockWidgets())
            dockWidget->setProperty(dockWidgetActiveState, dockWidget->isVisible());
    } else {
        d->m_handleDockVisibilityChanges = false;
    }
}

void FancyMainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(false);
}

void FancyMainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(true);
}

void FancyMainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    addDockActionsToMenu(&menu);
    menu.exec(event->globalPos());
}

void FancyMainWindow::handleVisibilityChanged(bool visible)
{
    d->m_handleDockVisibilityChanges = false;
    foreach (QDockWidget *dockWidget, dockWidgets()) {
        if (dockWidget->isFloating()) {
            dockWidget->setVisible(visible
                && dockWidget->property(dockWidgetActiveState).toBool());
        }
    }
    if (visible)
        d->m_handleDockVisibilityChanges = true;
}

void FancyMainWindow::saveSettings(QSettings *settings) const
{
    QHash<QString, QVariant> hash = saveSettings();
    QHashIterator<QString, QVariant> it(hash);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
}

void FancyMainWindow::restoreSettings(const QSettings *settings)
{
    QHash<QString, QVariant> hash;
    foreach (const QString &key, settings->childKeys()) {
        hash.insert(key, settings->value(key));
    }
    restoreSettings(hash);
}

QHash<QString, QVariant> FancyMainWindow::saveSettings() const
{
    QHash<QString, QVariant> settings;
    settings.insert(QLatin1String(stateKeyC), saveState(settingsVersion));
    foreach (QDockWidget *dockWidget, dockWidgets()) {
        settings.insert(dockWidget->objectName(),
                dockWidget->property(dockWidgetActiveState));
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<QString, QVariant> &settings)
{
    QByteArray ba = settings.value(QLatin1String(stateKeyC), QByteArray()).toByteArray();
    if (!ba.isEmpty())
        restoreState(ba, settingsVersion);
    foreach (QDockWidget *widget, dockWidgets()) {
        widget->setProperty(dockWidgetActiveState,
            settings.value(widget->objectName(), false));
    }
}

QList<QDockWidget *> FancyMainWindow::dockWidgets() const
{
    return findChildren<QDockWidget *>();
}

static bool actionLessThan(const QAction *action1, const QAction *action2)
{
    QTC_ASSERT(action1, return true);
    QTC_ASSERT(action2, return false);
    return action1->text().toLower() < action2->text().toLower();
}

void FancyMainWindow::addDockActionsToMenu(QMenu *menu)
{
    QList<QAction *> actions;
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    for (int i = 0; i < dockwidgets.size(); ++i) {
        QDockWidget *dockWidget = dockwidgets.at(i);
        if (dockWidget->property("managed_dockwidget").isNull()
                && dockWidget->parentWidget() == this) {
            actions.append(dockwidgets.at(i)->toggleViewAction());
        }
    }
    qSort(actions.begin(), actions.end(), actionLessThan);
    foreach (QAction *action, actions)
        menu->addAction(action);
    menu->addAction(&d->m_menuSeparator);
    menu->addAction(&d->m_resetLayoutAction);
}

QAction *FancyMainWindow::menuSeparator() const
{
    return &d->m_menuSeparator;
}

QAction *FancyMainWindow::resetLayoutAction() const
{
    return &d->m_resetLayoutAction;
}

void FancyMainWindow::setDockActionsVisible(bool v)
{
    foreach (const QDockWidget *dockWidget, dockWidgets())
        dockWidget->toggleViewAction()->setVisible(v);
    d->m_menuSeparator.setVisible(v);
    d->m_resetLayoutAction.setVisible(v);
}

QDockWidget *FancyMainWindow::toolBarDockWidget() const
{
    return d->m_toolBarDockWidget;
}

void FancyMainWindow::setToolBarDockWidget(QDockWidget *dock)
{
    d->m_toolBarDockWidget = dock;
}

} // namespace Utils
