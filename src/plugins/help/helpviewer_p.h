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

#ifndef HELPVIEWERPRIVATE_H
#define HELPVIEWERPRIVATE_H

#include "centralwidget.h"
#include "helpviewer.h"
#include "openpagesmanager.h"

#include <QObject>
#include <QTextBrowser>

namespace Help {
    namespace Internal {

class HelpViewer::HelpViewerPrivate : public QObject
{
    Q_OBJECT

public:
    HelpViewerPrivate(int zoom)
        : zoomCount(zoom)
        , forceFont(false)
        , lastAnchor(QString())

    {}

    bool hasAnchorAt(QTextBrowser *browser, const QPoint& pos)
    {
        lastAnchor = browser->anchorAt(pos);
        if (lastAnchor.isEmpty())
            return false;

        lastAnchor = browser->source().resolved(lastAnchor).toString();
        if (lastAnchor.at(0) == QLatin1Char('#')) {
            QString src = browser->source().toString();
            int hsh = src.indexOf(QLatin1Char('#'));
            lastAnchor = (hsh >= 0 ? src.left(hsh) : src) + lastAnchor;
        }
        return true;
    }

    void openLink(bool newPage)
    {
        if (lastAnchor.isEmpty())
            return;
        if (newPage)
            OpenPagesManager::instance().createPage(lastAnchor);
        else
            CentralWidget::instance()->setSource(lastAnchor);
        lastAnchor.clear();
    }

public slots:
    void openLink()
    {
        openLink(false);
    }

    void openLinkInNewPage()
    {
        openLink(true);
    }

public:
    int zoomCount;
    bool forceFont;
    QString lastAnchor;
};

    }   // namespace Help
}   // namespace Internal

#endif  // HELPVIEWERPRIVATE_H
