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

#ifndef INTERFACE_WRAP_HELPERS_H
#define INTERFACE_WRAP_HELPERS_H

#include <QtScript/QScriptEngine>

namespace SharedTools {

// Convert a QObjectInterface to Scriptvalue
// To be registered as a magic creation function with qScriptRegisterMetaType().
// (see registerQObjectInterface)

template <class QObjectInterface>
static QScriptValue qObjectInterfaceToScriptValue(QScriptEngine *engine, QObjectInterface* const &qoif)
{
    if (!qoif)
        return  QScriptValue(engine, QScriptValue::NullValue);

    QObject *qObject = const_cast<QObjectInterface *>(qoif);

    const QScriptEngine::QObjectWrapOptions wrapOptions =
        QScriptEngine::ExcludeChildObjects|QScriptEngine::ExcludeSuperClassMethods|QScriptEngine::ExcludeSuperClassProperties;
    return engine->newQObject(qObject, QScriptEngine::QtOwnership, wrapOptions);
}

// Convert  Scriptvalue back to  QObjectInterface
// To be registered as a magic conversion function with  qScriptRegisterMetaType().
// (see registerQObjectInterface)

template <class QObjectInterface>
static void scriptValueToQObjectInterface(const QScriptValue &sv, QObjectInterface *&p)
{
    QObject *qObject =  sv.toQObject();
    p = qobject_cast<QObjectInterface*>(qObject);
}

// Magically register a Workbench interface derived from
// ExtensionSystem::QObjectInterface class with the engine.
// To avoid lifecycle issues, the script value is created on the QObject returned
// by ExtensionSystem::QObjectInterface::qObject() and given the specified
// prototype. By convention, ExtensionSystem::QObjectInterface::qObject() returns an
// QObject that implements the interface, so it can be casted to it.

template <class QObjectInterface, class Prototype>
static void registerQObjectInterface(QScriptEngine &engine)
{
    Prototype *protoType = new Prototype(&engine);
    const QScriptValue scriptProtoType = engine.newQObject(protoType);

    const int metaTypeId = qScriptRegisterMetaType<QObjectInterface*>(
        &engine,
        qObjectInterfaceToScriptValue<QObjectInterface>,
        scriptValueToQObjectInterface<QObjectInterface>,
        scriptProtoType);
    Q_UNUSED(metaTypeId);
}

} // namespace SharedTools

#endif // INTERFACE_WRAP_HELPERS_H
