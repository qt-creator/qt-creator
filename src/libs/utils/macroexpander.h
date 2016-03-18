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

#include <functional>

#include <QList>
#include <QVector>
#include <QCoreApplication>

namespace Utils {

namespace Internal { class MacroExpanderPrivate; }

class MacroExpander;
typedef std::function<MacroExpander *()> MacroExpanderProvider;
typedef QVector<MacroExpanderProvider> MacroExpanderProviders;

class QTCREATOR_UTILS_EXPORT MacroExpander
{
    Q_DECLARE_TR_FUNCTIONS(Utils::MacroExpander)
    Q_DISABLE_COPY(MacroExpander)

public:
    explicit MacroExpander();
    ~MacroExpander();

    bool resolveMacro(const QString &name, QString *ret) const;

    QString value(const QByteArray &variable, bool *found = 0) const;

    QString expand(const QString &stringWithVariables) const;
    QByteArray expand(const QByteArray &stringWithVariables) const;

    QString expandProcessArgs(const QString &argsWithVariables) const;

    typedef std::function<QString(QString)> PrefixFunction;
    typedef std::function<bool(QString, QString *)> ResolverFunction;
    typedef std::function<QString()> StringFunction;
    typedef std::function<int()> IntFunction;

    void registerPrefix(const QByteArray &prefix,
        const QString &description, const PrefixFunction &value);

    void registerVariable(const QByteArray &variable,
        const QString &description, const StringFunction &value,
        bool visibleInChooser = true);

    void registerIntVariable(const QByteArray &variable,
        const QString &description, const IntFunction &value);

    void registerFileVariables(const QByteArray &prefix,
        const QString &heading, const StringFunction &value,
        bool visibleInChooser = true);

    void registerExtraResolver(const ResolverFunction &value);

    QList<QByteArray> visibleVariables() const;
    QString variableDescription(const QByteArray &variable) const;
    bool isPrefixVariable(const QByteArray &variable) const;

    MacroExpanderProviders subProviders() const;

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void registerSubProvider(const MacroExpanderProvider &provider);

    bool isAccumulating() const;
    void setAccumulating(bool on);

private:
    friend class Internal::MacroExpanderPrivate;
    Internal::MacroExpanderPrivate *d;
};

QTCREATOR_UTILS_EXPORT MacroExpander *globalMacroExpander();

} // namespace Utils
