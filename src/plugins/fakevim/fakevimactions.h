// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifndef FAKEVIM_STANDALONE

#include <coreplugin/dialogs/ioptionspage.h>

#else

namespace Utils { class FilePath {}; }

#endif

#include <QCoreApplication>
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>

namespace FakeVim::Internal {

#ifdef FAKEVIM_STANDALONE
class FvBaseAspect
{
public:
    FvBaseAspect() = default;
    virtual ~FvBaseAspect() = default;

    virtual void setVariantValue(const QVariant &value) = 0;
    virtual void setDefaultVariantValue(const QVariant &value) = 0;
    virtual QVariant variantValue() const = 0;
    virtual QVariant defaultVariantValue() const = 0;

    void setSettingsKey(const QString &group, const QString &key);
    QString settingsKey() const;
    void setCheckable(bool) {}
    void setDisplayName(const QString &) {}
    void setToolTip(const QString &) {}

private:
    QString m_settingsGroup;
    QString m_settingsKey;
};

template <class ValueType>
class FvTypedAspect : public FvBaseAspect
{
public:
    void setVariantValue(const QVariant &value) override
    {
        m_value = value.value<ValueType>();
    }
    void setDefaultVariantValue(const QVariant &value) override
    {
        m_defaultValue = value.value<ValueType>();
    }
    QVariant variantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_value);
    }
    QVariant defaultVariantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_defaultValue);
    }

    ValueType value() const { return m_value; }
    ValueType operator()() const { return m_value; }

    ValueType m_value;
    ValueType m_defaultValue;
};

using FvBoolAspect = FvTypedAspect<bool>;
using FvIntegerAspect = FvTypedAspect<qint64>;
using FvStringAspect = FvTypedAspect<QString>;
using FvFilePathAspect = FvTypedAspect<Utils::FilePath>;

class FvAspectContainer : public FvBaseAspect
{
public:
};

#else

using FvAspectContainer = Core::PagedSettings;
using FvBaseAspect = Utils::BaseAspect;
using FvBoolAspect = Utils::BoolAspect;
using FvIntegerAspect = Utils::IntegerAspect;
using FvStringAspect = Utils::StringAspect;
using FvFilePathAspect = Utils::FilePathAspect;

#endif

class FakeVimSettings final : public FvAspectContainer
{
public:
    FakeVimSettings();
    ~FakeVimSettings();

    FvBaseAspect *item(const QString &name);
    QString trySetValue(const QString &name, const QString &value);

    FvBoolAspect useFakeVim;
    FvBoolAspect readVimRc;
    FvFilePathAspect vimRcPath;

    FvBoolAspect startOfLine;
    FvIntegerAspect tabStop;
    FvBoolAspect hlSearch;
    FvBoolAspect smartTab;
    FvIntegerAspect shiftWidth;
    FvBoolAspect expandTab;
    FvBoolAspect autoIndent;
    FvBoolAspect smartIndent;

    FvBoolAspect incSearch;
    FvBoolAspect useCoreSearch;
    FvBoolAspect smartCase;
    FvBoolAspect ignoreCase;
    FvBoolAspect wrapScan;

    // command ~ behaves as g~
    FvBoolAspect tildeOp;

    // indent  allow backspacing over autoindent
    // eol     allow backspacing over line breaks (join lines)
    // start   allow backspacing over the start of insert; CTRL-W and CTRL-U
    //         stop once at the start of insert.
    FvStringAspect backspace;

    // @,48-57,_,192-255
    FvStringAspect isKeyword;

    // other actions
    FvBoolAspect showMarks;
    FvBoolAspect passControlKey;
    FvBoolAspect passKeys;
    FvStringAspect clipboard;
    FvBoolAspect showCmd;
    FvIntegerAspect scrollOff;
    FvBoolAspect relativeNumber;
    FvStringAspect formatOptions;

    // Plugin emulation
    FvBoolAspect emulateVimCommentary;
    FvBoolAspect emulateReplaceWithRegister;
    FvBoolAspect emulateExchange;
    FvBoolAspect emulateArgTextObj;
    FvBoolAspect emulateSurround;

    FvBoolAspect blinkingCursor;

private:
    void setup(FvBaseAspect *aspect, const QVariant &value,
                      const QString &settingsKey,
                      const QString &shortName,
                      const QString &label);

    QHash<QString, FvBaseAspect *> m_nameToAspect;
    QHash<FvBaseAspect *, QString> m_aspectToName;
};

FakeVimSettings &settings();

} // FakeVim::Internal
