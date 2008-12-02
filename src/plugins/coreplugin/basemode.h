/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef BASEMODE_H
#define BASEMODE_H

#include "core_global.h"

#include "imode.h"

#include <QtCore/QObject>
#include <QtGui/QWidget>

namespace Core {

class CORE_EXPORT BaseMode
  : public IMode
{
    Q_OBJECT

public:
    BaseMode(QObject *parent = 0);
    BaseMode(const QString &name,
             const char * uniqueModeName,
             const QIcon &icon,
             int priority,
             QWidget *widget,
             QObject *parent = 0);
    ~BaseMode();

    // IMode
    QString name() const { return m_name; }
    QIcon icon() const { return m_icon; }
    int priority() const { return m_priority; }
    QWidget *widget() { return m_widget; }
    const char *uniqueModeName() const { return m_uniqueModeName; }
    QList<int> context() const { return m_context; }

    void setName(const QString &name) { m_name = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setPriority(int priority) { m_priority = priority; }
    void setWidget(QWidget *widget) { m_widget = widget; }
    void setUniqueModeName(const char *uniqueModeName) { m_uniqueModeName = uniqueModeName; }
    void setContext(const QList<int> &context) { m_context = context; }

private:
    QString m_name;
    QIcon m_icon;
    int m_priority;
    QWidget *m_widget;
    const char * m_uniqueModeName;
    QList<int> m_context;
};

} // namespace Core

#endif // BASEMODE_H
