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

#include "projectexplorer_export.h"

#include <coreplugin/id.h>
#include <utils/macroexpander.h>

#include <QObject>
#include <QString>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QFormLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT ProjectConfigurationAspect : public QObject
{
    Q_OBJECT

public:
    ProjectConfigurationAspect();
    ~ProjectConfigurationAspect() override;

    void setId(Core::Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setSettingsKey(const QString &settingsKey) { m_settingsKey = settingsKey; }

    Core::Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    QString settingsKey() const { return  m_settingsKey; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    using ConfigWidgetCreator = std::function<QWidget *()>;
    void setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator);
    QWidget *createConfigWidget() const;

    virtual void fromMap(const QVariantMap &) {}
    virtual void toMap(QVariantMap &) const {}
    virtual void addToConfigurationLayout(QFormLayout *) {}

signals:
    void changed();

protected:
    Core::Id m_id;
    QString m_displayName;
    QString m_settingsKey; // Name of data in settings.
    bool m_visible = true;
    ConfigWidgetCreator m_configWidgetCreator;
};

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public QObject
{
    Q_OBJECT

protected:
    explicit ProjectConfiguration(QObject *parent, Core::Id id);

public:
    ~ProjectConfiguration() override;

    Core::Id id() const;

    QString displayName() const;

    bool usesDefaultDisplayName() const;
    void setDisplayName(const QString &name);
    void setDefaultDisplayName(const QString &name);

    void setToolTip(const QString &text);
    QString toolTip() const;

    // Note: Make sure subclasses call the superclasses' fromMap() function!
    virtual bool fromMap(const QVariantMap &map);

    // Note: Make sure subclasses call the superclasses' toMap() function!
    virtual QVariantMap toMap() const;

    Utils::MacroExpander *macroExpander() { return &m_macroExpander; }
    const Utils::MacroExpander *macroExpander() const { return &m_macroExpander; }

    virtual Project *project() const = 0;

    virtual bool isActive() const = 0;

    static QString settingsIdKey();

    template<class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        auto aspect = new Aspect(args...);
        m_aspects.append(aspect);
        return aspect;
    }

    const QList<ProjectConfigurationAspect *> aspects() const { return m_aspects; }

    ProjectConfigurationAspect *aspect(Core::Id id) const;

    template <typename T> T *aspect() const
    {
        for (ProjectConfigurationAspect *aspect : m_aspects)
            if (T *result = qobject_cast<T *>(aspect))
                return result;
        return nullptr;
    }

signals:
    void displayNameChanged();
    void toolTipChanged();

protected:
    QList<ProjectConfigurationAspect *> m_aspects;

private:
    const Core::Id m_id;
    QString m_displayName;
    QString m_defaultDisplayName;
    QString m_toolTip;
    Utils::MacroExpander m_macroExpander;
};

class PROJECTEXPLORER_EXPORT StatefulProjectConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    StatefulProjectConfiguration() = default;

    bool isEnabled() const;

    virtual QString disabledReason() const = 0;

signals:
    void enabledChanged();

protected:
    StatefulProjectConfiguration(QObject *parent, Core::Id id);

    void setEnabled(bool enabled);

private:
    bool m_isEnabled = false;
};

// helper function:
PROJECTEXPLORER_EXPORT Core::Id idFromMap(const QVariantMap &map);

} // namespace ProjectExplorer
