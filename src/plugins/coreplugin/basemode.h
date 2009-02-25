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
