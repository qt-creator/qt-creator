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
#include <QtDeclarative/private/qdeclarativemetatype_p.h>
#include <QtDeclarative/QDeclarativeView>

static QHash<QByteArray, const QDeclarativeType *> qmlTypeByCppName;
static QHash<QByteArray, QByteArray> cppToQml;
static QByteArray pluginPackage;

QByteArray convertToQmlType(const QByteArray &cppName)
{
    QByteArray qmlName = cppToQml.value(cppName, cppName);
    qmlName.replace("::", ".");
    qmlName.replace("/", ".");
    return qmlName;
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

    *typeName = convertToQmlType(*typeName);
}

void processMetaObject(const QMetaObject *meta, QSet<const QMetaObject *> *metas)
{
    if (! meta || metas->contains(meta))
        return;

    metas->insert(meta);
    processMetaObject(meta->superClass(), metas);
}

void processObject(QObject *object, QSet<const QMetaObject *> *metas)
{
    if (! object)
        return;

    const QMetaObject *meta = object->metaObject();
    processMetaObject(meta, metas);

    for (int index = 0; index < meta->propertyCount(); ++index) {
        QMetaProperty prop = meta->property(index);
        if (QDeclarativeMetaType::isQObject(prop.userType())) {
            QObject *oo = QDeclarativeMetaType::toQObject(prop.read(object));
            if (oo && !metas->contains(oo->metaObject()))
                processObject(oo, metas);
        }
    }
}

void processDeclarativeType(const QDeclarativeType *ty, QSet<const QMetaObject *> *metas)
{
    processMetaObject(ty->metaObject(), metas);
}

void writeType(QXmlStreamAttributes *attrs, QByteArray typeName)
{
    bool isList = false, isPointer = false;
    erasure(&typeName, &isList, &isPointer);
    attrs->append(QXmlStreamAttribute("type", typeName));
    if (isList)
        attrs->append(QXmlStreamAttribute("isList", "true"));
}

void dump(const QMetaProperty &prop, QXmlStreamWriter *xml)
{
    xml->writeStartElement("property");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", QString::fromUtf8(prop.name())));

    writeType(&attributes, prop.typeName());

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

    const QString typeName = convertToQmlType(meth.typeName());
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
    QByteArray qmlTypeName = convertToQmlType(meta->className());
    if (!pluginPackage.isEmpty() && !qmlTypeName.startsWith(pluginPackage))
        return;

    xml->writeStartElement("type");

    QXmlStreamAttributes attributes;
    attributes.append(QXmlStreamAttribute("name", qmlTypeName));

    if (const QDeclarativeType *qmlTy = qmlTypeByCppName.value(meta->className())) {
        attributes.append(QXmlStreamAttribute("version", QString("%1.%2").arg(qmlTy->majorVersion()).arg(qmlTy->minorVersion())));
    }

    for (int index = meta->classInfoCount() - 1 ; index >= 0 ; --index) {
        QMetaClassInfo classInfo = meta->classInfo(index);
        if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
            attributes.append(QXmlStreamAttribute("defaultProperty", QLatin1String(classInfo.value())));
            break;
        }
    }

    QString version;

    if (meta->superClass())
        attributes.append(QXmlStreamAttribute("extends", convertToQmlType(meta->superClass()->className())));

    if (! version.isEmpty())
        attributes.append(QXmlStreamAttribute("version", version));

    xml->writeAttributes(attributes);

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
        attributes.append(QXmlStreamAttribute("extends", "Qt.Easing"));
        xml->writeAttributes(attributes);
    }

    xml->writeEndElement();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (argc != 1 && argc != 2) {
        qWarning() << "Usage: qmldump [path/to/plugin/directory]";
        return 1;
    }

    QString pluginImportName;
    QString pluginImportPath;
    if (argc == 2) {
        QFileInfo pluginPath(argv[1]);
        if (pluginPath.exists() && pluginPath.isDir()) {
            pluginImportPath = pluginPath.absolutePath();
            pluginImportName = pluginPath.fileName();
            pluginPackage = (pluginImportName + ".").toLatin1();
        }
    }

    QDeclarativeView view;
    QDeclarativeEngine *engine = view.engine();
    if (!pluginImportPath.isEmpty())
        engine->addImportPath(pluginImportPath);

    QByteArray importCode;
    importCode += "import Qt 4.7;\n";
    importCode += "import Qt.labs.particles 4.7;\n";
    importCode += "import Qt.labs.gestures 4.7;\n";
    importCode += "import Qt.labs.folderlistmodel 4.7;\n";
    importCode += "import org.webkit 1.0;\n";
    if (!pluginImportName.isEmpty())
        importCode += QString("import %0 1.0;\n").arg(pluginImportName);

    {
        QByteArray code = importCode;
        code += "Item {}";
        QDeclarativeComponent c(engine);
        c.setData(code, QUrl("xxx"));
        c.create();
        if (!c.errors().isEmpty())
            qDebug() << c.errorString();
    }

    cppToQml.insert("QString", "string");
    cppToQml.insert("QDeclarativeEasingValueType::Type", "Type");

    QSet<const QMetaObject *> metas;

    metas.insert(FriendlyQObject::qtMeta());

    QMultiHash<QByteArray, QByteArray> extensions;
    foreach (const QDeclarativeType *ty, QDeclarativeMetaType::qmlTypes()) {
        qmlTypeByCppName.insert(ty->metaObject()->className(), ty);
        if (ty->isExtendedType()) {
            extensions.insert(ty->typeName(), ty->metaObject()->className());
        } else {
            cppToQml.insert(ty->metaObject()->className(), ty->qmlTypeName());
        }
        processDeclarativeType(ty, &metas);
    }

    // Adjust qml names of extended objects.
    // The chain ends up being:
    // __extended__.originalname - the base object
    // __extension_0_.originalname - first extension
    // ..
    // __extension_n-2_.originalname - second to last extension
    // originalname - last extension
    foreach (const QByteArray &extendedCpp, extensions.keys()) {
        const QByteArray extendedQml = cppToQml.value(extendedCpp);
        cppToQml.insert(extendedCpp, "__extended__." + extendedQml);
        QList<QByteArray> extensionCppNames = extensions.values(extendedCpp);
        for (int i = 0; i < extensionCppNames.size() - 1; ++i) {
            QByteArray adjustedName = QString("__extension__%1.%2").arg(QString::number(i), QString(extendedQml)).toAscii();
            cppToQml.insert(extensionCppNames.value(i), adjustedName);
        }
        cppToQml.insert(extensionCppNames.last(), extendedQml);
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
        processObject(c.create(), &metas);
    }

    QByteArray bytes;
    QXmlStreamWriter xml(&bytes);
    xml.setAutoFormatting(true);

    xml.writeStartDocument("1.0");
    xml.writeStartElement("module");

    QMap<QString, const QMetaObject *> nameToMeta;
    foreach (const QMetaObject *meta, metas) {
        nameToMeta.insert(convertToQmlType(meta->className()), meta);
    }
    foreach (const QMetaObject *meta, nameToMeta) {
        dump(meta, &xml);
    }

    // define QEasingCurve as an extension of Qt.Easing
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
