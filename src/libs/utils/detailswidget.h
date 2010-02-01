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

#include <QtGui/QPixmap>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QGridLayout;
QT_END_NAMESPACE

namespace Utils {
class DetailsButton;

class QTCREATOR_UTILS_EXPORT DetailsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString summaryText READ summaryText WRITE setSummaryText DESIGNABLE true)
    Q_PROPERTY(State state READ state WRITE setState)

public:
    enum State {
        Expanded,
        Collapsed,
        NoSummary
    };


    DetailsWidget(QWidget *parent = 0);
    ~DetailsWidget();

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
    void paintEvent(QPaintEvent *paintEvent);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private:
    void updateControls();
    QPixmap cacheBackground(const QSize &size, bool expanded);
    void changeHoverState(bool hovered);

    DetailsButton *m_detailsButton;
    QGridLayout *m_grid;
    QLabel *m_summaryLabel;
    QWidget *m_toolWidget;
    QWidget *m_widget;

    QPixmap m_collapsedPixmap;
    QPixmap m_expandedPixmap;

    State m_state;
    bool m_hovered;
};
}

#endif // DETAILSWIDGET_H
