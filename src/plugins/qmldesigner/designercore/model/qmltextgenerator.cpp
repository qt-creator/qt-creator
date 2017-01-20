/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmltextgenerator.h"

#include <QVariant>
#include <QColor>

#include "bindingproperty.h"
#include "signalhandlerproperty.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "variantproperty.h"
#include <nodemetainfo.h>
#include "model.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

inline static QString properColorName(const QColor &color)
{
    QString s;
    if (color.alpha() == 255)
        s.sprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    else
        s.sprintf("#%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
    return s;
}

inline static QString doubleToString(double d)
{
    QString string = QString::number(d, 'f', 3);
    if (string.contains(QLatin1Char('.'))) {
        while (string.at(string.length()- 1) == QLatin1Char('0'))
            string.chop(1);
        if (string.at(string.length()- 1) == QLatin1Char('.'))
            string.chop(1);
    }
    return string;
}

static QString unicodeEscape(const QString &stringValue)
{
    if (stringValue.count() == 1) {
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

QmlTextGenerator::QmlTextGenerator(const PropertyNameList &propertyOrder, int indentDepth):
        m_propertyOrder(propertyOrder),
        m_indentDepth(indentDepth)
{
}

QString QmlTextGenerator::toQml(const AbstractProperty &property, int indentDepth) const
{
    if (property.isBindingProperty()) {
        return property.toBindingProperty().expression();
    } else if (property.isSignalHandlerProperty()) {
        return property.toSignalHandlerProperty().source();
    } else if (property.isNodeProperty()) {
        return toQml(property.toNodeProperty().modelNode(), indentDepth);
    } else if (property.isNodeListProperty()) {
        const QList<ModelNode> nodes = property.toNodeListProperty().toModelNodeList();
        if (property.isDefaultProperty()) {
            QString result;
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += QStringLiteral("\n\n");
                result += QString(indentDepth, QLatin1Char(' '));
                result += toQml(nodes.at(i), indentDepth);
            }
            return result;
        } else {
            QString result = QStringLiteral("[");
            const int arrayContentDepth = indentDepth + 4;
            const QString arrayContentIndentation(arrayContentDepth, QLatin1Char(' '));
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += QLatin1Char(',');
                result += QLatin1Char('\n');
                result += arrayContentIndentation;
                result += toQml(nodes.at(i), arrayContentDepth);
            }
            return result + QLatin1Char(']');
        }
    } else if (property.isVariantProperty()) {
        const VariantProperty variantProperty = property.toVariantProperty();
        const QVariant value = variantProperty.value();
        const QString stringValue = value.toString();

        if (property.name() == "id")
            return stringValue;

          if (false) {
          }
        if (variantProperty.holdsEnumeration()) {
            return variantProperty.enumeration().toString();
        } else {

            switch (value.userType()) {
            case QMetaType::Bool:
                if (value.value<bool>())
                    return QStringLiteral("true");
                else
                    return QStringLiteral("false");

            case QMetaType::QColor:
                return QStringLiteral("\"%1\"").arg(properColorName(value.value<QColor>()));

            case QMetaType::Float:
            case QMetaType::Double:
                return doubleToString(value.toDouble());
            case QMetaType::Int:
            case QMetaType::LongLong:
            case QMetaType::UInt:
            case QMetaType::ULongLong:
                return stringValue;
            case QMetaType::QString:
            case QMetaType::QChar:
                return QStringLiteral("\"%1\"").arg(escape(unicodeEscape(stringValue)));
            default:
                return QStringLiteral("\"%1\"").arg(escape(stringValue));
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
        url = nameComponents.first();
        type = nameComponents.last();
    }

    QString alias;
    if (!url.isEmpty()) {
        foreach (const Import &import, node.model()->imports()) {
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
    result += QStringLiteral(" {\n");

    const int propertyIndentDepth = indentDepth + 4;

    const QString properties = propertiesToQml(node, propertyIndentDepth);

    return result + properties + QString(indentDepth, QLatin1Char(' ')) + QLatin1Char('}');
}

QString QmlTextGenerator::propertiesToQml(const ModelNode &node, int indentDepth) const
{
    QString topPart;
    QString bottomPart;

    PropertyNameList nodePropertyNames = node.propertyNames();
    bool addToTop = true;

    foreach (const PropertyName &propertyName, m_propertyOrder) {
        if (propertyName == "id") {
            // the model handles the id property special, so:
            if (!node.id().isEmpty()) {
                QString idLine(indentDepth, QLatin1Char(' '));
                idLine += QStringLiteral("id: ");
                idLine += node.id();
                idLine += QLatin1Char('\n');

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

    foreach (const PropertyName &propertyName, nodePropertyNames) {
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
            result = QString(indentDepth, QLatin1Char(' '))
                    + QStringLiteral("property ")
                    + QString::fromUtf8(property.dynamicTypeName())
                    + QStringLiteral(" ")
                    + QString::fromUtf8(property.name())
                    + QStringLiteral(": ")
                    + toQml(property, indentDepth);
        } else {
            result = QString(indentDepth, QLatin1Char(' '))
                    + QString::fromUtf8(property.name())
                    + QStringLiteral(": ")
                    + toQml(property, indentDepth);
        }
    }

    result += QLatin1Char('\n');

    return result;
}

QString QmlTextGenerator::escape(const QString &value)
{
    QString result = value;

    if (value.count() == 6 && value.startsWith("\\u")) //Do not dobule escape unicode chars
        return result;

    result.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));

    result.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    result.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    result.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    result.replace(QStringLiteral("\n"), QStringLiteral("\\n"));

    return result;
}
