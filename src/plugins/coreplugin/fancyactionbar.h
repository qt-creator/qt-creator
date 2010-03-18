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

#ifndef FANCYACTIONBAR_H
#define FANCYACTIONBAR_H

#include <QtGui/QToolButton>

QT_BEGIN_NAMESPACE
class QMenu;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core {
    class IMode;
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

private slots:
    void modeChanged(Core::IMode *mode);

private:
    QVBoxLayout *m_actionsLayout;
    FancyToolButton *m_runButton;
    FancyToolButton *m_debugButton;
};

} // namespace Internal
} // namespace Core

#endif // FANCYACTIONBAR_H
