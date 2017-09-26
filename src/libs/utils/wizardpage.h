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

#pragma once

#include "utils_global.h"

#include <QSet>
#include <QString>
#include <QWizardPage>

#include <functional>

namespace Utils {

class Wizard;
namespace Internal {

class QTCREATOR_UTILS_EXPORT ObjectToFieldWidgetConverter : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

public:
    template <class T, typename... Arguments>
    static ObjectToFieldWidgetConverter *create(T *sender, void (T::*member)(Arguments...), const std::function<QString()> &toTextFunction)
    {
        auto widget = new ObjectToFieldWidgetConverter();
        widget->toTextFunction = toTextFunction;
        connect(sender, &QObject::destroyed, widget, &QObject::deleteLater);
        connect(sender, member, widget, [widget] () {
            emit widget->textChanged(widget->text());
        });
        return widget;
    }

signals:
    void textChanged(const QString&);

private:
    ObjectToFieldWidgetConverter () = default;

    // is used by the property text
    QString text() {return toTextFunction();}
    std::function<QString()> toTextFunction;
};

} // Internal

class QTCREATOR_UTILS_EXPORT WizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit WizardPage(QWidget *parent = 0);

    virtual void pageWasAdded(); // called when this page was added to a Utils::Wizard

    template<class T, typename... Arguments>
    void registerObjectAsFieldWithName(const QString &name, T *sender, void (T::*changeSignal)(Arguments...),
        const std::function<QString()> &senderToString)
    {
        registerFieldWithName(name, Internal::ObjectToFieldWidgetConverter::create(sender, changeSignal, senderToString), "text", SIGNAL(textChanged(QString)));
    }

    void registerFieldWithName(const QString &name, QWidget *widget,
                               const char *property = 0, const char *changedSignal = 0);

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
