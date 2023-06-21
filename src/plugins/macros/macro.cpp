// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macro.h"
#include "macroevent.h"

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

Macro::MacroPrivate::MacroPrivate()
    : version(QCoreApplication::applicationVersion())
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
    if (!d->events.isEmpty())
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
    Utils::FileSaver saver(Utils::FilePath::fromString(fileName));
    if (!saver.hasError()) {
        QDataStream stream(saver.file());
        stream << d->version;
        stream << d->description;
        for (const MacroEvent &event : std::as_const(d->events)) {
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
