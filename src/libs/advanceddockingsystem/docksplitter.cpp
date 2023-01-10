// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "docksplitter.h"

#include "dockareawidget.h"

#include <QChildEvent>
#include <QLoggingCategory>
#include <QVariant>

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
