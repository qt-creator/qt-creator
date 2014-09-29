/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "centralwidget.h"

#include "helpviewer.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "topicchooser.h"

#include <utils/qtcassert.h>

#include <QHelpEngine>

using namespace Help::Internal;

CentralWidget *gStaticCentralWidget = 0;

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
    QString zoomFactors;
    QString currentPages;
    for (int i = 0; i < viewerCount(); ++i) {
        const HelpViewer * const viewer = viewerAt(i);
        const QUrl &source = viewer->source();
        if (source.isValid()) {
            currentPages += source.toString() + QLatin1Char('|');
            zoomFactors += QString::number(viewer->scale()) + QLatin1Char('|');
        }
    }

    QHelpEngineCore *engine = &LocalHelpManager::helpEngine();
    engine->setCustomValue(QLatin1String("LastShownPages"), currentPages);
    engine->setCustomValue(QLatin1String("LastShownPagesZoom"), zoomFactors);
    engine->setCustomValue(QLatin1String("LastTabPage"), currentIndex());
}

CentralWidget *CentralWidget::instance()
{
    Q_ASSERT(gStaticCentralWidget);
    return gStaticCentralWidget;
}

void CentralWidget::open(const QUrl &url, bool newPage)
{
    if (newPage)
        OpenPagesManager::instance().createPage(url);
    else
        setSource(url);
}

void CentralWidget::showTopicChooser(const QMap<QString, QUrl> &links,
    const QString &keyword, bool newPage)
{
    TopicChooser tc(this, keyword, links);
    if (tc.exec() == QDialog::Accepted)
        open(tc.link(), newPage);
}
