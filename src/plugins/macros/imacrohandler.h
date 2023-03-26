// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Macros {
namespace Internal {

class Macro;
class MacroEvent;
class MacroManager;

class IMacroHandler: public QObject
{
    Q_OBJECT

public:
    virtual void startRecording(Macro* macro);
    virtual void endRecordingMacro(Macro* macro);

    virtual bool canExecuteEvent(const MacroEvent &macroEvent) = 0;
    virtual bool executeEvent(const MacroEvent &macroEvent) = 0;

protected:
    void addMacroEvent(const MacroEvent& event);

    void setCurrentMacro(Macro *macro);

    bool isRecording() const;

private:
    friend class MacroManager;

    Macro *m_currentMacro = nullptr;
};

} // namespace Internal
} // namespace Macros
