// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QCoreApplication>
#include <QList>

#include <functional>

namespace Utils {

namespace Internal { class MacroExpanderPrivate; }

class FilePath;
class MacroExpander;
using MacroExpanderProvider = std::function<MacroExpander *()>;
using MacroExpanderProviders = QList<MacroExpanderProvider>;

class QTCREATOR_UTILS_EXPORT MacroExpander
{
    Q_DISABLE_COPY(MacroExpander)

public:
    explicit MacroExpander();
    ~MacroExpander();

    bool resolveMacro(const QString &name, QString *ret) const;

    QString value(const QByteArray &variable, bool *found = nullptr) const;

    QString expand(const QString &stringWithVariables) const;
    FilePath expand(const FilePath &fileNameWithVariables) const;
    QByteArray expand(const QByteArray &stringWithVariables) const;
    QVariant expandVariant(const QVariant &v) const;

    QString expandProcessArgs(const QString &argsWithVariables) const;

    using PrefixFunction = std::function<QString(QString)>;
    using ResolverFunction = std::function<bool(QString, QString *)>;
    using StringFunction = std::function<QString()>;
    using FileFunction = std::function<FilePath()>;
    using IntFunction = std::function<int()>;

    void registerPrefix(const QByteArray &prefix,
        const QString &description, const PrefixFunction &value, bool visible = true);

    void registerVariable(const QByteArray &variable,
        const QString &description, const StringFunction &value,
        bool visibleInChooser = true);

    void registerIntVariable(const QByteArray &variable,
        const QString &description, const IntFunction &value);

    void registerFileVariables(const QByteArray &prefix,
        const QString &heading, const FileFunction &value,
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
