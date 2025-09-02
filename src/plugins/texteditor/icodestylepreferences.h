// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tabsettings.h"
#include "texteditor_global.h"

#include <utils/id.h>
#include <utils/store.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QVariant;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal { class ICodeStylePreferencesPrivate; }

class CodeStylePool;

class TEXTEDITOR_EXPORT ICodeStylePreferences : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
public:
    // pool is a pool which will be used by this preferences for setting delegates
    explicit ICodeStylePreferences(QObject *parentObject = nullptr);
    ~ICodeStylePreferences() override;

    QByteArray id() const;
    void setId(const QByteArray &name);

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isReadOnly() const;
    void setReadOnly(bool on);

    void setTabSettings(const TabSettings &settings);
    TabSettings tabSettings() const;
    TabSettings currentTabSettings() const;

    virtual QVariant value() const = 0;
    virtual void setValue(const QVariant &) = 0;

    QVariant currentValue() const; // may be from grandparent

    ICodeStylePreferences *currentPreferences() const; // may be grandparent

    CodeStylePool *delegatingPool() const;
    void setDelegatingPool(CodeStylePool *pool);

    ICodeStylePreferences *currentDelegate() const; // null or one of delegates from the pool
    void setCurrentDelegate(ICodeStylePreferences *delegate);

    QByteArray currentDelegateId() const;
    void setCurrentDelegate(const QByteArray &id);

    void setSettingsSuffix(const Utils::Key &suffix);
    void toSettings(const Utils::Key &category) const;
    void fromSettings(const Utils::Key &category);

    // make below 2 protected?
    virtual Utils::Store toMap() const;
    virtual void fromMap(const Utils::Store &map);

    Utils::Id globalSettingsCategory();
    void setGlobalSettingsCategory(const Utils::Id &id);

signals:
    void tabSettingsChanged(const TextEditor::TabSettings &settings);
    void currentTabSettingsChanged(const TextEditor::TabSettings &settings);
    void valueChanged(const QVariant &);
    void currentValueChanged(const QVariant &);
    void currentDelegateChanged(TextEditor::ICodeStylePreferences *currentDelegate);
    void currentPreferencesChanged(TextEditor::ICodeStylePreferences *currentPreferences);
    void displayNameChanged(const QString &newName);
    void aboutToBeRemoved(TextEditor::ICodeStylePreferences *preferences);
    void aboutToBeCopied(TextEditor::ICodeStylePreferences *current,
                         TextEditor::ICodeStylePreferences *target);

private:
    void codeStyleRemoved(ICodeStylePreferences *preferences);

    Internal::ICodeStylePreferencesPrivate *d;
};

template <typename T>
class TypedCodeStylePreferences : public ICodeStylePreferences
{
public:
    TypedCodeStylePreferences(QObject *parent = nullptr)
        : ICodeStylePreferences(parent)
    {
        setSettingsSuffix("CodeStyleSettings");
        setGlobalSettingsCategory(T::settingsId());
    }

    QVariant value() const final
    {
        QVariant v;
        v.setValue(codeStyleSettings());
        return v;
    }
    void setValue(const QVariant &data) final
    {
        if (!data.canConvert<T>())
            return;

        setCodeStyleSettings(data.value<T>());
    }

    T codeStyleSettings() const { return m_data; }

    // Tracks parent hierarchy until currentParentSettings is null. TODO: return optional?
    T currentCodeStyleSettings() const
    {
        QVariant v = currentValue();
        if (!v.canConvert<T>()) {
            // warning
            return {};
        }
        return v.value<T>();
    }

    Utils::Store toMap() const override
    {
        Utils::Store map = ICodeStylePreferences::toMap();
        if (!currentDelegate()) {
            const Utils::Store dataMap = m_data.toMap();
            for (auto it = dataMap.begin(), end = dataMap.end(); it != end; ++it)
                map.insert(it.key(), it.value());
        }
        return map;
    }
    void fromMap(const Utils::Store &map) override
    {
        ICodeStylePreferences::fromMap(map);
        if (!currentDelegate())
            m_data.fromMap(map);
    }

    void setCodeStyleSettings(const T &data)
    {
        if (m_data == data)
            return;

        m_data = data;

        QVariant v;
        v.setValue(data);
        emit valueChanged(v);
        if (!currentDelegate())
            emit currentValueChanged(v);
    }

private:
    T m_data;
};

} // namespace TextEditor
