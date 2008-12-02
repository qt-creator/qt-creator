/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef INTERFACE_WRAP_HELPERS_H
#define INTERFACE_WRAP_HELPERS_H

#include <extensionsystem/ExtensionSystemInterfaces>
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
