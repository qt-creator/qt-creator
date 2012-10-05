/****************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Konstantin Tokarev.
**
** Contact: http://www.qt-project.org/
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef COMMANDBUTTON_H
#define COMMANDBUTTON_H

#include <coreplugin/core_global.h>

#include <QPointer>
#include <QString>
#include <QToolButton>

namespace Core {

class Command;
class Id;

class CORE_EXPORT CommandButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QString toolTipBase READ toolTipBase WRITE setToolTipBase)
public:
    explicit CommandButton(QWidget *parent = 0);
    explicit CommandButton(Id id, QWidget *parent = 0);
    void setCommandId(Id id);
    QString toolTipBase() const;
    void setToolTipBase(const QString &toolTipBase);

private slots:
    void updateToolTip();

private:
    QPointer<Core::Command> m_command;
    QString m_toolTipBase;
};

}

#endif // COMMANDBUTTON_H
