/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "outputpane.h"
#include "outputpanemanager.h"

#include "modemanager.h"

#include <QSplitter>
#include <QVBoxLayout>

namespace Core {

struct OutputPanePlaceHolderPrivate {
    explicit OutputPanePlaceHolderPrivate(Core::IMode *mode, QSplitter *parent);

    Core::IMode *m_mode;
    QSplitter *m_splitter;
    int m_lastNonMaxSize;
    static OutputPanePlaceHolder* m_current;
};

OutputPanePlaceHolderPrivate::OutputPanePlaceHolderPrivate(Core::IMode *mode, QSplitter *parent) :
    m_mode(mode), m_splitter(parent), m_lastNonMaxSize(0)
{
}

OutputPanePlaceHolder *OutputPanePlaceHolderPrivate::m_current = 0;

OutputPanePlaceHolder::OutputPanePlaceHolder(Core::IMode *mode, QSplitter* parent)
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
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(currentModeChanged(Core::IMode*)));
}

OutputPanePlaceHolder::~OutputPanePlaceHolder()
{
    if (d->m_current == this) {
        if (Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance()) {
            om->setParent(0);
            om->hide();
        }
    }
    delete d;
}

void OutputPanePlaceHolder::currentModeChanged(Core::IMode *mode)
{
    if (d->m_current == this) {
        d->m_current = 0;
        Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance();
        om->setParent(0);
        om->hide();
        om->updateStatusButtons(false);
    }
    if (d->m_mode == mode) {
        d->m_current = this;
        Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance();
        layout()->addWidget(om);
        om->show();
        om->updateStatusButtons(isVisible());
    }
}

void OutputPanePlaceHolder::maximizeOrMinimize(bool maximize)
{
    if (!d->m_splitter)
        return;
    int idx = d->m_splitter->indexOf(this);
    if (idx < 0)
        return;

    QList<int> sizes = d->m_splitter->sizes();

    if (maximize) {
        d->m_lastNonMaxSize = sizes[idx];
        int sum = 0;
        foreach (int s, sizes)
            sum += s;
        for (int i = 0; i < sizes.count(); ++i) {
            sizes[i] = 32;
        }
        sizes[idx] = sum - (sizes.count()-1) * 32;
    } else {
        int target = d->m_lastNonMaxSize > 0 ? d->m_lastNonMaxSize : sizeHint().height();
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
    return Internal::OutputPaneManager::instance()->isMaximized();
}

void OutputPanePlaceHolder::ensureSizeHintAsMinimum()
{
    if (!d->m_splitter)
        return;
    int idx = d->m_splitter->indexOf(this);
    if (idx < 0)
        return;

    QList<int> sizes = d->m_splitter->sizes();
    Internal::OutputPaneManager *om = Internal::OutputPaneManager::instance();
    int minimum = (d->m_splitter->orientation() == Qt::Vertical
                   ? om->sizeHint().height() : om->sizeHint().width());
    int difference = minimum - sizes.at(idx);
    if (difference <= 0) // is already larger
        return;
    for (int i = 0; i < sizes.count(); ++i) {
        sizes[i] += difference / (sizes.count()-1);
    }
    sizes[idx] = minimum;
    d->m_splitter->setSizes(sizes);
}

void OutputPanePlaceHolder::unmaximize()
{
    if (Internal::OutputPaneManager::instance()->isMaximized())
        Internal::OutputPaneManager::instance()->slotMinMax();
}

OutputPanePlaceHolder *OutputPanePlaceHolder::getCurrent()
{
    return OutputPanePlaceHolderPrivate::m_current;
}

bool OutputPanePlaceHolder::canMaximizeOrMinimize() const
{
    return d->m_splitter != 0;
}

bool OutputPanePlaceHolder::isCurrentVisible()
{
    return OutputPanePlaceHolderPrivate::m_current && OutputPanePlaceHolderPrivate::m_current->isVisible();
}

} // namespace Core


