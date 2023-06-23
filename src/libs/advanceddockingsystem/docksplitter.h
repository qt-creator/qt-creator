// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <QSplitter>

namespace ADS {

struct DockSplitterPrivate;

/**
 * Splitter used internally instead of QSplitter with some additional
 * fuctionality.
 */
class ADS_EXPORT DockSplitter : public QSplitter
{
    Q_OBJECT
private:
    DockSplitterPrivate *d;
    friend struct DockSplitterPrivate;

public:
    DockSplitter(QWidget *parent = nullptr);
    DockSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);

    /**
     * Prints debug info
     */
    ~DockSplitter() override;

    /**
     * Returns true, if any of the internal widgets is visible
     */
    bool hasVisibleContent() const;

    /**
     * Returns first widget or nullptr if splitter is empty
     */
    QWidget *firstWidget() const;

    /**
     * Returns last widget of nullptr is splitter is empty
     */
    QWidget *lastWidget() const;

    /**
     * Returns true if the splitter contains central widget of dock manager.
     */
    bool isResizingWithContainer() const;
}; // class DockSplitter

} // namespace ADS
