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

#include <utils/displayname.h>
#include <utils/id.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>
#include <QWidget>

namespace ProjectExplorer {

class Project;
class ProjectConfigurationAspects;
class Target;

class PROJECTEXPLORER_EXPORT LayoutBuilder
{
public:
    explicit LayoutBuilder(QWidget *parent);
    ~LayoutBuilder();

    class LayoutItem
    {
    public:
        LayoutItem(QLayout *layout) : layout(layout) {}
        LayoutItem(QWidget *widget) : widget(widget) {}
        LayoutItem(const QString &text) : text(text) {}

        QLayout *layout = nullptr;
        QWidget *widget = nullptr;
        QString text;
    };

    template<typename ...Items>
    LayoutBuilder &addItems(LayoutItem first, Items... rest) {
        return addItem(first).addItems(rest...);
    }
    LayoutBuilder &addItems() { return *this; }
    LayoutBuilder &addItem(LayoutItem item);

    LayoutBuilder &startNewRow();

    QLayout *layout() const;

private:
    void flushPendingItems();

    QLayout *m_layout = nullptr;
    QList<LayoutItem> m_pendingItems;
};

class PROJECTEXPLORER_EXPORT ProjectConfigurationAspect : public QObject
{
    Q_OBJECT

public:
    ProjectConfigurationAspect();
    ~ProjectConfigurationAspect() override;

    void setId(Utils::Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setSettingsKey(const QString &settingsKey) { m_settingsKey = settingsKey; }

    Utils::Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    QString settingsKey() const { return  m_settingsKey; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    using ConfigWidgetCreator = std::function<QWidget *()>;
    void setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator);
    QWidget *createConfigWidget() const;

    virtual void fromMap(const QVariantMap &) {}
    virtual void toMap(QVariantMap &) const {}
    virtual void acquaintSiblings(const ProjectConfigurationAspects &) {}

    virtual void addToLayout(LayoutBuilder &builder);

signals:
    void changed();

protected:
    virtual void setVisibleDynamic(bool visible) { Q_UNUSED(visible) } // TODO: Better name? Merge with setVisible() somehow?

    Utils::Id m_id;
    QString m_displayName;
    QString m_settingsKey; // Name of data in settings.
    bool m_visible = true;
    ConfigWidgetCreator m_configWidgetCreator;
};

class PROJECTEXPLORER_EXPORT ProjectConfigurationAspects
        : private QList<ProjectConfigurationAspect *>
{
    using Base = QList<ProjectConfigurationAspect *>;

public:
    ProjectConfigurationAspects();
    ProjectConfigurationAspects(const ProjectConfigurationAspects &) = delete;
    ProjectConfigurationAspects &operator=(const ProjectConfigurationAspects &) = delete;
    ~ProjectConfigurationAspects();

    template <class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        auto aspect = new Aspect(args...);
        append(aspect);
        return aspect;
    }

    ProjectConfigurationAspect *aspect(Utils::Id id) const;

    template <typename T> T *aspect() const
    {
        for (ProjectConfigurationAspect *aspect : *this)
            if (T *result = qobject_cast<T *>(aspect))
                return result;
        return nullptr;
    }

    void fromMap(const QVariantMap &map) const;
    void toMap(QVariantMap &map) const;

    using Base::append;
    using Base::begin;
    using Base::end;

private:
    Base &base() { return *this; }
    const Base &base() const { return *this; }
};

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public QObject
{
    Q_OBJECT

protected:
    explicit ProjectConfiguration(QObject *parent, Utils::Id id);

public:
    ~ProjectConfiguration() override;

    Utils::Id id() const;

    QString displayName() const { return m_displayName.value(); }
    bool usesDefaultDisplayName() const { return m_displayName.usesDefaultValue(); }
    void setDisplayName(const QString &name);
    void setDefaultDisplayName(const QString &name);

    void setToolTip(const QString &text);
    QString toolTip() const;

    // Note: Make sure subclasses call the superclasses' fromMap() function!
    virtual bool fromMap(const QVariantMap &map);

    // Note: Make sure subclasses call the superclasses' toMap() function!
    virtual QVariantMap toMap() const;

    Target *target() const;
    Project *project() const;

    static QString settingsIdKey();

    template<class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        return m_aspects.addAspect<Aspect>(std::forward<Args>(args)...);
    }

    const ProjectConfigurationAspects &aspects() const { return m_aspects; }

    ProjectConfigurationAspect *aspect(Utils::Id id) const;
    template <typename T> T *aspect() const { return m_aspects.aspect<T>(); }

    void acquaintAspects();

signals:
    void displayNameChanged();
    void toolTipChanged();

protected:
    ProjectConfigurationAspects m_aspects;

private:
    QPointer<Target> m_target;
    const Utils::Id m_id;
    Utils::DisplayName m_displayName;
    QString m_toolTip;
};

// helper function:
PROJECTEXPLORER_EXPORT Utils::Id idFromMap(const QVariantMap &map);

} // namespace ProjectExplorer
