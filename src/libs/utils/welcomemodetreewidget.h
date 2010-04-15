/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
class WelcomeModeItemWidget;

// Label usable for headers of a Welcome page.
class QTCREATOR_UTILS_EXPORT WelcomeModeLabel : public QLabel
{
    Q_OBJECT
public:
    explicit WelcomeModeLabel(QWidget *parent = 0);
    virtual ~WelcomeModeLabel();

private:
    void *m_unused;
};

// WelcomeModeTreeWidget: Show an itemized list with arrows and emits a signal on click.
class QTCREATOR_UTILS_EXPORT WelcomeModeTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WelcomeModeTreeWidget(QWidget *parent = 0);
    virtual ~WelcomeModeTreeWidget();

public slots:
    void addItem(const QString &label, const QString &data,const QString &toolTip = QString());

    // Add a 'News' item as two lines of "<bold>Breaking news!</bold>\nElided Start of article...."
    void addNewsItem(const QString &title, const QString &description, const QString &link);
    void clear();

signals:
    void activated(const QString &data);

private:
    void addItemWidget(WelcomeModeItemWidget *w);

    WelcomeModeTreeWidgetPrivate *m_d;
};

} // namespace Utils
#endif // WELCOMEMODETREEWIDGET_H
