/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "docksplitter.h"

#include "dockareawidget.h"

#include <QChildEvent>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
    /**
     * Private dock splitter data
     */
    struct DockSplitterPrivate
    {
        DockSplitter *q;
        int m_visibleContentCount = 0;

        DockSplitterPrivate(DockSplitter *parent)
            : q(parent)
        {}
    };

    DockSplitter::DockSplitter(QWidget *parent)
        : QSplitter(parent)
        , d(new DockSplitterPrivate(this))
    {
        //setProperty("ads-splitter", true); // TODO
        setProperty("minisplitter", true);
        setChildrenCollapsible(false);
    }

    DockSplitter::DockSplitter(Qt::Orientation orientation, QWidget *parent)
        : QSplitter(orientation, parent)
        , d(new DockSplitterPrivate(this))
    {}

    DockSplitter::~DockSplitter()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        delete d;
    }

    bool DockSplitter::hasVisibleContent() const
    {
        // TODO Cache or precalculate this to speed up
        for (int i = 0; i < count(); ++i) {
            if (!widget(i)->isHidden()) {
                return true;
            }
        }

        return false;
    }

    QWidget *DockSplitter::firstWidget() const
    {
        return (count() > 0) ? widget(0) : nullptr;
    }

    QWidget *DockSplitter::lastWidget() const
    {
        return (count() > 0) ? widget(count() - 1) : nullptr;
    }

} // namespace ADS
