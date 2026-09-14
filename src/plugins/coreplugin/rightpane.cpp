// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rightpane.h"

#include "modemanager.h"

#include <utils/qtcsettings.h>

#include <QVBoxLayout>
#include <QSplitter>
#include <QResizeEvent>


using namespace Core;
using namespace Core::Internal;
using namespace Utils;

RightPanePlaceHolder *RightPanePlaceHolder::m_current = nullptr;

RightPanePlaceHolder* RightPanePlaceHolder::current()
{
    return m_current;
}

RightPanePlaceHolder::RightPanePlaceHolder(Id mode, QWidget *parent)
    :QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &RightPanePlaceHolder::currentModeChanged);
}

RightPanePlaceHolder::~RightPanePlaceHolder()
{
    if (m_current == this) {
        RightPaneWidget::instance()->setParent(nullptr);
        RightPaneWidget::instance()->hide();
    }
}

void RightPanePlaceHolder::applyStoredSize(int width)
{
    if (width) {
        auto splitter = qobject_cast<QSplitter *>(parentWidget());
        if (splitter) {
            // A splitter we need to resize the splitter sizes
            QList<int> sizes = splitter->sizes();
            int index = splitter->indexOf(this);
            int diff = width - sizes.at(index);
            int adjust = sizes.count() > 1 ? (diff / (sizes.count() - 1)) : 0;
            for (int i = 0; i < sizes.count(); ++i) {
                if (i != index)
                    sizes[i] -= adjust;
            }
            sizes[index]= width;
            splitter->setSizes(sizes);
        } else {
            QSize s = size();
            s.setWidth(width);
            resize(s);
        }
    }
}

// This function does work even though the order in which
// the placeHolder get the signal is undefined.
// It does ensure that after all PlaceHolders got the signal
// m_current points to the current PlaceHolder, or zero if there
// is no PlaceHolder in this mode
// And that the parent of the RightPaneWidget gets the correct parent
void RightPanePlaceHolder::currentModeChanged(Id mode)
{
    if (m_current == this) {
        m_current = nullptr;
        RightPaneWidget::instance()->storeState(m_mode);
        RightPaneWidget::instance()->setParent(nullptr);
        RightPaneWidget::instance()->hide();
    }
    if (m_mode == mode) {
        m_current = this;
        RightPaneWidget::instance()->applyState(m_mode);

        int width = RightPaneWidget::instance()->storedWidth();

        layout()->addWidget(RightPaneWidget::instance());
        RightPaneWidget::instance()->show();

        applyStoredSize(width);
        setVisible(RightPaneWidget::instance()->isShown());
    }
}

/////
// RightPaneWidget
/////


RightPaneWidget *RightPaneWidget::m_instance = nullptr;

RightPaneWidget::RightPaneWidget()
{
    m_instance = this;

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

RightPaneWidget::~RightPaneWidget()
{
    clearWidget();
    m_instance = nullptr;
}

RightPaneWidget *RightPaneWidget::instance()
{
    return m_instance;
}

void RightPaneWidget::setWidget(QWidget *widget)
{
    if (widget == m_widget)
        return;
    clearWidget();
    m_widget = widget;
    if (m_widget) {
        m_widget->setParent(this);
        layout()->addWidget(m_widget);
        setFocusProxy(m_widget);
        m_widget->show();
    }
}

QWidget *RightPaneWidget::widget() const
{
    return m_widget;
}

int RightPaneWidget::storedWidth() const
{
    return m_width;
}

void RightPaneWidget::resizeEvent(QResizeEvent *re)
{
    if (m_width && re->size().width())
        m_width = re->size().width();
    QWidget::resizeEvent(re);
}

static const bool kVisibleDefault = false;
static const int kWidthDefault = 500;

// The invalid Id is the state the modes that did not ask for one share.
static Id stateKey(Id mode)
{
    return ModeManager::modeKeepsOwnLayout(mode) ? mode : Id();
}

void RightPaneWidget::storeState(Id mode)
{
    const ModeState state{m_shown, m_width};
    if (const Id key = stateKey(mode); key.isValid())
        m_modeStates.insert(key, state);
    else
        m_defaultState = state;
}

void RightPaneWidget::applyState(Id mode)
{
    const Id key = stateKey(mode);
    const ModeState state = key.isValid() ? m_modeStates.value(key, m_defaultState)
                                          : m_defaultState;
    setShown(state.visible);
    m_width = state.width;
}

void RightPaneWidget::saveSettings(Utils::QtcSettings *settings)
{
    settings->setValueWithDefault("RightPane/Visible", isShown(), kVisibleDefault);
    settings->setValueWithDefault("RightPane/Width", m_width, kWidthDefault);
}

void RightPaneWidget::readSettings(QtcSettings *settings)
{
    m_defaultState.visible = settings->value("RightPane/Visible", kVisibleDefault).toBool();
    m_defaultState.width = settings->value("RightPane/Width", kWidthDefault).toInt();

    // Only a mode that has a place holder owns the live state.
    m_modeStates.clear();
    applyState(RightPanePlaceHolder::m_current ? ModeManager::currentModeId() : Id());

    // Apply
    if (RightPanePlaceHolder::m_current)
        RightPanePlaceHolder::m_current->applyStoredSize(m_width);
}

void RightPaneWidget::setShown(bool b)
{
    if (RightPanePlaceHolder::m_current)
        RightPanePlaceHolder::m_current->setVisible(b);
    m_shown = b;
}

bool RightPaneWidget::isShown() const
{
    return m_shown;
}

void RightPaneWidget::clearWidget()
{
    if (m_widget) {
        m_widget->hide();
        m_widget->setParent(nullptr);
        m_widget = nullptr;
    }
}
