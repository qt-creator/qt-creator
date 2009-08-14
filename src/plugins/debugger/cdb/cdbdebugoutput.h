/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_CDBOUTPUT_H
#define DEBUGGER_CDBOUTPUT_H

#include "cdbcom.h"

#include <QtCore/QObject>

namespace Debugger {
namespace Internal {

// CdbDebugOutputBase is a base class for output handlers
// that takes care of the Active X magic and conversion to QString.

class CdbDebugOutputBase : public IDebugOutputCallbacksWide
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG mask,
        IN PCWSTR text
        );

    // Helpers to retrieve the output callbacks IF
    static IDebugOutputCallbacksWide *getOutputCallback(CIDebugClient *client);

protected:
    CdbDebugOutputBase();
    virtual void output(ULONG mask, const QString &message) = 0;
};

// Standard CDB output handler
class CdbDebugOutput : public QObject, public CdbDebugOutputBase
{
    Q_OBJECT
public:
    CdbDebugOutput();

protected:
    virtual void output(ULONG mask, const QString &message);

signals:
    void debuggerOutput(int channel, const QString &message);
    void debuggerInputPrompt(int channel, const QString &message);
    void debuggeeOutput(const QString &message);
    void debuggeeInputPrompt(const QString &message);
};

// An output handler that adds lines to a string (to be
// used for cases in which linebreaks occur in-between calls
// to output).
class StringOutputHandler : public CdbDebugOutputBase
{
public:
    StringOutputHandler() {}
    QString result() const { return m_result; }

protected:
    virtual void output(ULONG, const QString &message) { m_result += message; }

private:
    QString m_result;
};

// Utility class to temporarily redirect output to another handler
// as long as in scope
class OutputRedirector {
    Q_DISABLE_COPY(OutputRedirector)
public:
    explicit OutputRedirector(CIDebugClient *client, IDebugOutputCallbacksWide *newHandler);
    ~OutputRedirector();
private:
    CIDebugClient *m_client;
    IDebugOutputCallbacksWide *m_oldHandler;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBOUTPUT_H
