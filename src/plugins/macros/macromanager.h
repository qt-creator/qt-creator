// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QMap>

namespace Macros {
namespace Internal {

class IMacroHandler;
class Macro;
class MacroOptionsWidget;

class MacroManager : public QObject
{
    Q_OBJECT

public:
    MacroManager();
    ~MacroManager() override;

    static MacroManager *instance();

    static const QMap<QString, Macro *> &macros();

    static void registerMacroHandler(IMacroHandler *handler);

    static QString macrosDirectory();

    void startMacro();
    void executeLastMacro();
    void saveLastMacro();
    bool executeMacro(const QString &name);
    void endMacro();

protected:
    friend class Internal::MacroOptionsWidget;

    void deleteMacro(const QString &name);
    void changeMacro(const QString &name, const QString &description);

private:
    class MacroManagerPrivate *d;
};

} // namespace Internal
} // namespace Macros
