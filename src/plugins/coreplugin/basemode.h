/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASEMODE_H
#define BASEMODE_H

#include "core_global.h"
#include "imode.h"

#include <QtCore/QObject>

#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT BaseMode
  : public IMode
{
    Q_OBJECT

public:
    BaseMode(QObject *parent = 0);
    ~BaseMode();

    // IMode
    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    int priority() const { return m_priority; }
    QWidget *widget() { return m_widget; }
    QString id() const { return m_id; }
    QString type() const { return m_type; }
    Context context() const { return m_context; }
    QString contextHelpId() const { return m_helpId; }

    void setDisplayName(const QString &name) { m_displayName = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setPriority(int priority) { m_priority = priority; }
    void setWidget(QWidget *widget) { m_widget = widget; }
    void setId(const QString &id) { m_id = id; }
    void setType(const QString &type) { m_type = type; }
    void setContextHelpId(const QString &helpId) { m_helpId = helpId; }
    void setContext(const Context &context) { m_context = context; }

private:
    QString m_displayName;
    QIcon m_icon;
    int m_priority;
    QWidget *m_widget;
    QString m_id;
    QString m_type;
    QString m_helpId;
    Context m_context;
};

} // namespace Core

#endif // BASEMODE_H
