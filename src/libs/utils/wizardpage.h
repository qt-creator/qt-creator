// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QSet>
#include <QString>
#include <QVariant>
#include <QWizardPage>

#include <functional>

namespace Utils {

class Wizard;
namespace Internal {

class QTCREATOR_UTILS_EXPORT ObjectToFieldWidgetConverter : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)

public:
    template<class T, typename... Arguments>
    static ObjectToFieldWidgetConverter *create(T *sender,
                                                void (T::*member)(Arguments...),
                                                const std::function<QVariant()> &toVariantFunction)
    {
        auto widget = new ObjectToFieldWidgetConverter();
        widget->toVariantFunction = toVariantFunction;
        connect(sender, &QObject::destroyed, widget, &QObject::deleteLater);
        connect(sender, member, widget, [widget] { emit widget->valueChanged(widget->value()); });
        return widget;
    }

signals:
    void valueChanged(const QVariant &);

private:
    ObjectToFieldWidgetConverter () = default;

    // is used by the property value
    QVariant value() { return toVariantFunction(); }
    std::function<QVariant()> toVariantFunction;
};

} // Internal

class QTCREATOR_UTILS_EXPORT WizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit WizardPage(QWidget *parent = nullptr);

    virtual void pageWasAdded(); // called when this page was added to a Utils::Wizard

    template<class T, typename... Arguments>
    void registerObjectAsFieldWithName(const QString &name,
                                       T *sender,
                                       void (T::*changeSignal)(Arguments...),
                                       const std::function<QVariant()> &senderToVariant)
    {
        registerFieldWithName(name,
                              Internal::ObjectToFieldWidgetConverter::create(sender,
                                                                             changeSignal,
                                                                             senderToVariant),
                              "value",
                              SIGNAL(valueChanged(QValue)));
    }

    void registerFieldWithName(const QString &name, QWidget *widget,
                               const char *property = nullptr, const char *changedSignal = nullptr);

    virtual bool handleReject();
    virtual bool handleAccept();

signals:
    // Emitted when there is something that the developer using this page should be aware of.
    void reportError(const QString &errorMessage);

private:
    void registerFieldName(const QString &name);

    QSet<QString> m_toRegister;
};

} // namespace Utils
