// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fancymainwindow.h"

#include "algorithm.h"
#include "qtcassert.h"
#include "qtcsettings.h"
#include "stringutils.h"
#include "utilstr.h"

#include <QAbstractButton>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>

static const char AutoHideTitleBarsKey[] = "AutoHideTitleBars";
static const char ShowCentralWidgetKey[] = "ShowCentralWidget";
static const char StateKey[] = "State";

static const int settingsVersion = 2;
static const char dockWidgetActiveState[] = "DockWidgetActiveState";

namespace Utils {

class TitleBarWidget;

struct FancyMainWindowPrivate
{
    FancyMainWindowPrivate(FancyMainWindow *parent);

    FancyMainWindow *q;

    bool m_handleDockVisibilityChanges;
    QAction m_showCentralWidget;
    QAction m_menuSeparator1;
    QAction m_menuSeparator2;
    QAction m_resetLayoutAction;
    QAction m_autoHideTitleBars;
};

class DockWidget : public QDockWidget
{
public:
    DockWidget(QWidget *inner, FancyMainWindow *parent, bool immutable = false);

    bool eventFilter(QObject *, QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void handleMouseTimeout();
    void handleToplevelChanged(bool floating);

    FancyMainWindow *q;

private:
    QPoint m_startPos;
    TitleBarWidget *m_titleBar;
    QTimer m_timer;
    bool m_immutable = false;
};

// Stolen from QDockWidgetTitleButton
class DockWidgetTitleButton : public QAbstractButton
{
public:
    DockWidgetTitleButton(QWidget *parent)
        : QAbstractButton(parent)
    {
        setFocusPolicy(Qt::NoFocus);
    }

    QSize sizeHint() const override
    {
        ensurePolished();

        int size = 2*style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, nullptr, this);
        if (!icon().isNull()) {
            int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
            QSize sz = icon().actualSize(QSize(iconSize, iconSize));
            size += qMax(sz.width(), sz.height());
        }

        return QSize(size, size);
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

    void enterEvent(QEnterEvent *event) override
    {
        if (isEnabled())
            update();
        QAbstractButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (isEnabled())
            update();
        QAbstractButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event) override;
};

void DockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QStyleOptionToolButton opt;
    opt.initFrom(this);
    opt.state |= QStyle::State_AutoRaise;
    opt.icon = icon();
    opt.subControls = {};
    opt.activeSubControls = {};
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
    opt.iconSize = QSize(size, size);
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
}

class TitleBarWidget : public QWidget
{
public:
    TitleBarWidget(DockWidget *parent, const QStyleOptionDockWidget &opt)
      : QWidget(parent), q(parent), m_active(true)
    {
        m_titleLabel = new QLabel(this);

        m_floatButton = new DockWidgetTitleButton(this);
        m_floatButton->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarNormalButton, &opt, q));

        m_closeButton = new DockWidgetTitleButton(this);
        m_closeButton->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, q));

#ifndef QT_NO_ACCESSIBILITY
        m_floatButton->setAccessibleName(QDockWidget::tr("Float"));
        m_floatButton->setAccessibleDescription(QDockWidget::tr("Undocks and re-attaches the dock widget"));
        m_closeButton->setAccessibleName(QDockWidget::tr("Close"));
        m_closeButton->setAccessibleDescription(QDockWidget::tr("Closes the dock widget"));
#endif

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
        layout->setSpacing(0);
        layout->setContentsMargins(4, 0, 0, 0);
        layout->addWidget(m_titleLabel);
        layout->addStretch();
        layout->addWidget(m_floatButton);
        layout->addWidget(m_closeButton);
        setLayout(layout);

        setProperty("managed_titlebar", 1);

        connect(parent, &QDockWidget::featuresChanged, this, [this, parent] {
            m_closeButton->setVisible(parent->features().testFlag(QDockWidget::DockWidgetClosable));
            m_floatButton->setVisible(parent->features().testFlag(QDockWidget::DockWidgetFloatable));
        });
    }

    void enterEvent(QEnterEvent *event) override
    {
        setActive(true);
        QWidget::enterEvent(event);
    }

    void setActive(bool on)
    {
        m_active = on;
        updateChildren();
    }

    void updateChildren()
    {
        bool clickable = isClickable();
        m_titleLabel->setVisible(clickable);

        m_floatButton->setVisible(clickable
                                  && q->features().testFlag(QDockWidget::DockWidgetFloatable));
        m_closeButton->setVisible(clickable
                                  && q->features().testFlag(QDockWidget::DockWidgetClosable));
    }

    bool isClickable() const
    {
        return m_active || !q->q->autoHideTitleBars();
    }

    QSize sizeHint() const override
    {
        ensurePolished();
        return isClickable() ? m_maximumActiveSize : m_maximumInactiveSize;
    }

    QSize minimumSizeHint() const override
    {
        ensurePolished();
        return isClickable() ? m_minimumActiveSize : m_minimumInactiveSize;
    }

private:
    DockWidget *q;
    bool m_active;
    QSize m_minimumActiveSize;
    QSize m_maximumActiveSize;
    QSize m_minimumInactiveSize;
    QSize m_maximumInactiveSize;

public:
    QLabel *m_titleLabel;
    DockWidgetTitleButton *m_floatButton;
    DockWidgetTitleButton *m_closeButton;
};

DockWidget::DockWidget(QWidget *inner, FancyMainWindow *parent, bool immutable)
    : QDockWidget(parent), q(parent), m_immutable(immutable)
{
    setWidget(inner);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    setObjectName(inner->objectName() + QLatin1String("DockWidget"));
    setMouseTracking(true);

    QString title = inner->windowTitle();
    toggleViewAction()->setProperty("original_title", title);
    title = stripAccelerator(title);
    setWindowTitle(title);

    QStyleOptionDockWidget opt;
    initStyleOption(&opt);
    m_titleBar = new TitleBarWidget(this, opt);
    m_titleBar->m_titleLabel->setText(title);
    setTitleBarWidget(m_titleBar);

    if (immutable)
        return;

    m_timer.setSingleShot(true);
    m_timer.setInterval(500);

    connect(&m_timer, &QTimer::timeout, this, &DockWidget::handleMouseTimeout);
    connect(this, &QDockWidget::topLevelChanged, this, &DockWidget::handleToplevelChanged);
    connect(toggleViewAction(), &QAction::triggered, this, [this] {
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

bool DockWidget::eventFilter(QObject *, QEvent *event)
{
    if (!m_immutable && event->type() == QEvent::MouseMove && q->autoHideTitleBars()) {
        auto me = static_cast<QMouseEvent *>(event);
        int y = me->pos().y();
        int x = me->pos().x();
        int h = qMin(8, m_titleBar->m_floatButton->height());
        if (!isFloating() && widget() && 0 <= x && x < widget()->width() && 0 <= y && y <= h) {
            m_timer.start();
            m_startPos = mapToGlobal(me->pos());
        }
    }
    return false;
}

void DockWidget::enterEvent(QEnterEvent *event)
{
    if (!m_immutable)
        QApplication::instance()->installEventFilter(this);
    QDockWidget::enterEvent(event);
}

void DockWidget::leaveEvent(QEvent *event)
{
    if (!m_immutable) {
        if (!isFloating()) {
            m_timer.stop();
            m_titleBar->setActive(false);
        }
        QApplication::instance()->removeEventFilter(this);
    }
    QDockWidget::leaveEvent(event);
}

void DockWidget::handleMouseTimeout()
{
    QPoint dist = m_startPos - QCursor::pos();
    if (!isFloating() && dist.manhattanLength() < 4)
        m_titleBar->setActive(true);
}

void DockWidget::handleToplevelChanged(bool floating)
{
    m_titleBar->setActive(floating);
}



/*!
    \class Utils::FancyMainWindow
    \inmodule QtCreator

    \brief The FancyMainWindow class is a MainWindow with dock widgets and
    additional "lock" functionality
    (locking the dock widgets in place) and "reset layout" functionality.

    The dock actions and the additional actions should be accessible
    in a Window-menu.
*/

FancyMainWindowPrivate::FancyMainWindowPrivate(FancyMainWindow *parent) :
    q(parent),
    m_handleDockVisibilityChanges(true),
    m_showCentralWidget(Tr::tr("Central Widget"), nullptr),
    m_menuSeparator1(nullptr),
    m_menuSeparator2(nullptr),
    m_resetLayoutAction(Tr::tr("Reset to Default Layout"), nullptr),
    m_autoHideTitleBars(Tr::tr("Automatically Hide View Title Bars"), nullptr)
{
    m_showCentralWidget.setCheckable(true);
    m_showCentralWidget.setChecked(true);

    m_menuSeparator1.setSeparator(true);
    m_menuSeparator2.setSeparator(true);

    m_autoHideTitleBars.setCheckable(true);
    m_autoHideTitleBars.setChecked(true);

    QObject::connect(&m_autoHideTitleBars, &QAction::toggled, q, [this](bool) {
        for (QDockWidget *dock : q->dockWidgets()) {
            if (auto titleBar = dynamic_cast<TitleBarWidget *>(dock->titleBarWidget()))
                titleBar->updateChildren();
        }
    });

    QObject::connect(&m_showCentralWidget, &QAction::toggled, q, [this](bool visible) {
        if (q->centralWidget())
            q->centralWidget()->setVisible(visible);
    });
}

FancyMainWindow::FancyMainWindow(QWidget *parent) :
    QMainWindow(parent), d(new FancyMainWindowPrivate(this))
{
    connect(&d->m_resetLayoutAction, &QAction::triggered,
            this, &FancyMainWindow::resetLayout);
}

FancyMainWindow::~FancyMainWindow()
{
    delete d;
}

QDockWidget *FancyMainWindow::addDockForWidget(QWidget *widget, bool immutable)
{
    QTC_ASSERT(widget, return nullptr);
    QTC_CHECK(widget->objectName().size());
    QTC_CHECK(widget->windowTitle().size());

    auto dockWidget = new DockWidget(widget, this, immutable);

    if (!immutable) {
        connect(dockWidget, &QDockWidget::visibilityChanged, this,
                [this, dockWidget](bool visible) {
            if (d->m_handleDockVisibilityChanges)
                dockWidget->setProperty(dockWidgetActiveState, visible);
        });

        connect(dockWidget->toggleViewAction(), &QAction::triggered, dockWidget, [dockWidget] {
            if (dockWidget->isVisible())
                dockWidget->raise();
        }, Qt::QueuedConnection);

        dockWidget->setProperty(dockWidgetActiveState, true);
    }

    return dockWidget;
}

void FancyMainWindow::setTrackingEnabled(bool enabled)
{
    if (enabled) {
        d->m_handleDockVisibilityChanges = true;
        for (QDockWidget *dockWidget : dockWidgets())
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
    for (QDockWidget *dockWidget : dockWidgets()) {
        if (dockWidget->isFloating()) {
            dockWidget->setVisible(visible
                && dockWidget->property(dockWidgetActiveState).toBool());
        }
    }
    if (visible)
        d->m_handleDockVisibilityChanges = true;
}

void FancyMainWindow::saveSettings(QtcSettings *settings) const
{
    const QHash<Key, QVariant> hash = saveSettings();
    for (auto it = hash.cbegin(), end = hash.cend(); it != end; ++it)
        settings->setValue(it.key(), it.value());
}

void FancyMainWindow::restoreSettings(const QtcSettings *settings)
{
    QHash<Key, QVariant> hash;
    const KeyList childKeys = settings->childKeys();
    for (const Key &key : childKeys)
        hash.insert(key, settings->value(key));
    restoreSettings(hash);
}

QHash<Key, QVariant> FancyMainWindow::saveSettings() const
{
    QHash<Key, QVariant> settings;
    settings.insert(StateKey, saveState(settingsVersion));
    settings.insert(AutoHideTitleBarsKey, d->m_autoHideTitleBars.isChecked());
    settings.insert(ShowCentralWidgetKey, d->m_showCentralWidget.isChecked());
    for (QDockWidget *dockWidget : dockWidgets()) {
        settings.insert(keyFromString(dockWidget->objectName()),
                dockWidget->property(dockWidgetActiveState));
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<Key, QVariant> &settings)
{
    QByteArray ba = settings.value(StateKey, QByteArray()).toByteArray();
    if (!ba.isEmpty()) {
        if (!restoreState(ba, settingsVersion))
            qWarning() << "Restoring the state of dock widgets failed.";
    }
    bool on = settings.value(AutoHideTitleBarsKey, true).toBool();
    d->m_autoHideTitleBars.setChecked(on);
    d->m_showCentralWidget.setChecked(settings.value(ShowCentralWidgetKey, true).toBool());
    for (QDockWidget *widget : dockWidgets()) {
        widget->setProperty(dockWidgetActiveState,
                            settings.value(keyFromString(widget->objectName()), false));
    }
}

static void findDockChildren(QWidget *parent, QList<QDockWidget *> &result)
{
    for (QObject *child : parent->children()) {
        QWidget *childWidget = qobject_cast<QWidget *>(child);
        if (!childWidget)
            continue;

        if (auto dockWidget = qobject_cast<QDockWidget *>(child))
            result.append(dockWidget);
        else if (!qobject_cast<QMainWindow *>(child))
            findDockChildren(qobject_cast<QWidget *>(child), result);
    }
}

const QList<QDockWidget *> FancyMainWindow::dockWidgets() const
{
    QList<QDockWidget *> result;
    findDockChildren((QWidget *) this, result);
    return result;
}

bool FancyMainWindow::autoHideTitleBars() const
{
    return d->m_autoHideTitleBars.isChecked();
}

void FancyMainWindow::setAutoHideTitleBars(bool on)
{
    d->m_autoHideTitleBars.setChecked(on);
}

bool FancyMainWindow::isCentralWidgetShown() const
{
    return d->m_showCentralWidget.isChecked();
}

void FancyMainWindow::showCentralWidget(bool on)
{
    d->m_showCentralWidget.setChecked(on);
}

void FancyMainWindow::addDockActionsToMenu(QMenu *menu)
{
    QList<QAction *> actions;
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    for (int i = 0; i < dockwidgets.size(); ++i) {
        QDockWidget *dockWidget = dockwidgets.at(i);
        if (dockWidget->property("managed_dockwidget").isNull()
                && dockWidget->parentWidget() == this) {
            QAction *action = dockWidget->toggleViewAction();
            action->setText(action->property("original_title").toString());
            actions.append(action);
        }
    }
    sort(actions, [](const QAction *action1, const QAction *action2) {
        QTC_ASSERT(action1, return true);
        QTC_ASSERT(action2, return false);
        return stripAccelerator(action1->text()).toLower() < stripAccelerator(action2->text()).toLower();
    });
    for (QAction *action : std::as_const(actions))
        menu->addAction(action);
    menu->addAction(&d->m_showCentralWidget);
    menu->addAction(&d->m_menuSeparator1);
    menu->addAction(&d->m_autoHideTitleBars);
    menu->addAction(&d->m_menuSeparator2);
    menu->addAction(&d->m_resetLayoutAction);
}

QAction *FancyMainWindow::menuSeparator1() const
{
    return &d->m_menuSeparator1;
}

QAction *FancyMainWindow::autoHideTitleBarsAction() const
{
    return &d->m_autoHideTitleBars;
}

QAction *FancyMainWindow::menuSeparator2() const
{
    return &d->m_menuSeparator2;
}

QAction *FancyMainWindow::resetLayoutAction() const
{
    return &d->m_resetLayoutAction;
}

QAction *FancyMainWindow::showCentralWidgetAction() const
{
    return &d->m_showCentralWidget;
}

void FancyMainWindow::setDockActionsVisible(bool v)
{
    for (const QDockWidget *dockWidget : dockWidgets())
        dockWidget->toggleViewAction()->setVisible(v);
    d->m_showCentralWidget.setVisible(v);
    d->m_autoHideTitleBars.setVisible(v);
    d->m_menuSeparator1.setVisible(v);
    d->m_menuSeparator2.setVisible(v);
    d->m_resetLayoutAction.setVisible(v);
}

} // namespace Utils
