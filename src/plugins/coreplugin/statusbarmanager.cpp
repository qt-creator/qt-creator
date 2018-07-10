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

#include "statusbarmanager.h"

#include "imode.h"
#include "mainwindow.h"
#include "minisplitter.h"
#include "modemanager.h"

#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QSplitter>
#include <QStatusBar>

namespace Core {

const char kSettingsGroup[] = "StatusBar";
const char kLeftSplitWidthKey[] = "LeftSplitWidth";

static QPointer<QSplitter> m_splitter;
static QList<QPointer<QWidget>> m_statusBarWidgets;
static QList<QPointer<IContext>> m_contexts;

/*!
    Context that always returns the context of the active's mode widget (if available).
*/
class StatusBarContext : public IContext
{
public:
    StatusBarContext(QObject *parent);

    Context context() const final;
};

static QWidget *createWidget(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    w->setLayout(new QHBoxLayout);
    w->setVisible(true);
    w->layout()->setMargin(0);
    return w;
}

static void createStatusBarManager()
{
    QStatusBar *bar = ICore::statusBar();

    m_splitter = new NonResizingSplitter(bar);
    bar->insertPermanentWidget(0, m_splitter, 10);
    m_splitter->setChildrenCollapsible(false);
    // first
    QWidget *w = createWidget(m_splitter);
    w->layout()->setContentsMargins(0, 0, 3, 0);
    m_splitter->addWidget(w);
    m_statusBarWidgets.append(w);

    QWidget *w2 = createWidget(m_splitter);
    w2->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_splitter->addWidget(w2);
    // second
    w = createWidget(w2);
    w2->layout()->addWidget(w);
    m_statusBarWidgets.append(w);
    // third
    w = createWidget(w2);
    w2->layout()->addWidget(w);
    m_statusBarWidgets.append(w);

    static_cast<QBoxLayout *>(w2->layout())->addStretch(1);

    QWidget *rightCornerWidget = createWidget(bar);
    bar->insertPermanentWidget(1, rightCornerWidget);
    m_statusBarWidgets.append(rightCornerWidget);

    auto context = new StatusBarContext(bar);
    context->setWidget(bar);
    ICore::addContextObject(context);

    QObject::connect(ICore::instance(), &ICore::saveSettingsRequested, [] {
        QSettings *s = ICore::settings();
        s->beginGroup(QLatin1String(kSettingsGroup));
        s->setValue(QLatin1String(kLeftSplitWidthKey), m_splitter->sizes().at(0));
        s->endGroup();
    });

    QObject::connect(ICore::instance(), &ICore::coreAboutToClose, [] {
        // This is the catch-all on rampdown. Individual items may
        // have been removed earlier by destroyStatusBarWidget().
        for (const QPointer<IContext> &context : m_contexts) {
            ICore::removeContextObject(context);
            delete context;
        }
        m_contexts.clear();
    });
}

void StatusBarManager::addStatusBarWidget(QWidget *widget,
                                          StatusBarPosition position,
                                          const Context &ctx)
{
    if (!m_splitter)
        createStatusBarManager();

    QTC_ASSERT(widget, return);
    QTC_CHECK(widget->parent() == nullptr); // We re-parent, so user code does need / should not set it.
    m_statusBarWidgets.at(position)->layout()->addWidget(widget);

    auto context = new IContext;
    context->setWidget(widget);
    context->setContext(ctx);
    m_contexts.append(context);

    ICore::addContextObject(context);
}

void StatusBarManager::destroyStatusBarWidget(QWidget *widget)
{
    QTC_ASSERT(widget, return);
    for (const QPointer<IContext> &context : m_contexts) {
        if (context->widget() == widget) {
            ICore::removeContextObject(context);
            m_contexts.removeAll(context);
            break;
        }
    }
    widget->setParent(nullptr);
    delete widget;
}

void StatusBarManager::restoreSettings()
{
    QSettings *s = ICore::settings();
    s->beginGroup(QLatin1String(kSettingsGroup));
    int leftSplitWidth = s->value(QLatin1String(kLeftSplitWidthKey), -1).toInt();
    s->endGroup();
    if (leftSplitWidth < 0) {
        // size first split after its sizeHint + a bit of buffer
        leftSplitWidth = m_splitter->widget(0)->sizeHint().width();
    }
    int sum = 0;
    foreach (int w, m_splitter->sizes())
        sum += w;
    m_splitter->setSizes(QList<int>() << leftSplitWidth << (sum - leftSplitWidth));
}

StatusBarContext::StatusBarContext(QObject *parent)
    : IContext(parent)
{
}

Context StatusBarContext::context() const
{
    IMode *currentMode = ModeManager::currentMode();
    QWidget *modeWidget = currentMode ? currentMode->widget() : nullptr;
    if (modeWidget) {
        if (IContext *context = ICore::contextObject(modeWidget))
            return context->context();
    }
    return Context();
}

} // Core
