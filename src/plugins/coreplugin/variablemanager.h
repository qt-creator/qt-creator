/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Utils {
class AbstractMacroExpander;
}

namespace Core {
class VariableManagerPrivate;

class CORE_EXPORT VariableManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VariableManager)
public:
    VariableManager();
    ~VariableManager();

    static VariableManager* instance();

    void insert(const QString &variable, const QString &value);
    bool remove(const QString &variable);
    QString value(const QString &variable, bool *found = 0);
    QString value(const QString &variable, const QString &defaultValue);
    Utils::AbstractMacroExpander *macroExpander();

    void registerVariable(const QString &variable,
                          const QString &description);
    QList<QString> variables() const;
    QString variableDescription(const QString &variable) const;

signals:
    void variableUpdateRequested(const QString &variable);

private:
    QScopedPointer<VariableManagerPrivate> d;
};

} // namespace Core

#endif // VARIABLEMANAGER_H
