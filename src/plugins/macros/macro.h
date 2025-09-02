// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>

namespace Macros::Internal {

class MacroEvent;

class Macro
{
public:
    Macro();
    Macro(const Macro& other);
    ~Macro();
    Macro& operator=(const Macro& other);

    bool load();
    bool loadHeader(const QString &fileName);
    bool save(const QString &fileName);

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

} // namespace Macros::Internal
