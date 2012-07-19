/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CRUMBLEPATH_H
#define CRUMBLEPATH_H

#include "utils_global.h"

#include <QWidget>
#include <QVariant>

namespace Utils {

struct CrumblePathPrivate;

class QTCREATOR_UTILS_EXPORT CrumblePath : public QWidget
{
    Q_OBJECT

public:
    explicit CrumblePath(QWidget *parent = 0);
    ~CrumblePath();

    void selectIndex(int index);
    QVariant dataForIndex(int index) const;
    QVariant dataForLastIndex() const;
    int length() const;
    void sortChildren(Qt::SortOrder order = Qt::AscendingOrder);

public slots:
    void pushElement(const QString &title, const QVariant &data = QVariant());
    void addChild(const QString &title, const QVariant &data = QVariant());
    void popElement();
    virtual void clear();

signals:
    void elementClicked(const QVariant &data);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void emitElementClicked();

private:
    void resizeButtons();
    void setBackgroundStyle();

private:
    CrumblePathPrivate *d;
};

} // namespace Utils

#endif // CRUMBLEPATH_H
