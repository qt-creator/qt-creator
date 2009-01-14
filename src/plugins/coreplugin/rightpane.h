/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef RIGHTPANE_H
#define RIGHTPANE_H

#include "core_global.h"

#include <QtGui/QWidget>
#include <QtCore/QSettings>

namespace Core {

class IMode;
class RightPaneWidget;

// TODO: The right pane works only for the help plugin atm.
// It can't cope with more than one plugin publishing objects they want in the right pane
// For that the API would need to be different. (Might be that instead of adding objects
// to the pool, there should be a method RightPaneWidget::setWidget(QWidget *w)
// Anyway if a second plugin wants to show something there, redesign this API
class CORE_EXPORT RightPanePlaceHolder : public QWidget
{
    friend class Core::RightPaneWidget;
    Q_OBJECT
public:
    RightPanePlaceHolder(Core::IMode *mode, QWidget *parent = 0);
    ~RightPanePlaceHolder();
    static RightPanePlaceHolder *current();
private slots:
    void currentModeChanged(Core::IMode *);
private:
    void applyStoredSize(int width);
    Core::IMode *m_mode;
    static RightPanePlaceHolder* m_current;
};

class CORE_EXPORT BaseRightPaneWidget : public QObject
{
    Q_OBJECT
public:
    BaseRightPaneWidget(QWidget *widget);
    ~BaseRightPaneWidget();
    QWidget *widget() const;
private:
    QWidget *m_widget;
};

class CORE_EXPORT RightPaneWidget : public QWidget
{
    Q_OBJECT
public:
    RightPaneWidget();
    ~RightPaneWidget();

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    bool isShown();
    void setShown(bool b);

    static RightPaneWidget* instance();

    int storedWidth();
protected:
    void resizeEvent(QResizeEvent *);
private slots:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

private:
    bool m_shown;
    int m_width;
    static RightPaneWidget *m_instance;
};

} // namespace Core

#endif // RIGHTPANE_H
