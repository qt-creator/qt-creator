/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "scriptmanager_p.h"
#include "qworkbench_wrapper.h"
#include "metatypedeclarations.h"

#include <utils/qtcassert.h>
#include <interface_wrap_helpers.h>
#include <wrap_helpers.h>

#include <limits.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>
#include <QtGui/QStatusBar>

namespace {
    enum { debugQWorkbenchWrappers = 0 };
}

// Script function template to pop up a message box
// with a certain icon and buttons.
template <int MsgBoxIcon, int MsgBoxButtons>
        static QScriptValue messageBox(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() < 3)
        return QScriptValue(engine, -1);

    QWidget *parent = qscriptvalue_cast<QWidget *>(context->argument(0));
    const QString title = context->argument(1).toString();
    const QString msg = context->argument(2).toString();

    QMessageBox msgBox(static_cast<QMessageBox::Icon>(MsgBoxIcon),  title, msg,
                       static_cast<QMessageBox::StandardButtons>(MsgBoxButtons), parent);

    return QScriptValue(engine,  msgBox.exec());
}

static QScriptValue inputDialogGetText(QScriptContext *context, QScriptEngine *engine)
{
    const int argumentCount = context->argumentCount();
    if (argumentCount < 3)
        return QScriptValue(engine, QScriptValue::NullValue);

    QWidget *parent = qscriptvalue_cast<QWidget *>(context->argument(0));
    const QString title = context->argument(1).toString();
    const QString label = context->argument(2).toString();
    const QString defaultValue = argumentCount > 3 ? context->argument(3).toString() : QString();

    bool ok;
    const QString rc = QInputDialog::getText(parent, title, label, QLineEdit::Normal, defaultValue, &ok);
    if (!ok)
        return QScriptValue(engine, QScriptValue::NullValue);
    return QScriptValue(engine, rc);
}

static QScriptValue inputDialogGetInteger(QScriptContext *context, QScriptEngine *engine)
{
    const int argumentCount = context->argumentCount();
    if (argumentCount < 3)
        return QScriptValue(engine, QScriptValue::NullValue);

    QWidget *parent = qscriptvalue_cast<QWidget *>(context->argument(0));
    const QString title = context->argument(1).toString();
    const QString label = context->argument(2).toString();
    const int defaultValue = argumentCount > 3 ? context->argument(3).toInt32() : 0;
    const int minValue = argumentCount > 4 ? context->argument(4).toInt32() : INT_MIN;
    const int maxValue = argumentCount > 5 ? context->argument(5).toInt32() : INT_MAX;

    bool ok;
    const int rc = QInputDialog::getInteger(parent, title, label,  defaultValue, minValue, maxValue, 1, &ok);
    if (!ok)
        return QScriptValue(engine, QScriptValue::NullValue);
    return QScriptValue(engine, rc);
}

static QScriptValue inputDialogGetDouble(QScriptContext *context, QScriptEngine *engine)
{
    const int argumentCount = context->argumentCount();
    if (argumentCount < 3)
        return QScriptValue(engine, QScriptValue::NullValue);

    QWidget *parent = qscriptvalue_cast<QWidget *>(context->argument(0));
    const QString title = context->argument(1).toString();
    const QString label = context->argument(2).toString();
    const double defaultValue = argumentCount > 3 ? context->argument(3).toNumber() : 0;
    // Use QInputDialog defaults
    const double minValue = argumentCount > 4 ? context->argument(4).toNumber() : INT_MIN;
    const double maxValue = argumentCount > 5 ? context->argument(5).toNumber() : INT_MAX;

    bool ok;
    const double rc = QInputDialog::getDouble(parent, title, label,  defaultValue, minValue, maxValue, 1, &ok);
    if (!ok)
        return QScriptValue(engine, QScriptValue::NullValue);
    return QScriptValue(engine, rc);
}

static QScriptValue inputDialogGetItem(QScriptContext *context, QScriptEngine *engine)
{
    const int argumentCount = context->argumentCount();
    if (argumentCount < 4)
        return QScriptValue(engine, QScriptValue::NullValue);

    QWidget *parent = qscriptvalue_cast<QWidget *>(context->argument(0));
    const QString title = context->argument(1).toString();
    const QString label = context->argument(2).toString();
    const QStringList items = qscriptvalue_cast<QStringList>(context->argument(3));
    const int defaultItem = argumentCount > 4 ? context->argument(4).toInt32() : 0;
    const bool editable   = argumentCount > 5 ? context->argument(5).toInt32() : 0;

    bool ok;
    const QString rc = QInputDialog::getItem (parent, title, label, items, defaultItem, editable, &ok);
    if (!ok)
        return QScriptValue(engine, QScriptValue::NullValue);

    return QScriptValue(engine, rc);
}

// Script function template to pop up a file box
// with a certain icon and buttons.
template <int TAcceptMode, int TFileMode>
static QScriptValue fileBox(QScriptContext *context, QScriptEngine *engine)
{
    const int argumentCount = context->argumentCount();
    if (argumentCount < 2)
        return QScriptValue(engine, QScriptValue::NullValue);

    QWidget *parent = qscriptvalue_cast<QWidget *>(context->argument(0));
    const QString title     = context->argument(1).toString();
    const QString directory = argumentCount > 2 ? context->argument(2).toString() : QString();
    const QString filter    = argumentCount > 3 ? context->argument(3).toString() : QString();
    QFileDialog fileDialog(parent, title, directory, filter);
    fileDialog.setAcceptMode(static_cast<QFileDialog::AcceptMode>(TAcceptMode));
    fileDialog.setFileMode (static_cast<QFileDialog::FileMode>(TFileMode));
    if (fileDialog.exec() == QDialog::Rejected)
        return  QScriptValue(engine, QScriptValue::NullValue);
    const QStringList rc = fileDialog.selectedFiles();
    QTC_ASSERT(!rc.empty(), /**/);
    return TFileMode == QFileDialog::ExistingFiles ?
        engine->toScriptValue(rc) : engine->toScriptValue(rc.front());
}

// ------ ScriptManagerPrivate

namespace Core {
namespace Internal {

ScriptManagerPrivate::ScriptManagerPrivate(QObject *parent)
   : ScriptManager(parent), m_initialized(false)
{
}

QScriptEngine &ScriptManagerPrivate::scriptEngine()
{
    ensureEngineInitialized();
    return m_engine;
}

// Split a backtrace of the form:
// "<anonymous>(BuildManagerCommand(ls))@:0
// demoProjectExplorer()@:237
// <anonymous>()@:276
// <global>()@:0"
static void parseBackTrace(const QStringList &backTrace, ScriptManagerPrivate::Stack &stack)
{
    const QChar at = QLatin1Char('@');
    const QChar colon = QLatin1Char(':');
    stack.clear();
    foreach (const QString &line, backTrace) {
        const int atPos = line.lastIndexOf(at);
        if (atPos == -1)
            continue;
        const int colonPos = line.indexOf(colon, atPos + 1);
        if (colonPos == -1)
            continue;

        ScriptManagerPrivate::StackFrame frame;
        frame.function = line.left(atPos);
        frame.fileName = line.mid(atPos + 1, colonPos - atPos - 1);
        frame.lineNumber = line.right(line.size() -  colonPos - 1).toInt();
        stack.push_back(frame);
    }
}

bool ScriptManagerPrivate::runScript(const QString &script, QString *errorMessage)
{
    Stack stack;
    return runScript(script, errorMessage, &stack);
}

bool ScriptManagerPrivate::runScript(const QString &script, QString *errorMessage, Stack *stack)
{
    ensureEngineInitialized();
    stack->clear();

    m_engine.pushContext();
    m_engine.evaluate(script);

    const bool failed = m_engine.hasUncaughtException ();
    if (failed) {
        const int errorLineNumber = m_engine.uncaughtExceptionLineNumber();
        const QStringList backTrace = m_engine.uncaughtExceptionBacktrace();
        parseBackTrace(backTrace, *stack);
        const QString backtrace = backTrace.join(QString(QLatin1Char('\n')));
        *errorMessage = QObject::tr("Exception at line %1: %2\n%3").arg(errorLineNumber).arg(engineError(m_engine)).arg(backtrace);
    }
    m_engine.popContext();
    return !failed;
}

void ScriptManagerPrivate::ensureEngineInitialized()
{
    if (m_initialized)
        return;
    // register QObjects that occur as properties
    SharedTools::registerQObject<QMainWindow>(m_engine);
    SharedTools::registerQObject<QStatusBar>(m_engine);
    SharedTools::registerQObject<QToolBar>(m_engine);
    SharedTools::registerQObject<QSettings>(m_engine);
    // WB interfaces
//    SharedTools::registerQObjectInterface<Core::MessageManager, MessageManagerPrototype>(m_engine);

//    SharedTools::registerQObjectInterface<Core::IFile, FilePrototype>(m_engine);
//    qScriptRegisterSequenceMetaType<QList<Core::IFile *> >(&m_engine);
//    SharedTools::registerQObjectInterface<Core::FileManager, FileManagerPrototype>(m_engine);

//    SharedTools::registerQObjectInterface<Core::IEditor, EditorPrototype>(m_engine);
    qScriptRegisterSequenceMetaType<QList<Core::IEditor *> >(&m_engine);

//    SharedTools::registerQObjectInterface<Core::EditorGroup, EditorGroupPrototype>(m_engine);
    qScriptRegisterSequenceMetaType<QList<Core::EditorGroup *> >(&m_engine);

    SharedTools::registerQObjectInterface<Core::EditorManager, EditorManagerPrototype>(m_engine);

//    SharedTools::registerQObjectInterface<Core::ICore, CorePrototype>(m_engine);

    // Make "core" available
    m_engine.globalObject().setProperty(QLatin1String("core"), qScriptValueFromValue(&m_engine, Core::ICore::instance()));

    // CLASSIC:  registerInterfaceWithDefaultPrototype<Core::MessageManager, MessageManagerPrototype>(m_engine);

    // Message box conveniences
    m_engine.globalObject().setProperty(QLatin1String("critical"),
                                        m_engine.newFunction(messageBox<QMessageBox::Critical, QMessageBox::Ok>, 3));
    m_engine.globalObject().setProperty(QLatin1String("warning"),
                                        m_engine.newFunction(messageBox<QMessageBox::Warning, QMessageBox::Ok>, 3));
    m_engine.globalObject().setProperty(QLatin1String("information"),
                                        m_engine.newFunction(messageBox<QMessageBox::Information, QMessageBox::Ok>, 3));
    // StandardButtons has overloaded operator '|' - grrr.
    enum { MsgBoxYesNo = 0x00014000 };
    m_engine.globalObject().setProperty(QLatin1String("yesNoQuestion"),
                                        m_engine.newFunction(messageBox<QMessageBox::Question, MsgBoxYesNo>, 3));

    m_engine.globalObject().setProperty(QLatin1String("getText"), m_engine.newFunction(inputDialogGetText, 3));
    m_engine.globalObject().setProperty(QLatin1String("getInteger"), m_engine.newFunction(inputDialogGetInteger, 3));
    m_engine.globalObject().setProperty(QLatin1String("getDouble"), m_engine.newFunction(inputDialogGetDouble, 3));
    m_engine.globalObject().setProperty(QLatin1String("getItem"), m_engine.newFunction(inputDialogGetItem, 3));

    // file box
    m_engine.globalObject().setProperty(QLatin1String("getOpenFileNames"), m_engine.newFunction(fileBox<QFileDialog::AcceptOpen, QFileDialog::ExistingFiles> , 2));
    m_engine.globalObject().setProperty(QLatin1String("getOpenFileName"), m_engine.newFunction(fileBox<QFileDialog::AcceptOpen, QFileDialog::ExistingFile> , 2));
    m_engine.globalObject().setProperty(QLatin1String("getSaveFileName"), m_engine.newFunction(fileBox<QFileDialog::AcceptSave, QFileDialog::AnyFile> , 2));
    m_engine.globalObject().setProperty(QLatin1String("getExistingDirectory"), m_engine.newFunction(fileBox<QFileDialog::AcceptSave, QFileDialog::DirectoryOnly> , 2));
    m_initialized = true;
}

QString ScriptManagerPrivate::engineError(QScriptEngine &scriptEngine)
{
    QScriptValue error = scriptEngine.evaluate(QLatin1String("Error"));
    if (error.isValid())
        return error.toString();
    return QObject::tr("Unknown error");
}

} // namespace Internal
} // namespace Core
