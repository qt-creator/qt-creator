/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

// WARNING: This code is shared with the qmlplugindump tool code in Qt.
//          Modifications to this file need to be applied there.

#include <QtDeclarative>
#include <private/qdeclarativemetatype_p.h>
#include <private/qdeclarativeopenmetaobject_p.h>
#include <QDeclarativeView>

#include <QApplication>

#include <QSet>
#include <QMetaObject>
#include <QMetaProperty>
#include <QDebug>
#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>

#include <iostream>

#include "qmlstreamwriter.h"

#ifdef QT_SIMULATOR
#include <private/qsimulatorconnection_p.h>
#endif

#ifdef Q_OS_UNIX
#include <signal.h>
#endif

QString pluginImportPath;
bool verbose = false;

QString currentProperty;
QString inObjectInstantiation;

void collectReachableMetaObjects(const QMetaObject *meta, QSet<const QMetaObject *> *metas)
{
    if (! meta || metas->contains(meta))
        return;

    // dynamic meta objects break things badly, so just ignore them
    const QMetaObjectPrivate *mop = reinterpret_cast<const QMetaObjectPrivate *>(meta->d.data);
    if (!(mop->flags & DynamicMetaObject))
        metas->insert(meta);

    collectReachableMetaObjects(meta->superClass(), metas);
}

void collectReachableMetaObjects(QObject *object, QSet<const QMetaObject *> *metas)
{
    if (! object)
        return;

    const QMetaObject *meta = object->metaObject();
    if (verbose)
        qDebug() << "Processing object" << meta->className();
    collectReachableMetaObjects(meta, metas);

    for (int index = 0; index < meta->propertyCount(); ++index) {
        QMetaProperty prop = meta->property(index);
        if (QDeclarativeMetaType::isQObject(prop.userType())) {
            if (verbose)
                qDebug() << "  Processing property" << prop.name();
            currentProperty = QString("%1::%2").arg(meta->className(), prop.name());

            // if the property was not initialized during construction,
            // accessing a member of oo is going to cause a segmentation fault
            QObject *oo = QDeclarativeMetaType::toQObject(prop.read(object));
            if (oo && !metas->contains(oo->metaObject()))
                collectReachableMetaObjects(oo, metas);
            currentProperty.clear();
        }
    }
}

void collectReachableMetaObjects(const QDeclarativeType *ty, QSet<const QMetaObject *> *metas)
{
    collectReachableMetaObjects(ty->metaObject(), metas);
    if (ty->attachedPropertiesType())
        collectReachableMetaObjects(ty->attachedPropertiesType(), metas);
}

/* We want to add the MetaObject for 'Qt' to the list, this is a
   simple way to access it.
*/
class FriendlyQObject: public QObject
{
public:
    static const QMetaObject *qtMeta() { return &staticQtMetaObject; }
};

/* When we dump a QMetaObject, we want to list all the types it is exported as.
   To do this, we need to find the QDeclarativeTypes associated with this
   QMetaObject.
*/
static QHash<QByteArray, QSet<const QDeclarativeType *> > qmlTypesByCppName;

static QHash<QByteArray, QByteArray> cppToId;

/* Takes a C++ type name, such as Qt::LayoutDirection or QString and
   maps it to how it should appear in the description file.

   These names need to be unique globally, so we don't change the C++ symbol's
   name much. It is mostly used to for explicit translations such as
   QString->string and translations for extended QML objects.
*/
QByteArray convertToId(const QByteArray &cppName)
{
    return cppToId.value(cppName, cppName);
}

QByteArray convertToId(const QMetaObject *mo)
{
    QByteArray className(mo->className());
    if (!className.isEmpty())
        return convertToId(className);

    // likely a metaobject generated for an extended qml object
    if (mo->superClass()) {
        className = convertToId(mo->superClass());
        className.append("_extended");
        return className;
    }

    static QHash<const QMetaObject *, QByteArray> generatedNames;
    className = generatedNames.value(mo);
    if (!className.isEmpty())
        return className;

    qWarning() << "Found a QMetaObject without a className, generating a random name";
    className = QByteArray("error-unknown-name-");
    className.append(QByteArray::number(generatedNames.size()));
    generatedNames.insert(mo, className);
    return className;
}

QSet<const QMetaObject *> collectReachableMetaObjects(const QList<QDeclarativeType *> &skip = QList<QDeclarativeType *>())
{
    QSet<const QMetaObject *> metas;
    metas.insert(FriendlyQObject::qtMeta());

    QHash<QByteArray, QSet<QByteArray> > extensions;
    foreach (const QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        qmlTypesByCppName[ty->metaObject()->className()].insert(ty);
        if (ty->isExtendedType()) {
            extensions[ty->typeName()].insert(ty->metaObject()->className());
        }
        collectReachableMetaObjects(ty, &metas);
    }

    // Adjust exports of the base object if there are extensions.
    // For each export of a base object there can be a single extension object overriding it.
    // Example: QDeclarativeGraphicsWidget overrides the QtQuick/QGraphicsWidget export
    //          of QGraphicsWidget.
    foreach (const QByteArray &baseCpp, extensions.keys()) {
        QSet<const QDeclarativeType *> baseExports = qmlTypesByCppName.value(baseCpp);

        const QSet<QByteArray> extensionCppNames = extensions.value(baseCpp);
        foreach (const QByteArray &extensionCppName, extensionCppNames) {
            const QSet<const QDeclarativeType *> extensionExports = qmlTypesByCppName.value(extensionCppName);

            // remove extension exports from base imports
            // unfortunately the QDeclarativeType pointers don't match, so can't use QSet::substract
            QSet<const QDeclarativeType *> newBaseExports;
            foreach (const QDeclarativeType *baseExport, baseExports) {
                bool match = false;
                foreach (const QDeclarativeType *extensionExport, extensionExports) {
                    if (baseExport->qmlTypeName() == extensionExport->qmlTypeName()
                            && baseExport->majorVersion() == extensionExport->majorVersion()
                            && baseExport->minorVersion() == extensionExport->minorVersion()) {
                        match = true;
                        break;
                    }
                }
                if (!match)
                    newBaseExports.insert(baseExport);
            }
            baseExports = newBaseExports;
        }
        qmlTypesByCppName[baseCpp] = baseExports;
    }

    // find even more QMetaObjects by instantiating QML types and running
    // over the instances
    foreach (QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        if (skip.contains(ty))
            continue;
        if (ty->isExtendedType())
            continue;
        if (!ty->isCreatable())
            continue;
        if (ty->typeName() == "QDeclarativeComponent")
            continue;

        QByteArray tyName = ty->qmlTypeName();
        tyName = tyName.mid(tyName.lastIndexOf('/') + 1);
        if (tyName.isEmpty())
            continue;

        inObjectInstantiation = tyName;
        QObject *object = ty->create();
        inObjectInstantiation.clear();

        if (object)
            collectReachableMetaObjects(object, &metas);
        else
            qWarning() << "Could not create" << tyName;
    }

    return metas;
}


class Dumper
{
    QmlStreamWriter *qml;
    QString relocatableModuleUri;

public:
    Dumper(QmlStreamWriter *qml) : qml(qml) {}

    void setRelocatableModuleUri(const QString &uri)
    {
        relocatableModuleUri = uri;
    }

    void dump(const QMetaObject *meta)
    {
        qml->writeStartObject("Component");

        QByteArray id = convertToId(meta);
        qml->writeScriptBinding(QLatin1String("name"), enquote(id));

        for (int index = meta->classInfoCount() - 1 ; index >= 0 ; --index) {
            QMetaClassInfo classInfo = meta->classInfo(index);
            if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
                qml->writeScriptBinding(QLatin1String("defaultProperty"), enquote(QLatin1String(classInfo.value())));
                break;
            }
        }

        if (meta->superClass())
            qml->writeScriptBinding(QLatin1String("prototype"), enquote(convertToId(meta->superClass())));

        QSet<const QDeclarativeType *> qmlTypes = qmlTypesByCppName.value(meta->className());
        if (!qmlTypes.isEmpty()) {
            QStringList exports;

            foreach (const QDeclarativeType *qmlTy, qmlTypes) {
                QString qmlTyName = qmlTy->qmlTypeName();
                // some qmltype names are missing the actual names, ignore that import
                if (qmlTyName.endsWith('/'))
                    continue;
                if (qmlTyName.startsWith(relocatableModuleUri + QLatin1Char('/'))) {
                    qmlTyName.remove(0, relocatableModuleUri.size() + 1);
                }
                if (qmlTyName.startsWith("./")) {
                    qmlTyName.remove(0, 2);
                }
                exports += enquote(QString("%1 %2.%3").arg(
                                       qmlTyName,
                                       QString::number(qmlTy->majorVersion()),
                                       QString::number(qmlTy->minorVersion())));
            }

            // ensure exports are sorted and don't change order when the plugin is dumped again
            exports.removeDuplicates();
            qSort(exports);

            qml->writeArrayBinding(QLatin1String("exports"), exports);

            if (const QMetaObject *attachedType = (*qmlTypes.begin())->attachedPropertiesType()) {
                qml->writeScriptBinding(QLatin1String("attachedType"), enquote(
                                            convertToId(attachedType)));
            }
        }

        for (int index = meta->enumeratorOffset(); index < meta->enumeratorCount(); ++index)
            dump(meta->enumerator(index));

        for (int index = meta->propertyOffset(); index < meta->propertyCount(); ++index)
            dump(meta->property(index));

        for (int index = meta->methodOffset(); index < meta->methodCount(); ++index)
            dump(meta->method(index));

        qml->writeEndObject();
    }

    void writeEasingCurve()
    {
        qml->writeStartObject("Component");
        qml->writeScriptBinding(QLatin1String("name"), enquote(QLatin1String("QEasingCurve")));
        qml->writeScriptBinding(QLatin1String("prototype"), enquote(QLatin1String("QDeclarativeEasingValueType")));
        qml->writeEndObject();
    }

private:
    static QString enquote(const QString &string)
    {
        return QString("\"%1\"").arg(string);
    }

    /* Removes pointer and list annotations from a type name, returning
       what was removed in isList and isPointer
    */
    static void removePointerAndList(QByteArray *typeName, bool *isList, bool *isPointer)
    {
        static QByteArray declListPrefix = "QDeclarativeListProperty<";

        if (typeName->endsWith('*')) {
            *isPointer = true;
            typeName->truncate(typeName->length() - 1);
            removePointerAndList(typeName, isList, isPointer);
        } else if (typeName->startsWith(declListPrefix)) {
            *isList = true;
            typeName->truncate(typeName->length() - 1); // get rid of the suffix '>'
            *typeName = typeName->mid(declListPrefix.size());
            removePointerAndList(typeName, isList, isPointer);
        }

        *typeName = convertToId(*typeName);
    }

    void writeTypeProperties(QByteArray typeName, bool isWritable)
    {
        bool isList = false, isPointer = false;
        removePointerAndList(&typeName, &isList, &isPointer);

        qml->writeScriptBinding(QLatin1String("type"), enquote(typeName));
        if (isList)
            qml->writeScriptBinding(QLatin1String("isList"), QLatin1String("true"));
        if (!isWritable)
            qml->writeScriptBinding(QLatin1String("isReadonly"), QLatin1String("true"));
        if (isPointer)
            qml->writeScriptBinding(QLatin1String("isPointer"), QLatin1String("true"));
    }

    void dump(const QMetaProperty &prop)
    {
        qml->writeStartObject("Property");

        qml->writeScriptBinding(QLatin1String("name"), enquote(QString::fromUtf8(prop.name())));
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 4))
        if (int revision = prop.revision())
            qml->writeScriptBinding(QLatin1String("revision"), QString::number(revision));
#endif
        writeTypeProperties(prop.typeName(), prop.isWritable());

        qml->writeEndObject();
    }

    void dump(const QMetaMethod &meth)
    {
        if (meth.methodType() == QMetaMethod::Signal) {
            if (meth.access() != QMetaMethod::Protected)
                return; // nothing to do.
        } else if (meth.access() != QMetaMethod::Public) {
            return; // nothing to do.
        }

        QByteArray name = meth.signature();
        int lparenIndex = name.indexOf('(');
        if (lparenIndex == -1) {
            return; // invalid signature
        }
        name = name.left(lparenIndex);

        if (meth.methodType() == QMetaMethod::Signal)
            qml->writeStartObject(QLatin1String("Signal"));
        else
            qml->writeStartObject(QLatin1String("Method"));

        qml->writeScriptBinding(QLatin1String("name"), enquote(name));

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 4))
        if (int revision = meth.revision())
            qml->writeScriptBinding(QLatin1String("revision"), QString::number(revision));
#endif

        const QString typeName = convertToId(meth.typeName());
        if (! typeName.isEmpty())
            qml->writeScriptBinding(QLatin1String("type"), enquote(typeName));

        for (int i = 0; i < meth.parameterTypes().size(); ++i) {
            QByteArray argName = meth.parameterNames().at(i);

            qml->writeStartObject(QLatin1String("Parameter"));
            if (! argName.isEmpty())
                qml->writeScriptBinding(QLatin1String("name"), enquote(argName));
            writeTypeProperties(meth.parameterTypes().at(i), true);
            qml->writeEndObject();
        }

        qml->writeEndObject();
    }

    void dump(const QMetaEnum &e)
    {
        qml->writeStartObject(QLatin1String("Enum"));
        qml->writeScriptBinding(QLatin1String("name"), enquote(QString::fromUtf8(e.name())));

        QList<QPair<QString, QString> > namesValues;
        for (int index = 0; index < e.keyCount(); ++index) {
            namesValues.append(qMakePair(enquote(QString::fromUtf8(e.key(index))), QString::number(e.value(index))));
        }

        qml->writeScriptObjectLiteralBinding(QLatin1String("values"), namesValues);
        qml->writeEndObject();
    }
};


enum ExitCode {
    EXIT_INVALIDARGUMENTS = 1,
    EXIT_SEGV = 2,
    EXIT_IMPORTERROR = 3
};

#ifdef Q_OS_UNIX
void sigSegvHandler(int) {
    fprintf(stderr, "Error: SEGV\n");
    if (!currentProperty.isEmpty())
        fprintf(stderr, "While processing the property '%s', which probably has uninitialized data.\n", currentProperty.toLatin1().constData());
    if (!inObjectInstantiation.isEmpty())
        fprintf(stderr, "While instantiating the object '%s'.\n", inObjectInstantiation.toLatin1().constData());
    exit(EXIT_SEGV);
}
#endif

void printUsage(const QString &appName)
{
    qWarning() << qPrintable(QString(
                                 "Usage: %1 [-v] [-notrelocatable] module.uri version [module/import/path]\n"
                                 "       %1 [-v] -path path/to/qmldir/directory [version]\n"
                                 "       %1 [-v] -builtins\n"
                                 "Example: %1 Qt.labs.particles 4.7 /home/user/dev/qt-install/imports").arg(
                                 appName));
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_UNIX
    // qmldump may crash, but we don't want any crash handlers to pop up
    // therefore we intercept the segfault and just exit() ourselves
    struct sigaction sigAction;

    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_handler = &sigSegvHandler;
    sigAction.sa_flags   = 0;

    sigaction(SIGSEGV, &sigAction, 0);
#endif

#ifdef QT_SIMULATOR
    // Running this application would bring up the Qt Simulator (since it links QtGui), avoid that!
    QtSimulatorPrivate::SimulatorConnection::createStubInstance();
#endif
    QApplication app(argc, argv);
    const QStringList args = app.arguments();
    const QString appName = QFileInfo(app.applicationFilePath()).baseName();
    if (args.size() < 2) {
        printUsage(appName);
        return EXIT_INVALIDARGUMENTS;
    }

    QString pluginImportUri;
    QString pluginImportVersion;
    bool relocatable = true;
    enum Action { Uri, Path, Builtins };
    Action action = Uri;
    {
        QStringList positionalArgs;
        foreach (const QString &arg, args) {
            if (!arg.startsWith(QLatin1Char('-'))) {
                positionalArgs.append(arg);
                continue;
            }

            if (arg == QLatin1String("--notrelocatable")
                    || arg == QLatin1String("-notrelocatable")) {
                relocatable = false;
            } else if (arg == QLatin1String("--path")
                       || arg == QLatin1String("-path")) {
                action = Path;
            } else if (arg == QLatin1String("--builtins")
                       || arg == QLatin1String("-builtins")) {
                action = Builtins;
            } else if (arg == QLatin1String("-v")) {
                verbose = true;
            } else {
                qWarning() << "Invalid argument: " << arg;
                return EXIT_INVALIDARGUMENTS;
            }
        }

        if (action == Uri) {
            if (positionalArgs.size() != 3 && positionalArgs.size() != 4) {
                qWarning() << "Incorrect number of positional arguments";
                return EXIT_INVALIDARGUMENTS;
            }
            pluginImportUri = positionalArgs[1];
            pluginImportVersion = positionalArgs[2];
            if (positionalArgs.size() >= 4)
                pluginImportPath = positionalArgs[3];
        } else if (action == Path) {
            if (positionalArgs.size() != 2 && positionalArgs.size() != 3) {
                qWarning() << "Incorrect number of positional arguments";
                return EXIT_INVALIDARGUMENTS;
            }
            pluginImportPath = QDir::fromNativeSeparators(positionalArgs[1]);
            if (positionalArgs.size() == 3)
                pluginImportVersion = positionalArgs[2];
        } else if (action == Builtins) {
            if (positionalArgs.size() != 1) {
                qWarning() << "Incorrect number of positional arguments";
                return EXIT_INVALIDARGUMENTS;
            }
        }
    }

    QDeclarativeView view;
    QDeclarativeEngine *engine = view.engine();
    if (!pluginImportPath.isEmpty()) {
        QDir cur = QDir::current();
        cur.cd(pluginImportPath);
        pluginImportPath = cur.absolutePath();
        QDir::setCurrent(pluginImportPath);
        engine->addImportPath(pluginImportPath);
    }

    // find all QMetaObjects reachable from the builtin module
    QSet<const QMetaObject *> defaultReachable = collectReachableMetaObjects();
    QList<QDeclarativeType *> defaultTypes = QDeclarativeMetaType::qmlTypes();

    // this will hold the meta objects we want to dump information of
    QSet<const QMetaObject *> metas;

    if (action == Builtins) {
        metas = defaultReachable;
    } else {
        // find a valid QtQuick import
        QByteArray importCode;
        QDeclarativeType *qtObjectType = QDeclarativeMetaType::qmlType(&QObject::staticMetaObject);
        if (!qtObjectType) {
            qWarning() << "Could not find QtObject type";
            importCode = QByteArray("import QtQuick 1.0\n");
        } else {
            QByteArray module = qtObjectType->qmlTypeName();
            module = module.mid(0, module.lastIndexOf('/'));
            importCode = QString("import %1 %2.%3\n").arg(module,
                                                          QString::number(qtObjectType->majorVersion()),
                                                          QString::number(qtObjectType->minorVersion())).toUtf8();
        }

        // find all QMetaObjects reachable when the specified module is imported
        if (action != Path) {
            importCode += QString("import %0 %1\n").arg(pluginImportUri, pluginImportVersion).toLatin1();
        } else {
            // pluginImportVersion can be empty
            importCode += QString("import \".\" %2\n").arg(pluginImportVersion).toLatin1();
        }

        // create a component with these imports to make sure the imports are valid
        // and to populate the declarative meta type system
        {
            QByteArray code = importCode;
            code += "QtObject {}";
            QDeclarativeComponent c(engine);

            c.setData(code, QUrl::fromLocalFile(pluginImportPath + "/typelist.qml"));
            c.create();
            if (!c.errors().isEmpty()) {
                foreach (const QDeclarativeError &error, c.errors())
                    qWarning() << error.toString();
                return EXIT_IMPORTERROR;
            }
        }

        QSet<const QMetaObject *> candidates = collectReachableMetaObjects(defaultTypes);
        candidates.subtract(defaultReachable);

        // Also eliminate meta objects with the same classname.
        // This is required because extended objects seem not to share
        // a single meta object instance.
        QSet<QByteArray> defaultReachableNames;
        foreach (const QMetaObject *mo, defaultReachable)
            defaultReachableNames.insert(QByteArray(mo->className()));
        foreach (const QMetaObject *mo, candidates) {
            if (!defaultReachableNames.contains(mo->className()))
                metas.insert(mo);
        }
    }

    // setup static rewrites of type names
    cppToId.insert("QString", "string");
    cppToId.insert("QDeclarativeEasingValueType::Type", "Type");

    // start dumping data
    QByteArray bytes;
    QmlStreamWriter qml(&bytes);

    qml.writeStartDocument();
    qml.writeLibraryImport(QLatin1String("QtQuick.tooling"), 1, 1);
    qml.write("\n"
              "// This file describes the plugin-supplied types contained in the library.\n"
              "// It is used for QML tooling purposes only.\n"
              "\n");
    qml.writeStartObject("Module");

    // put the metaobjects into a map so they are always dumped in the same order
    QMap<QString, const QMetaObject *> nameToMeta;
    foreach (const QMetaObject *meta, metas)
        nameToMeta.insert(convertToId(meta), meta);

    Dumper dumper(&qml);
    if (relocatable)
        dumper.setRelocatableModuleUri(pluginImportUri);
    foreach (const QMetaObject *meta, nameToMeta) {
        dumper.dump(meta);
    }

    // define QEasingCurve as an extension of QDeclarativeEasingValueType, this way
    // properties using the QEasingCurve type get useful type information.
    if (pluginImportUri.isEmpty())
        dumper.writeEasingCurve();

    qml.writeEndObject();
    qml.writeEndDocument();

    std::cout << bytes.constData() << std::flush;

    // workaround to avoid crashes on exit
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(0);
    QObject::connect(&timer, SIGNAL(timeout()), &app, SLOT(quit()));
    timer.start();

    return app.exec();
}
