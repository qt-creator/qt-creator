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
}; // class DockSplitter

} // namespace ADS
