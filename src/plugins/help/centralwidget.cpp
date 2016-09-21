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

#include "centralwidget.h"

#include "helpviewer.h"
#include "localhelpmanager.h"

#include <utils/qtcassert.h>

using namespace Help::Internal;

static CentralWidget *gStaticCentralWidget = 0;

// -- CentralWidget

CentralWidget::CentralWidget(const Core::Context &context, QWidget *parent)
    : HelpWidget(context, HelpWidget::ModeWidget, parent)
{
    QTC_CHECK(!gStaticCentralWidget);
    gStaticCentralWidget = this;
}

CentralWidget::~CentralWidget()
{
    // TODO: this shouldn't be done here
    QList<qreal> zoomFactors;
    QStringList currentPages;
    for (int i = 0; i < viewerCount(); ++i) {
        const HelpViewer * const viewer = viewerAt(i);
        const QUrl &source = viewer->source();
        if (source.isValid()) {
            currentPages.append(source.toString());
            zoomFactors.append(viewer->scale());
        }
    }

    LocalHelpManager::setLastShownPages(currentPages);
    LocalHelpManager::setLastShownPagesZoom(zoomFactors);
    LocalHelpManager::setLastSelectedTab(currentIndex());
}

CentralWidget *CentralWidget::instance()
{
    Q_ASSERT(gStaticCentralWidget);
    return gStaticCentralWidget;
}
