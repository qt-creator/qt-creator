/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <coreplugin/icore.h>
#include <mesonpluginconstants.h>
#include <QObject>
namespace MesonProjectManager {
namespace Internal {
template<class F>
void with_group(QSettings *settings, const QString &name, const F &f)
{
    settings->beginGroup(name);
    f();
    settings->endGroup();
};

#define ADD_PROPERTY(name, setter, type) \
private: \
    type m_##name; \
\
public: \
    inline static type name() { return instance()->m_##name; } \
    inline static void setter(type value) \
    { \
        instance()->m_##name = value; \
        emit instance()->name##Changed(value); \
    } \
    Q_SIGNAL void name##Changed(type newValue);

class Settings : public QObject
{
    Q_OBJECT
    explicit Settings(QObject *parent = nullptr);

public:
    inline static Settings *instance()
    {
        static Settings m_settings;
        return &m_settings;
    }

    ADD_PROPERTY(autorunMeson, setAutorunMeson, bool)
    ADD_PROPERTY(verboseNinja, setVerboseNinja, bool)

    static inline void saveAll()
    {
        using namespace Constants;
        auto settings = Core::ICore::settings(QSettings::Scope::UserScope);
        with_group(settings, GeneralSettings::SECTION, [settings]() {
            settings->setValue(GeneralSettings::AUTORUN_MESON_KEY, Settings::autorunMeson());
            settings->setValue(GeneralSettings::VERBOSE_NINJA_KEY, Settings::verboseNinja());
        });
    }
    static inline void loadAll()
    {
        using namespace Constants;
        auto settings = Core::ICore::settings(QSettings::Scope::UserScope);
        with_group(settings, GeneralSettings::SECTION, [settings]() {
            Settings::setAutorunMeson(
                settings->value(GeneralSettings::AUTORUN_MESON_KEY, true).toBool());
            Settings::setVerboseNinja(
                settings->value(GeneralSettings::VERBOSE_NINJA_KEY, true).toBool());
        });
    }
};
} // namespace Internal
} // namespace MesonProjectManager
