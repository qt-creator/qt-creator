// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltextgenerator.h"

#include <QColor>
#include <QVariant>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "bindingproperty.h"
#include "model.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "signalhandlerproperty.h"
#include "stringutils.h"
#include "variantproperty.h"
#include <nodemetainfo.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace Qt::StringLiterals;

static QString properColorName(const QColor &color)
{
    if (color.alpha() == 255)
        return QString::asprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    else
        return QString::asprintf("#%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
}

static bool isLowPrecisionProperties(PropertyNameView property)
{
    static constexpr PropertyNameView properties[] = {
        "width", "height", "x", "y", "rotation", "scale", "opacity"};

    return std::find(std::begin(properties), std::end(properties), property) != std::end(properties);
}

static QString doubleToString(const PropertyName &propertyName, double d)
{
    int precision = 5;
    if (propertyName.contains("anchors") || propertyName.contains("font")
        || isLowPrecisionProperties(propertyName))
        precision = 3;

    QString string = QString::number(d, 'f', precision);
    if (string.contains('.'_L1)) {
        while (string.at(string.length() - 1) == '0'_L1)
            string.chop(1);
        if (string.at(string.length() - 1) == '.'_L1)
            string.chop(1);
    }
    return string;
}

static QString unicodeEscape(const QString &stringValue)
{
    if (stringValue.size() == 1) {
        ushort code = stringValue.at(0).unicode();
        bool isUnicode = code <= 127;
        if (isUnicode) {
            return stringValue;
        } else {
            QString escaped;
            escaped += "\\u";
            escaped += QString::number(code, 16).rightJustified(4, '0');
            return escaped;
        }
    }
    return stringValue;
}

QmlTextGenerator::QmlTextGenerator(const PropertyNameList &propertyOrder,
                                   const TextEditor::TabSettings &tabSettings,
                                   const int startIndentDepth)
    : m_propertyOrder(propertyOrder)
    , m_tabSettings(tabSettings)
    , m_startIndentDepth(startIndentDepth)
{
}

QString QmlTextGenerator::toQml(const AbstractProperty &property, int indentDepth) const
{
    if (property.isBindingProperty()) {
        return property.toBindingProperty().expression();
    } else if (property.isSignalHandlerProperty()) {
        return property.toSignalHandlerProperty().source();
    } else if (property.isSignalDeclarationProperty()) {
        return property.toSignalDeclarationProperty().signature();
    } else if (property.isNodeProperty()) {
        return toQml(property.toNodeProperty().modelNode(), indentDepth);
    } else if (property.isNodeListProperty()) {
        const QList<ModelNode> nodes = property.toNodeListProperty().toModelNodeList();
        if (property.isDefaultProperty()) {
            QString result;
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += "\n\n"_L1;
                result += m_tabSettings.indentationString(0, indentDepth, 0);
                result += toQml(nodes.at(i), indentDepth);
            }
            return result;
        } else {
            QString result = "["_L1;
            const int arrayContentDepth = indentDepth + m_tabSettings.m_indentSize;
            const QString arrayContentIndentation(arrayContentDepth, ' '_L1);
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += ','_L1;
                result += '\n'_L1;
                result += arrayContentIndentation;
                result += toQml(nodes.at(i), arrayContentDepth);
            }
            return result + ']'_L1;
        }
    } else if (property.isVariantProperty()) {
        const VariantProperty variantProperty = property.toVariantProperty();
        const QVariant value = variantProperty.value();
        const QString stringValue = value.toString();

        if (property.name() == "id")
            return stringValue;
        if (variantProperty.holdsEnumeration()) {
            return variantProperty.enumeration().toString();
        } else {
            switch (value.typeId()) {
            case QMetaType::Bool:
                if (value.toBool())
                    return QStringLiteral("true");
                else
                    return QStringLiteral("false");

            case QMetaType::QColor:
                return QStringView(u"\"%1\"").arg(properColorName(value.value<QColor>()));

            case QMetaType::Float:
            case QMetaType::Double:
                return doubleToString(property.name(), value.toDouble());
            case QMetaType::Int:
            case QMetaType::LongLong:
            case QMetaType::UInt:
            case QMetaType::ULongLong:
                return stringValue;
            case QMetaType::QString:
            case QMetaType::QChar:
                return QStringView(u"\"%1\"").arg(escape(unicodeEscape(stringValue)));
            case QMetaType::QVector2D: {
                auto vec = value.value<QVector2D>();
                return QStringLiteral("Qt.vector2d(%1, %2)").arg(vec.x(), vec.y());
            }
            case QMetaType::QVector3D: {
                auto vec = value.value<QVector3D>();
                return QStringLiteral("Qt.vector3d(%1, %2, %3)").arg(vec.x(), vec.y(), vec.z());
            }
            case QMetaType::QVector4D: {
                auto vec = value.value<QVector4D>();
                return QStringLiteral("Qt.vector4d(%1, %2, %3, %4)")
                    .arg(vec.x(), vec.y(), vec.z(), vec.w());
            }
            default:
                return QStringView(u"\"%1\"").arg(escape(stringValue));
            }
        }
    } else {
        Q_ASSERT("Unknown property type");
        return QString();
    }
}

QString QmlTextGenerator::toQml(const ModelNode &node, int indentDepth) const
{
    QString type = QString::fromLatin1(node.type());
    QString url;
    if (type.contains('.')) {
        QStringList nameComponents = type.split('.');
        url = nameComponents.constFirst();
        type = nameComponents.constLast();
    }

    QString alias;
    if (!url.isEmpty()) {
        for (const Import &import : node.model()->imports()) {
            if (import.url() == url) {
                alias = import.alias();
                break;
            }
            if (import.file() == url) {
                alias = import.alias();
                break;
            }
        }
    }

    QString result;

    if (!alias.isEmpty())
        result = alias + '.';

    result += type;
    if (!node.behaviorPropertyName().isEmpty()) {
        result += " on " +  node.behaviorPropertyName();
    }

    result += QStringLiteral(" {\n");

    const int propertyIndentDepth = indentDepth + m_tabSettings.m_indentSize;

    const QString properties = propertiesToQml(node, propertyIndentDepth);

    return result + properties + m_tabSettings.indentationString(0, indentDepth, 0) + '}'_L1;
}

QString QmlTextGenerator::propertiesToQml(const ModelNode &node, int indentDepth) const
{
    QString topPart;
    QString bottomPart;

    PropertyNameList nodePropertyNames = node.propertyNames();
    bool addToTop = true;

    for (const PropertyName &propertyName : std::as_const(m_propertyOrder)) {
        if (propertyName == "id") {
            // the model handles the id property special, so:
            if (!node.id().isEmpty()) {
                QString idLine = m_tabSettings.indentationString(0, indentDepth, 0);
                idLine += QStringLiteral("id: ");
                idLine += node.id();
                idLine += '\n'_L1;

                if (addToTop)
                    topPart.append(idLine);
                else
                    bottomPart.append(idLine);
            }
        } else if (propertyName.isEmpty()) {
            addToTop = false;
        } else if (nodePropertyNames.removeAll(propertyName)) {
            const QString newContent = propertyToQml(node.property(propertyName), indentDepth);

            if (addToTop)
                topPart.append(newContent);
            else
                bottomPart.append(newContent);
        }
    }

    for (const PropertyName &propertyName : std::as_const(nodePropertyNames)) {
        bottomPart.prepend(propertyToQml(node.property(propertyName), indentDepth));
    }

    return topPart + bottomPart;
}

QString QmlTextGenerator::propertyToQml(const AbstractProperty &property, int indentDepth) const
{
    QString result;

    if (property.isDefaultProperty()) {
        result = toQml(property, indentDepth);
    } else {
        if (property.isDynamic()) {
            result = m_tabSettings.indentationString(0, indentDepth, 0) + "property "_L1
                     + QString::fromUtf8(property.dynamicTypeName()) + " "_L1
                     + QString::fromUtf8(property.name()) + ": "_L1 + toQml(property, indentDepth);
        } else if (property.isSignalDeclarationProperty()) {
            result = m_tabSettings.indentationString(0, indentDepth, 0) + "signal" + " "
                     + QString::fromUtf8(property.name()) + " "_L1 + toQml(property, indentDepth);
        } else {
            result = m_tabSettings.indentationString(0, indentDepth, 0)
                     + QString::fromUtf8(property.name()) + ": "_L1 + toQml(property, indentDepth);
        }
    }

    result += '\n'_L1;

    return result;
}
