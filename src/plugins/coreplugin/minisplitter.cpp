/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "minisplitter.h"
#include "stylehelper.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QSplitterHandle>

namespace Core {
namespace Internal {

class MiniSplitterHandle : public QSplitterHandle
{
public:
    MiniSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
            : QSplitterHandle(orientation, parent)
    {
        setMask(QRegion(contentsRect()));
        setAttribute(Qt::WA_MouseNoMask, true);
    }
protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
};

} // namespace Internal
} // namespace Core

using namespace Core;
using namespace Core::Internal;

void MiniSplitterHandle::resizeEvent(QResizeEvent *event)
{
    if (orientation() == Qt::Horizontal)
        setContentsMargins(2, 0, 2, 0);
    else
        setContentsMargins(0, 2, 0, 2);
    setMask(QRegion(contentsRect()));
    QSplitterHandle::resizeEvent(event);
}

void MiniSplitterHandle::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), StyleHelper::borderColor());
}

QSplitterHandle *MiniSplitter::createHandle()
{
    return new MiniSplitterHandle(orientation(), this);
}

MiniSplitter::MiniSplitter(QWidget *parent)
    : QSplitter(parent)
{
    setHandleWidth(1);
    setChildrenCollapsible(false);
    setProperty("minisplitter", true);
}

MiniSplitter::MiniSplitter(Qt::Orientation orientation)
    : QSplitter(orientation)
{
    setHandleWidth(1);
    setChildrenCollapsible(false);
    setProperty("minisplitter", true);
}
