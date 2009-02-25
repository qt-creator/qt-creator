/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FANCYTABWIDGET_H
#define FANCYTABWIDGET_H

#include <QtGui/QPushButton>
#include <QtGui/QTabBar>
#include <QtGui/QStyleOptionTabV2>
#include <QtCore/QTimeLine>

QT_BEGIN_NAMESPACE
class QPainter;
class QStackedLayout;
class QStatusBar;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class FancyTabBar : public QTabBar
{
    Q_OBJECT

public:
    FancyTabBar(QWidget *parent = 0);
    ~FancyTabBar();

    QSize tabSizeHint(int index) const;
    void paintEvent(QPaintEvent *event);
    void paintTab(QPainter *painter, int tabIndex) const;
    void mouseMoveEvent(QMouseEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void tabInserted(int index);
    void tabRemoved(int index);

public slots:
    void updateHover();

private:
    static const int m_rounding;
    static const int m_textPadding;
    QTimeLine m_hoverControl;
    QRect m_hoverRect;
};

class FancyTabWidget : public QWidget
{
    Q_OBJECT

public:
    FancyTabWidget(QWidget *parent = 0);

    void insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label);
    void removeTab(int index);
    void setBackgroundBrush(const QBrush &brush);
    void addCornerWidget(QWidget *widget);
    void insertCornerWidget(int pos, QWidget *widget);
    int cornerWidgetCount() const;
    void setTabToolTip(int index, const QString &toolTip);

    void paintEvent(QPaintEvent *event);

    int currentIndex() const;
    QStatusBar *statusBar() const;

signals:
    void currentAboutToShow(int index);
    void currentChanged(int index);

public slots:
    void setCurrentIndex(int index);

private slots:
    void showWidget(int index);

private:
    FancyTabBar *m_tabBar;
    QWidget *m_cornerWidgetContainer;
    QStackedLayout *m_modesStack;
    QWidget *m_selectionWidget;
    QStatusBar *m_statusBar;
};

} // namespace Internal
} // namespace Core

#endif // FANCYTABWIDGET_H
