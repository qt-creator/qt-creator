/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_PALETTEBOX_H
#define QMT_PALETTEBOX_H

#include <QWidget>
#include "qmt/infrastructure/qmt_global.h"

#include <QColor>
#include <QVector>


namespace qmt {

class QMT_EXPORT PaletteBox : public QWidget
{
    Q_OBJECT

public:
    explicit PaletteBox(QWidget *parent = 0);

    ~PaletteBox();

signals:

    void activated(int index);

public:

    QBrush getBrush(int index) const;

    void setBrush(int index, const QBrush &brush);

    QPen getLinePen(int index) const;

    void setLinePen(int index, const QPen &pen);

    int currentIndex() const { return m_currentIndex; }

public slots:

    void clear();

    void setCurrentIndex(int index);

protected:

    virtual void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);

private:

    QVector<QBrush> m_brushes;

    QVector<QPen> m_pens;

    int m_currentIndex;
};

} // namespace qmt

#endif // QMT_PALETTEBOX_H
