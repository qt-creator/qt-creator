// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <QLabel>

namespace ADS {

class ElidingLabelPrivate;

/**
 * A QLabel that supports eliding text.
 * Because the functions setText() and text() are no virtual functions setting and reading the
 * text via a pointer to the base class QLabel does not work properly.
 */
class ADS_EXPORT ElidingLabel : public QLabel
{
    Q_OBJECT
private:
    ElidingLabelPrivate *d;
    friend class ElidingLabelPrivate;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;

public:
    using Super = QLabel;

    ElidingLabel(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ElidingLabel(const QString &text,
                 QWidget *parent = nullptr,
                 Qt::WindowFlags flags = Qt::WindowFlags());
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
