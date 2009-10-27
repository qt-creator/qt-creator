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

#ifndef WELCOMEMODETREEWIDGET_H
#define WELCOMEMODETREEWIDGET_H

#include "utils_global.h"

#include <QtGui/QTreeWidget>
#include <QtGui/QLabel>

namespace Utils {

struct WelcomeModeTreeWidgetPrivate;
struct WelcomeModeLabelPrivate;

class QTCREATOR_UTILS_EXPORT WelcomeModeLabel : public QLabel
{
    Q_OBJECT
public:
    WelcomeModeLabel(QWidget *parent) : QLabel(parent) {};
    void setStyledText(const QString &text);
    WelcomeModeLabelPrivate *m_d;
};

class QTCREATOR_UTILS_EXPORT WelcomeModeTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    WelcomeModeTreeWidget(QWidget *parent = 0);
    ~WelcomeModeTreeWidget();
    QTreeWidgetItem *addItem(const QString &label, const QString &data,const QString &toolTip = QString::null);

public slots:
    void slotAddNewsItem(const QString &title, const QString &description, const QString &link);

signals:
    void activated(const QString &data);

protected:
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;

private slots:
    void slotItemClicked(QTreeWidgetItem *item);

private:
    WelcomeModeTreeWidgetPrivate *m_d;
};

}

#endif // WELCOMEMODETREEWIDGET_H
