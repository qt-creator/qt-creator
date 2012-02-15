/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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

    void forceVisible(bool visible);

private slots:
    void actionChanged();

private:
    bool m_hasForceVisible;
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
