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

#include <QtCore/QDebug>
#include <QtCore/QtCore>
#include <QtCore/QCoreApplication>

#include <QtDeclarative/QmlDomDocument>
#include <QtDeclarative/QmlEngine>

#define SPACING (QString("    "))

const char* getDefault(const QmlDomProperty& prop)
{
    if (prop.isDefaultProperty())
        return " (default property)";
    else
        return "";
}

void dumpList(const QmlDomList& list, const QString& indent);
void dumpTree(const QmlDomObject& domObject, const QString& indent);

void dumpProperty(const QmlDomProperty& prop, const QString& indent)
{
    const QmlDomValue val(prop.value());

    switch (val.type()) {
        case QmlDomValue::Invalid:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "(invalid)" << getDefault(prop);
            break;

        case QmlDomValue::List:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "(list)" << getDefault(prop);
            dumpList(val.toList(), indent + SPACING);
            break;

        case QmlDomValue::Literal:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "->" << val.toLiteral().literal() << "(literal)" << getDefault(prop);
            break;

        case QmlDomValue::Object:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "->" << val.toObject().objectId() << "(object)" << getDefault(prop);
            dumpTree(val.toObject(), indent + SPACING);
            break;

        case QmlDomValue::PropertyBinding:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "->" << val.toBinding().binding() << "(property binding)" << getDefault(prop);
            break;

        case QmlDomValue::ValueSource:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "->" << val.toValueSource().object().objectId() << "(value source)" << getDefault(prop);
            break;

        default:
            qDebug() << indent.toAscii().data() << prop.propertyName() << "(unknown)" << getDefault(prop);
            break;
    }
}

void dumpList(const QmlDomList& list, const QString& indent)
{
    foreach (const QmlDomValue& val, list.values()) {
        switch (val.type()) {
            case QmlDomValue::Invalid:
                qDebug() << indent.toAscii().data() << "(invalid)";
                break;

            case QmlDomValue::List:
                qDebug() << indent.toAscii().data() << "(list)";
                dumpList(val.toList(), indent + SPACING);
                break;

            case QmlDomValue::Literal:
                qDebug() << indent.toAscii().data() << val.toLiteral().literal() << "(literal)";
                break;

            case QmlDomValue::Object:
                qDebug() << indent.toAscii().data() << val.toObject().objectId() << "(object)";
                dumpTree(val.toObject(), indent + SPACING);
                break;

            case QmlDomValue::PropertyBinding:
                qDebug() << indent.toAscii().data() << val.toBinding().binding() << "(property binding)";
                break;

            case QmlDomValue::ValueSource:
                qDebug() << indent.toAscii().data() << val.toValueSource().object().objectId() << "(value source)";
                break;

            default:
                qDebug() << indent.toAscii().data() << "(unknown)";
                break;
        }
    }
}

void dumpTree(const QmlDomObject& domObject, const QString& indent)
{
    qDebug() << (indent + "Object ID:").toAscii().data() << domObject.objectId() <<"Type: "<<domObject.objectType()<< "Class: " << domObject.objectClassName() << "Url: " << domObject.url().toString();

    if (domObject.isComponent()) {
        QString indent2 = indent + SPACING;
        qDebug() << (indent2 + "isComponent").toAscii().data();
        QmlDomComponent component = domObject.toComponent();
        dumpTree(component.componentRoot(), indent2);
    } else {
        foreach (const QmlDomProperty& prop, domObject.properties()) {
            QString indent2 = indent + SPACING;
            dumpProperty(prop, indent2);
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QByteArray qml;

    QStringList args = a.arguments();
    args.removeFirst();
    if (args.size() != 1) {
        qDebug() << "add file name";
        return -2;
    }

    QFile f(args.at(0));
    if (!f.open(QFile::ReadOnly)) { qDebug() << "cannot open file" << args.at(0); return -3;}
    qml = f.readAll();

    QmlEngine engine;
    QmlDomDocument doc;
    if (!doc.load(&engine, qml, QUrl::fromLocalFile(QFileInfo(f.fileName()).absoluteFilePath()))) {
        foreach (const QmlError &error, doc.errors())
            qDebug() << QString("Error in %1, %2:%3: %4").arg(error.url().toString()).arg(error.line()).arg(error.column()).arg(error.description());
        return -1;
    }

    qDebug() << "---------------------------";

    foreach (const QmlDomImport &import, doc.imports())
        qDebug() << "Import type " << (import.type() == QmlDomImport::Library ? "Library" : "File")
                << "uri" << import.uri() << "qualifier" << import.qualifier() << "version" << import.version();
    dumpTree(doc.rootObject(), "");
    return 0;
}
