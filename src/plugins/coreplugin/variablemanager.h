/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include "core_global.h"

#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace Utils {
class AbstractMacroExpander;
}

namespace Core {

class VariableManagerPrivate;

class CORE_EXPORT VariableManager : public QObject
{
    Q_OBJECT

public:
    VariableManager();
    ~VariableManager();

    static VariableManager *instance();

    static void insert(const QByteArray &variable, const QString &value);
    static bool remove(const QByteArray &variable);
    static QString value(const QByteArray &variable, bool *found = 0);

    static QString expandedString(const QString &stringWithVariables);
    static Utils::AbstractMacroExpander *macroExpander();

    static void registerVariable(const QByteArray &variable,
                          const QString &description);

    static void registerFileVariables(const QByteArray &prefix,
                              const QString &heading);
    static bool isFileVariable(const QByteArray &variable, const QByteArray &prefix);
    static QString fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                              const QString &fileName);
    static QString fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                              const QFileInfo &fileInfo);

    static QList<QByteArray> variables();
    static QString variableDescription(const QByteArray &variable);

signals:
    void variableUpdateRequested(const QByteArray &variable);
};

} // namespace Core

#endif // VARIABLEMANAGER_H
