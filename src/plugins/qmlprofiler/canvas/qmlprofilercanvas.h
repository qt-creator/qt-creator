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

#ifndef QMLPROFILERCANVAS_H
#define QMLPROFILERCANVAS_H

#include <QDeclarativeItem>

QT_BEGIN_NAMESPACE
class Context2D;
QT_END_NAMESPACE

namespace QmlProfiler {
namespace Internal {

class QmlProfilerCanvas : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

public:
    QmlProfilerCanvas();

    bool dirty() const { return m_dirty; }
    void setDirty(bool dirty)
    {
        if (m_dirty != dirty) {
            m_dirty = dirty;
            emit dirtyChanged(dirty);
        }
    }

signals:
    void dirtyChanged(bool dirty);

    void drawRegion(Context2D *ctxt, const QRect &region);

public slots:
    void requestPaint();
    void requestRedraw();

protected:
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void componentComplete();

private:
    Context2D *m_context2d;

    bool m_dirty;
};

}
}

#endif // QMLPROFILERCANVAS_H
