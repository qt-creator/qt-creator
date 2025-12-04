// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "signalhandlerproperty.h"
#include "externaldependenciesinterface.h"
#include "internalnode_p.h"
#include "internalproperty.h"
#include "model.h"
#include "model_p.h"

#include <QRegularExpression>

namespace QmlDesigner {

SignalHandlerProperty::SignalHandlerProperty() = default;

SignalHandlerProperty::SignalHandlerProperty(const SignalHandlerProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNodeSharedPointer(), property.model(), view)
{
}

void SignalHandlerProperty::setSource(const QString &source, SL sl)
{
    NanotraceHR::Tracer tracer{"signal handler property set source",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    Internal::WriteLocker locker(model());
    if (!isValid())
        return;

    if (name() == "id")
        return;

    if (source.isEmpty())
        return;

    if (auto internalProperty = internalNode()->property(name())) {
        auto signalHandlerProperty = internalProperty->to<PropertyType::SignalHandler>();
        //check if oldValue != value
        if (signalHandlerProperty && signalHandlerProperty->source() == source)
            return;

        if (!signalHandlerProperty)
            privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    privateModel()->setSignalHandlerProperty(internalNodeSharedPointer(), name(), source);
}

QString SignalHandlerProperty::source(SL sl) const
{
    NanotraceHR::Tracer tracer{"signal handler property source",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    if (auto property = internalNode()->signalHandlerProperty(name()))
        return property->source();

    return QString();
}

QString SignalHandlerProperty::sourceNormalizedWithBraces(SL sl) const
{
    NanotraceHR::Tracer tracer{"signal handler property source normalized with braces",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    return normalizedSourceWithBraces(source());
}

bool SignalHandlerProperty::useNewFunctionSyntax(SL sl)
{
    NanotraceHR::Tracer tracer{"signal handler property use new function syntax",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (name().contains('.'))
        return false;

    if (!view())
        return false;

    if (view()->externalDependencies().isQtForMcusProject())
        return false;

    return parentModelNode().metaInfo().isQtQmlConnections();
}

PropertyName SignalHandlerProperty::prefixAdded(PropertyNameView propertyName, SL sl)
{
    NanotraceHR::Tracer tracer{"signal handler property prefix added",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    QString nameAsString = QString::fromUtf8(propertyName);
    if (propertyName.startsWith("on"))
        return propertyName.toByteArray();

    QChar firstChar = nameAsString.at(0).toUpper();
    nameAsString[0] = firstChar;
    nameAsString.prepend("on");

    return nameAsString.toLatin1();
}

PropertyName SignalHandlerProperty::prefixRemoved(PropertyNameView propertyName, SL sl)
{
    NanotraceHR::Tracer tracer{"signal handler property prefix removed",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    QString nameAsString = QString::fromUtf8(propertyName);
    if (!nameAsString.startsWith("on"))
        return propertyName.toByteArray();

    nameAsString.remove(0, 2);
    QChar firstChar = nameAsString.at(0).toLower();
    nameAsString[0] = firstChar;

    return nameAsString.toLatin1();
}

QString SignalHandlerProperty::normalizedSourceWithBraces(const QString &source, SL sl)
{
    NanotraceHR::Tracer tracer{"signal handler property normalized source with braces",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    static const QRegularExpression reg("^\\{(\\s*?.*?)*?\\}$");

    const QString trimmed = source.trimmed();
    auto match = reg.match(trimmed);

    if (match.hasMatch())
        return trimmed;

    return QString("{%2%1%2}").arg(trimmed).arg(trimmed.contains('\n') ? "\n" : " ");
}

SignalDeclarationProperty::SignalDeclarationProperty() = default;

SignalDeclarationProperty::SignalDeclarationProperty(const SignalDeclarationProperty &property,
                                                     AbstractView *view)
    : AbstractProperty(property.name(), property.internalNodeSharedPointer(), property.model(), view)
{}

void SignalDeclarationProperty::setSignature(const QString &signature, SL sl)
{
    NanotraceHR::Tracer tracer{"signal declaration property set signature",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    Internal::WriteLocker locker(model());
    if (!isValid())
        return;

    if (name() == "id")
        return;

    if (signature.isEmpty())
        return;

    if (auto internalProperty = internalNode()->property(name())) {
        auto signalDeclarationProperty = internalProperty->to<PropertyType::SignalDeclaration>();
        //check if oldValue != value
        if (signalDeclarationProperty && signalDeclarationProperty->signature() == signature)
            return;

        if (!signalDeclarationProperty)
            privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    privateModel()->setSignalDeclarationProperty(internalNodeSharedPointer(), name(), signature);
}

QString SignalDeclarationProperty::signature(SL sl) const
{
    NanotraceHR::Tracer tracer{"signal declaration property signature",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    if (auto property = internalNode()->signalDeclarationProperty(name()))
        return property->signature();

    return QString();
}

} // namespace QmlDesigner
