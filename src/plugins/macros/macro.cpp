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

#include "macro.h"
#include "macroevent.h"

#include <app/app_version.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QDataStream>

using namespace Macros::Internal;

/*!
    \class Macros::Macro

    \brief The Macro class represents a macro, which is more or less a list of
    Macros::MacroEvent.

    A macro is a list of events that can be replayed in \QC. A macro has
    an header consisting of the \QC version where the macro was created
    and a description.
    The name of the macro is the filename without the extension.
*/

class Macro::MacroPrivate
{
public:
    MacroPrivate();

    QString description;
    QString version;
    QString fileName;
    QList<MacroEvent> events;
};

Macro::MacroPrivate::MacroPrivate() :
    version(QLatin1String(Core::Constants::IDE_VERSION_LONG))
{
}



// ---------- Macro ------------

Macro::Macro() :
    d(new MacroPrivate)
{
}

Macro::Macro(const Macro &other):
    d(new MacroPrivate)
{
    d->description = other.d->description;
    d->version = other.d->version;
    d->fileName = other.d->fileName;
    d->events = other.d->events;
}

Macro::~Macro()
{
    delete d;
}

Macro& Macro::operator=(const Macro &other)
{
    if (this == &other)
        return *this;
    d->description = other.d->description;
    d->version = other.d->version;
    d->fileName = other.d->fileName;
    d->events = other.d->events;
    return *this;
}

bool Macro::load(QString fileName)
{
    if (d->events.count())
        return true; // the macro is not empty

    // Take the current filename if the parameter is null
    if (fileName.isNull())
        fileName = d->fileName;
    else
        d->fileName = fileName;

    // Load all the macroevents
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QDataStream stream(&file);
        stream >> d->version;
        stream >> d->description;
        while (!stream.atEnd()) {
            MacroEvent macroEvent;
            macroEvent.load(stream);
            append(macroEvent);
        }
        return true;
    }
    return false;
}

bool Macro::loadHeader(const QString &fileName)
{
    d->fileName = fileName;
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QDataStream stream(&file);
        stream >> d->version;
        stream >> d->description;
        return true;
    }
    return false;
}

bool Macro::save(const QString &fileName, QWidget *parent)
{
    Utils::FileSaver saver(fileName);
    if (!saver.hasError()) {
        QDataStream stream(saver.file());
        stream << d->version;
        stream << d->description;
        foreach (const MacroEvent &event, d->events) {
            event.save(stream);
        }
        saver.setResult(&stream);
    }
    if (!saver.finalize(parent))
        return false;
    d->fileName = fileName;
    return true;
}

QString Macro::displayName() const
{
    QFileInfo fileInfo(d->fileName);
    return fileInfo.baseName();
}

void Macro::append(const MacroEvent &event)
{
    d->events.append(event);
}

const QString &Macro::description() const
{
    return d->description;
}

void Macro::setDescription(const QString &text)
{
    d->description = text;
}

const QString &Macro::version() const
{
    return d->version;
}

const QString &Macro::fileName() const
{
    return d->fileName;
}

const QList<MacroEvent> &Macro::events() const
{
    return d->events;
}

bool Macro::isWritable() const
{
    QFileInfo fileInfo(d->fileName);
    return fileInfo.exists() && fileInfo.isWritable();
}
