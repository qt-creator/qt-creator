#include <QApplication>
#include <QSet>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include <QMetaObject>
#include <QMetaProperty>
#include <QPushButton>
#include <QDebug>
#include <iostream>
#include <QtDeclarative>
#include <QtCore/private/qobject_p.h>
#include <QtCore/private/qmetaobject_p.h>
#include <QtDeclarative/private/qdeclarativemetatype_p.h>
#include <QtDeclarative/private/qdeclarativeopenmetaobject_p.h>
#include <QtDeclarative/QDeclarativeView>
#ifdef QT_SIMULATOR
#include <QtGui/private/qsimulatorconnection_p.h>
#endif
#ifdef Q_OS_UNIX
#include <signal.h>
#endif

static QHash<QByteArray, QList<const QDeclarativeType *> > qmlTypesByCppName;
static QHash<QByteArray, QByteArray> cppToId;
QString currentProperty;

QByteArray convertToId(const QByteArray &cppName)
{
    QByteArray idName = cppToId.value(cppName, cppName);
    idName.replace("::", ".");
    idName.replace("/", ".");
    return idName;
}

void erasure(QByteArray *typeName, bool *isList, bool *isPointer)
{
    static QByteArray declListPrefix = "QDeclarativeListProperty<";

    if (typeName->endsWith('*')) {
        *isPointer = true;
        typeName->truncate(typeName->length() - 1);
        erasure(typeName, isList, isPointer);
    } else if (typeName->startsWith(declListPrefix)) {
        *isList = true;
        typeName->truncate(typeName->length() - 1); // get rid of the suffix '>'
        *typeName = typeName->mid(declListPrefix.size());
        erasure(typeName, isList, isPointer);
    }

    *typeName = convertToId(*typeName);
}

void processMetaObject(const QMetaObject *meta, QSet<const QMetaObject *> *metas)
{
    if (! meta || metas->contains(meta))
        return;

    // dynamic meta objects break things badly
    const QMetaObjectPrivate *mop = reinterpret_cast<const QMetaObjectPrivate *>(meta->d.data);
    if (!(mop->flags & DynamicMetaObject))
        metas->insert(meta);

    processMetaObject(meta->superClass(), metas);
}

void processObject(QObject *object, QSet<const QMetaObject *> *metas)
{
    if (! object)
        return;

    const QMetaObject *meta = object->metaObject();
    qDebug() << "Processing object" << meta->className();
    processMetaObject(meta, metas);

    for (int index = 0; index < meta->propertyCount(); ++index) {
        QMetaProperty prop = meta->property(index);
        if (QDeclarativeMetaType::isQObject(prop.userType())) {
            qDebug() << "  Processing property" << prop.name();
            currentProperty = QString("%1::%2").arg(meta->className(), prop.name());
            QObject *oo = QDeclarativeMetaType::toQObject(prop.read(object));
            if (oo && !metas->contains(oo->metaObject()))
                processObject(oo, metas);
            currentProperty.clear();
        }
    }
}

void processDeclarativeType(const QDeclarativeType *ty, QSet<const QMetaObject *> *metas)
{
    processMetaObject(ty->metaObject(), metas);
}

void writeType(QXmlStreamAttributes *attrs, QByteArray typeName, bool isWritable = false)
{
    bool isList = false, isPointer = false;
    erasure(&typeName, &isList, &isPointer);
    attrs->append(QXmlStreamAttribute("type", typeName));
    if (isList)
        attrs->append(QXmlStreamAttribute("isList", "true"));
    if (isWritable)
        attrs->append(QXmlStreamAttribute("isWritable", "true"));
    if (isPointer)
        attrs->append(QXmlStreamAttribute("isPointer", "true"));
}

void dump(const QMetaProperty &prop, QXmlStreamWriter *xml)
{
    xml->writeStartElement("property");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(prop.name())));

    writeType(&attributes, prop.typeName(), prop.isWritable());

    xml->writeAttributes(attributes);
    xml->writeEndElement();
}

void dump(const QMetaMethod &meth, QXmlStreamWriter *xml)
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


    QString elementName = QLatin1String("method");

    QXmlStreamAttributes attributes;
    if (meth.methodType() == QMetaMethod::Signal)
        elementName = QLatin1String("signal");

    xml->writeStartElement(elementName);

    attributes.append(QXmlStreamAttribute("name", name));

    const QString typeName = convertToId(meth.typeName());
    if (! typeName.isEmpty())
        attributes.append(QXmlStreamAttribute("type", typeName));

    xml->writeAttributes(attributes);

    for (int i = 0; i < meth.parameterTypes().size(); ++i) {
        QByteArray argName = meth.parameterNames().at(i);

        xml->writeStartElement("param");

        QXmlStreamAttributes attrs;

        if (! argName.isEmpty())
            attrs.append(QXmlStreamAttribute("name", argName));

        writeType(&attrs, meth.parameterTypes().at(i));
        xml->writeAttributes(attrs);

        xml->writeEndElement();
    }

    xml->writeEndElement();
}

void dump(const QMetaEnum &e, QXmlStreamWriter *xml)
{
    xml->writeStartElement("enum");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(e.name()))); // ### FIXME
    xml->writeAttributes(attributes);

    for (int index = 0; index < e.keyCount(); ++index) {
        xml->writeStartElement("enumerator");

        QXmlStreamAttributes attributes;
        attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(e.key(index))));
        attributes.append(QXmlStreamAttribute("value", QString::number(e.value(index))));
        xml->writeAttributes(attributes);

        xml->writeEndElement();
    }

    xml->writeEndElement();
}

class FriendlyQObject: public QObject
{
public:
    static const QMetaObject *qtMeta() { return &staticQtMetaObject; }
};

void dump(const QMetaObject *meta, QXmlStreamWriter *xml)
{
    QByteArray id = convertToId(meta->className());

    xml->writeStartElement("type");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", id));

    for (int index = meta->classInfoCount() - 1 ; index >= 0 ; --index) {
        QMetaClassInfo classInfo = meta->classInfo(index);
        if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
            attributes.append(QXmlStreamAttribute("defaultProperty", QLatin1String(classInfo.value())));
            break;
        }
    }

    if (meta->superClass())
        attributes.append(QXmlStreamAttribute("extends", convertToId(meta->superClass()->className())));

    xml->writeAttributes(attributes);

    QList<const QDeclarativeType *> qmlTypes = qmlTypesByCppName.value(meta->className());
    if (!qmlTypes.isEmpty()) {
        xml->writeStartElement("exports");
        foreach (const QDeclarativeType *qmlTy, qmlTypes) {
            QXmlStreamAttributes moduleAttributes;
            const QString qmlTyName = qmlTy->qmlTypeName();
            int slashIdx = qmlTyName.lastIndexOf(QLatin1Char('/'));
            if (slashIdx == -1)
                continue;
            const QString moduleName = qmlTyName.left(slashIdx);
            const QString typeName = qmlTyName.mid(slashIdx + 1);
            moduleAttributes.append(QXmlStreamAttribute("module", moduleName));
            moduleAttributes.append(QXmlStreamAttribute("version", QString("%1.%2").arg(qmlTy->majorVersion()).arg(qmlTy->minorVersion())));
            moduleAttributes.append(QXmlStreamAttribute("type", typeName));
            xml->writeEmptyElement("export");
            xml->writeAttributes(moduleAttributes);
        }
        xml->writeEndElement();
    }

    for (int index = meta->enumeratorOffset(); index < meta->enumeratorCount(); ++index)
        dump(meta->enumerator(index), xml);

    for (int index = meta->propertyOffset(); index < meta->propertyCount(); ++index)
        dump(meta->property(index), xml);

    for (int index = meta->methodOffset(); index < meta->methodCount(); ++index)
        dump(meta->method(index), xml);

    xml->writeEndElement();
}

void writeEasingCurve(QXmlStreamWriter *xml)
{
    xml->writeStartElement("type");
    {
        QXmlStreamAttributes attributes;
        attributes.append(QXmlStreamAttribute("name", "QEasingCurve"));
        attributes.append(QXmlStreamAttribute("extends", "QDeclarativeEasingValueType"));
        xml->writeAttributes(attributes);
    }

    xml->writeEndElement();
}

#ifdef Q_OS_UNIX
void sigSegvHandler(int) {
    fprintf(stderr, "Error: qmldump SEGV\n");
    if (!currentProperty.isEmpty())
        fprintf(stderr, "While processing the property '%s', which probably has uninitialized data.\n", currentProperty.toLatin1().constData());
    exit(EXIT_FAILURE);
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_UNIX
    // qmldump may crash, but we don't want any crash handlers to pop up
    // therefore we intercept the segfault and just exit() ourselves
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    action.sa_handler = &sigSegvHandler;
    action.sa_flags   = 0;

    sigaction(SIGSEGV, &action, 0);
#endif

#ifdef QT_SIMULATOR
    QtSimulatorPrivate::SimulatorConnection::createStubInstance();
#endif
    QApplication app(argc, argv);

    if (argc != 1 && argc != 3) {
        qWarning() << "Usage: qmldump [plugin/import/path plugin.uri]";
        qWarning() << "Example: ./qmldump /home/user/dev/qt-install/imports Qt.labs.particles";
        return 1;
    }

    QString pluginImportName;
    QString pluginImportPath;
    if (argc == 3) {
        pluginImportPath = QString(argv[1]);
        pluginImportName = QString(argv[2]);
    }

    QDeclarativeView view;
    QDeclarativeEngine *engine = view.engine();
    if (!pluginImportPath.isEmpty())
        engine->addImportPath(pluginImportPath);

    bool hasQtQuickModule = false;
    {
        QByteArray code = "import QtQuick 1.0; Item {}";
        QDeclarativeComponent c(engine);
        c.setData(code, QUrl("xxx"));
        c.create();
        if (c.errors().isEmpty()) {
            hasQtQuickModule = true;
        }
    }

    QByteArray importCode;
    importCode += "import Qt 4.7;\n";
    if (hasQtQuickModule) {
        importCode += "import QtQuick 1.0;\n";
    }
    if (pluginImportName.isEmpty()) {
        importCode += "import Qt.labs.particles 4.7;\n";
        importCode += "import Qt.labs.gestures 4.7;\n";
        importCode += "import Qt.labs.folderlistmodel 4.7;\n";
        importCode += "import QtWebKit 1.0;\n";
    } else {
        importCode += QString("import %0 1.0;\n").arg(pluginImportName).toAscii();
    }

    {
        QByteArray code = importCode;
        code += "Item {}";
        QDeclarativeComponent c(engine);

        c.setData(code, QUrl("xxx"));
        c.create();
        if (!c.errors().isEmpty())
            qDebug() << c.errorString();
    }

    cppToId.insert("QString", "string");
    cppToId.insert("QDeclarativeEasingValueType::Type", "Type");

    QSet<const QMetaObject *> metas;

    metas.insert(FriendlyQObject::qtMeta());

    QHash<QByteArray, QSet<QByteArray> > extensions;
    foreach (const QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        qmlTypesByCppName[ty->metaObject()->className()].append(ty);
        if (ty->isExtendedType()) {
            extensions[ty->typeName()].insert(ty->metaObject()->className());
        } else {
            cppToId.insert(ty->metaObject()->className(), ty->metaObject()->className());
        }
        processDeclarativeType(ty, &metas);
    }

    // Adjust ids of extended objects.
    // The chain ends up being:
    // __extended__.originalname - the base object
    // __extension_0_.originalname - first extension
    // ..
    // __extension_n-2_.originalname - second to last extension
    // originalname - last extension
    foreach (const QByteArray &extendedCpp, extensions.keys()) {
        const QByteArray extendedId = cppToId.value(extendedCpp);
        cppToId.insert(extendedCpp, "__extended__." + extendedId);
        QSet<QByteArray> extensionCppNames = extensions.value(extendedCpp);
        int c = 0;
        foreach (const QByteArray &extensionCppName, extensionCppNames) {
            if (c != extensionCppNames.size() - 1) {
                QByteArray adjustedName = QString("__extension__%1.%2").arg(QString::number(c), QString(extendedId)).toAscii();
                cppToId.insert(extensionCppName, adjustedName);
            } else {
                cppToId.insert(extensionCppName, extendedId);
            }
            ++c;
        }
    }

    foreach (const QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        if (ty->isExtendedType())
            continue;

        QByteArray tyName = ty->qmlTypeName();
        tyName = tyName.mid(tyName.lastIndexOf('/') + 1);

        QByteArray code = importCode;
        code += tyName;
        code += " {}\n";

        QDeclarativeComponent c(engine);
        c.setData(code, QUrl("xxx"));

        QObject *object = c.create();
        if (object)
            processObject(object, &metas);
        else
            qDebug() << "Could not create" << tyName << ":" << c.errorString();
    }

    QByteArray bytes;
    QXmlStreamWriter xml(&bytes);
    xml.setAutoFormatting(true);

    xml.writeStartDocument("1.0");
    xml.writeStartElement("module");

    QMap<QString, const QMetaObject *> nameToMeta;
    foreach (const QMetaObject *meta, metas) {
        nameToMeta.insert(convertToId(meta->className()), meta);
    }
    foreach (const QMetaObject *meta, nameToMeta) {
        dump(meta, &xml);
    }

    // define QEasingCurve as an extension of QDeclarativeEasingValueType
    writeEasingCurve(&xml);

    xml.writeEndElement();
    xml.writeEndDocument();

    std::cout << bytes.constData();

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(0);
    QObject::connect(&timer, SIGNAL(timeout()), &app, SLOT(quit()));

    timer.start();

    return app.exec();
}
