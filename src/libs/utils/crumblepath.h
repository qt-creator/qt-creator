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

#ifndef CRUMBLEPATH_H
#define CRUMBLEPATH_H

#include <QWidget>
#include <QList>
#include "utils_global.h"

QT_FORWARD_DECLARE_CLASS(QResizeEvent);

namespace Utils {

class CrumblePathButton;

class QTCREATOR_UTILS_EXPORT CrumblePath : public QWidget
{
    Q_OBJECT
public:
    explicit CrumblePath(QWidget *parent = 0);
    ~CrumblePath();

public slots:
    void pushElement(const QString &title);
    void popElement();
    void clear();

signals:
    void elementClicked(int index);
    void elementContextMenuRequested(int index);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void mapClickToIndex();
    void mapContextMenuRequestToIndex();

private:
    void resizeButtons();

private:
    QList<CrumblePathButton*> m_buttons;
    QWidget *m_background;
};

} // namespace QmlViewer

#endif // CRUMBLEPATH_H
