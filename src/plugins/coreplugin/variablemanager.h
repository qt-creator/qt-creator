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

#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QString>

namespace Core {

class CORE_EXPORT VariableManager : public QObject
{
    Q_OBJECT

public:
    VariableManager(QObject *parent);
    ~VariableManager();

    static VariableManager* instance() { return m_instance; }

    void insert(const QString &variable, const QString &value);
    QString value(const QString &variable);
    QString value(const QString &variable, const QString &defaultValue);
    void remove(const QString &variable);
    QString resolve(const QString &stringWithVariables);

private:
    QMap<QString, QString> m_map;
    static VariableManager *m_instance;
};

} // namespace Core

#endif // VARIABLEMANAGER_H
