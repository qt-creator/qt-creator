/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Macros {
namespace Internal {

class MacroEvent;

class Macro
{
public:
    Macro();
    Macro(const Macro& other);
    ~Macro();
    Macro& operator=(const Macro& other);

    bool load(QString fileName = QString());
    bool loadHeader(const QString &fileName);
    bool save(const QString &fileName, QWidget *parent);

    const QString &description() const;
    void setDescription(const QString &text);

    const QString &version() const;
    const QString &fileName() const;
    QString displayName() const;

    void append(const MacroEvent &event);
    const QList<MacroEvent> &events() const;

    bool isWritable() const;

private:
    class MacroPrivate;
    MacroPrivate* d;
};

} // namespace Internal
} // namespace Macros
