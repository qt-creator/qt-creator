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

#include "imode.h"
#include "modemanager.h"
#include "outputpane.h"
#include "outputpanemanager.h"

#include <QResizeEvent>
#include <QSplitter>
#include <QVBoxLayout>

namespace Core {

class OutputPanePlaceHolderPrivate {
public:
    explicit OutputPanePlaceHolderPrivate(Id mode, QSplitter *parent);

    Id m_mode;
    QSplitter *m_splitter;
    int m_nonMaximizedSize = 0;
    bool m_isMaximized = false;
    bool m_initialized = false;
    static OutputPanePlaceHolder* m_current;
};

OutputPanePlaceHolderPrivate::OutputPanePlaceHolderPrivate(Id mode, QSplitter *parent) :
    m_mode(mode), m_splitter(parent)
{
}

OutputPanePlaceHolder *OutputPanePlaceHolderPrivate::m_current = 0;

OutputPanePlaceHolder::OutputPanePlaceHolder(Id mode, QSplitter *parent)
   : QWidget(parent), d(new OutputPanePlaceHolderPrivate(mode, parent))
{
    setVisible(false);
    setLayout(new QVBoxLayout);
    QSizePolicy sp;
    sp.setHorizontalPolicy(QSizePolicy::Preferred);
    sp.setVerticalPolicy(QSizePolicy::Preferred);
    sp.setHorizontalStretch(0);
    setSizePolicy(sp);
    layout()->setMargin(0);
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &OutputPanePlaceHolder::currentModeChanged);
    // if this is part of a lazily created mode widget,
    // we need to check if this is the current placeholder
    currentModeChanged(ModeManager::currentMode());
}

OutputPanePlaceHolder::~OutputPanePlaceHolder()
{
    if (d->m_current == this) {
        if (Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance()) {
            om->setParent(0);
            om->hide();
        }
        OutputPanePlaceHolderPrivate::m_current = nullptr;
    }
    delete d;
}

void OutputPanePlaceHolder::currentModeChanged(Id mode)
{
    if (d->m_current == this) {
        d->m_current = 0;
        if (d->m_initialized)
            Internal::OutputPaneManager::setOutputPaneHeightSetting(d->m_nonMaximizedSize);
        Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance();
        om->hide();
        om->setParent(0);
        om->updateStatusButtons(false);
    }
    if (d->m_mode == mode) {
        if (d->m_current && d->m_current->d->m_initialized)
            Internal::OutputPaneManager::setOutputPaneHeightSetting(d->m_current->d->m_nonMaximizedSize);
        d->m_current = this;
        Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance();
        layout()->addWidget(om);
        om->show();
        om->updateStatusButtons(isVisible());
        Internal::OutputPaneManager::updateMaximizeButton(d->m_isMaximized);
    }
}

void OutputPanePlaceHolder::setMaximized(bool maximize)
{
    if (d->m_isMaximized == maximize)
        return;
    if (!d->m_splitter)
        return;
    int idx = d->m_splitter->indexOf(this);
    if (idx < 0)
        return;

    d->m_isMaximized = maximize;
    if (d->m_current == this)
        Internal::OutputPaneManager::updateMaximizeButton(d->m_isMaximized);
    QList<int> sizes = d->m_splitter->sizes();

    if (maximize) {
        d->m_nonMaximizedSize = sizes[idx];
        int sum = 0;
        foreach (int s, sizes)
            sum += s;
        for (int i = 0; i < sizes.count(); ++i) {
            sizes[i] = 32;
        }
        sizes[idx] = sum - (sizes.count()-1) * 32;
    } else {
        int target = d->m_nonMaximizedSize > 0 ? d->m_nonMaximizedSize : sizeHint().height();
        int space = sizes[idx] - target;
        if (space > 0) {
            for (int i = 0; i < sizes.count(); ++i) {
                sizes[i] += space / (sizes.count()-1);
            }
            sizes[idx] = target;
        }
    }

    d->m_splitter->setSizes(sizes);
}

bool OutputPanePlaceHolder::isMaximized() const
{
    return d->m_isMaximized;
}

void OutputPanePlaceHolder::setHeight(int height)
{
    if (height == 0)
        return;
    if (!d->m_splitter)
        return;
    const int idx = d->m_splitter->indexOf(this);
    if (idx < 0)
        return;

    d->m_splitter->refresh();
    QList<int> sizes = d->m_splitter->sizes();
    const int difference = height - sizes.at(idx);
    if (difference == 0)
        return;
    const int adaption = difference / (sizes.count()-1);
    for (int i = 0; i < sizes.count(); ++i) {
        sizes[i] -= adaption;
    }
    sizes[idx] = height;
    d->m_splitter->setSizes(sizes);
}

void OutputPanePlaceHolder::ensureSizeHintAsMinimum()
{
    Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance();
    int minimum = (d->m_splitter->orientation() == Qt::Vertical
                   ? om->sizeHint().height() : om->sizeHint().width());
    if (nonMaximizedSize() < minimum && !d->m_isMaximized)
        setHeight(minimum);
}

int OutputPanePlaceHolder::nonMaximizedSize() const
{
    if (!d->m_initialized)
        return Internal::OutputPaneManager::outputPaneHeightSetting();
    return d->m_nonMaximizedSize;
}

void OutputPanePlaceHolder::resizeEvent(QResizeEvent *event)
{
    if (d->m_isMaximized || event->size().height() == 0)
        return;
    d->m_nonMaximizedSize = event->size().height();
}

void OutputPanePlaceHolder::showEvent(QShowEvent *)
{
    if (!d->m_initialized) {
        d->m_initialized = true;
        setHeight(Internal::OutputPaneManager::outputPaneHeightSetting());
    }
}

OutputPanePlaceHolder *OutputPanePlaceHolder::getCurrent()
{
    return OutputPanePlaceHolderPrivate::m_current;
}

bool OutputPanePlaceHolder::isCurrentVisible()
{
    return OutputPanePlaceHolderPrivate::m_current && OutputPanePlaceHolderPrivate::m_current->isVisible();
}

} // namespace Core


