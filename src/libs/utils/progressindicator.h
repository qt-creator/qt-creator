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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#ifndef PROGRESSINDICATOR_H
#define PROGRESSINDICATOR_H

#include "utils_global.h"

#include <QTimer>
#include <QWidget>

namespace Utils {

namespace Internal { class ProgressIndicatorPrivate; }

class QTCREATOR_UTILS_EXPORT ProgressIndicator : public QWidget
{
    Q_OBJECT
public:
    enum IndicatorSize {
        Small,
        Medium,
        Large
    };

    explicit ProgressIndicator(IndicatorSize size, QWidget *parent = 0);

    void setIndicatorSize(IndicatorSize size);
    IndicatorSize indicatorSize() const;

    QSize sizeHint() const;

    void attachToWidget(QWidget *parent);

protected:
    void paintEvent(QPaintEvent *);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    bool eventFilter(QObject *obj, QEvent *ev);

private:
    void step();
    void resizeToParent();

    ProgressIndicator::IndicatorSize m_size;
    int m_rotationStep;
    int m_rotation;
    QTimer m_timer;
    QPixmap m_pixmap;
};

} // Utils

#endif // PROGRESSINDICATOR_H
