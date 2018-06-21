/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#pragma once

#include <QWidget>
#include "qmt/infrastructure/qmt_global.h"

#include <QColor>
#include <QVector>

namespace qmt {

class QMT_EXPORT PaletteBox : public QWidget
{
    Q_OBJECT

public:
    explicit PaletteBox(QWidget *parent = nullptr);
    ~PaletteBox() override;

signals:
    void activated(int index);

public:
    QBrush brush(int index) const;
    void setBrush(int index, const QBrush &brush);
    QPen linePen(int index) const;
    void setLinePen(int index, const QPen &pen);
    int currentIndex() const { return m_currentIndex; }

    void clear();
    void setCurrentIndex(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QVector<QBrush> m_brushes;
    QVector<QPen> m_pens;
    int m_currentIndex = -1;
};

} // namespace qmt
