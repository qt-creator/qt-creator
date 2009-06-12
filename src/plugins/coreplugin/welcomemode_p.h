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

#ifndef WELCOMEMODE_P_H
#define WELCOMEMODE_P_H

#include <QtGui/QIcon>
#include <QtGui/QLabel>
#include <QtGui/QTreeWidget>

namespace Core {
namespace Internal {

class WelcomeModeButton : public QLabel
{
    Q_OBJECT

public:
    WelcomeModeButton(QWidget *parent = 0);

signals:
    void clicked();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private:
    bool m_isPressed;
    bool m_isInited;
    QString m_text;
    QString m_hoverText;
};

class WelcomeModeTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    WelcomeModeTreeWidget(QWidget *parent = 0);
    QTreeWidgetItem *addItem(const QString &label, const QString &data);

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
    QIcon m_bullet;
};

}
}

#endif // WELCOMEMODE_P_H
