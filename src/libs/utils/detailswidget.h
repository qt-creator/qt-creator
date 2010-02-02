/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE

namespace Utils {

struct DetailsWidgetPrivate;

class QTCREATOR_UTILS_EXPORT DetailsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString summaryText READ summaryText WRITE setSummaryText DESIGNABLE true)
    Q_PROPERTY(State state READ state WRITE setState)
    Q_ENUMS(State)

public:
    enum State {
        Expanded,
        Collapsed,
        NoSummary
    };

    explicit DetailsWidget(QWidget *parent = 0);
    virtual ~DetailsWidget();

    void setSummaryText(const QString &text);
    QString summaryText() const;

    void setState(State state);
    State state() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setToolWidget(QWidget *widget);
    QWidget *toolWidget() const;

private slots:
    void setExpanded(bool);

protected:
    virtual void paintEvent(QPaintEvent *paintEvent);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private:
    void updateControls();
    QPixmap cacheBackground(const QSize &size, bool expanded);
    void changeHoverState(bool hovered);

    DetailsWidgetPrivate *d;
};

} // namespace Utils

#endif // DETAILSWIDGET_H
