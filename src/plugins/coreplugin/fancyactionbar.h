/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FANCYACTIONBAR_H
#define FANCYACTIONBAR_H

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class FancyToolButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(float fader READ fader WRITE setFader)

public:
    FancyToolButton(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);
    bool event(QEvent *e);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    float m_fader;
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

private slots:
    void actionChanged();
};

class FancyActionBar : public QWidget
{
    Q_OBJECT

public:
    FancyActionBar(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);
    void insertAction(int index, QAction *action);
    void addProjectSelector(QAction *action);
    QLayout *actionsLayout() const;
    QSize minimumSizeHint() const;

private:
    QVBoxLayout *m_actionsLayout;
};

} // namespace Internal
} // namespace Core

#endif // FANCYACTIONBAR_H
