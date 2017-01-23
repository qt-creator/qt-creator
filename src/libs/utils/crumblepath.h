/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"

#include <QWidget>
#include <QVariant>

namespace Utils {

class CrumblePathButton;

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

private:
    void emitElementClicked();
    void resizeButtons();
    void setBackgroundStyle();

private:
    QList<CrumblePathButton*> m_buttons;
};

} // namespace Utils
