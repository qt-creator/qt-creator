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

#include <QLabel>

namespace ADS {

struct ElidingLabelPrivate;

/**
 * A QLabel that supports eliding text.
 * Because the functions setText() and text() are no virtual functions setting
 * and reading the text via a pointer to the base class QLabel does not work
 * properly
 */
class ADS_EXPORT ElidingLabel : public QLabel
{
    Q_OBJECT
private:
    ElidingLabelPrivate *d;
    friend struct ElidingLabelPrivate;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;

public:
    using Super = QLabel;

    ElidingLabel(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ElidingLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ElidingLabel() override;

    /**
     * Returns the text elide mode.
     * The default mode is ElideNone
     */
    Qt::TextElideMode elideMode() const;

    /**
     * Sets the text elide mode
     */
    void setElideMode(Qt::TextElideMode mode);

    /**
     * This function indicates whether the text on this label is currently elided
     */
    bool isElided() const;

public: // reimplements QLabel
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    void setText(const QString &text);
    QString text() const;

signals:
    /**
     * This signal is emitted if the user clicks on the label (i.e. pressed
     * down then released while the mouse cursor is inside the label)
     */
    void clicked();

    /**
     * This signal is emitted if the user does a double click on the label
     */
    void doubleClicked();

    /**
     * This signal is emitted when isElided() state of this label is changed
     */
    void elidedChanged(bool elided);

private:
    /**
      * Helper to port to Qt 6
      */
    bool hasPixmap() const;
}; //class ElidingLabel

} // namespace ADS
