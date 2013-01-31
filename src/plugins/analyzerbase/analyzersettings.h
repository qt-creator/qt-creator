/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANALYZER_INTERNAL_ANALYZERSETTINGS_H
#define ANALYZER_INTERNAL_ANALYZERSETTINGS_H

#include <QObject>
#include <QVariant>

#include "analyzerbase_global.h"

#include <coreplugin/id.h>
#include <projectexplorer/runconfiguration.h>

namespace Analyzer {

class IAnalyzerTool;

/**
 * Utility function to set @p val if @p key is present in @p map.
 */
template <typename T> void setIfPresent(const QVariantMap &map, const QString &key, T *val)
{
    if (!map.contains(key))
        return;
    *val = map.value(key).template value<T>();
}

/**
 * Subclass this to add configuration to your analyzer tool.
 *
 * If global and project-specific settings differ for your tool,
 * create one subclass for each.
 */
class ANALYZER_EXPORT AbstractAnalyzerSubConfig : public QObject
{
    Q_OBJECT

public:
    AbstractAnalyzerSubConfig() {}

    /// return a list of default values
    virtual QVariantMap defaults() const = 0;
    /// convert current configuration into map for storage
    virtual QVariantMap toMap() const = 0;
    /// read configuration from @p map
    virtual void fromMap(const QVariantMap &map) = 0;

    /// unique ID for this configuration
    virtual Core::Id id() const = 0;
    /// user readable display name for this configuration
    virtual QString displayName() const = 0;
    /// create a configuration widget for this configuration
    virtual QWidget *createConfigWidget(QWidget *parent) = 0;
    /// clones s AbstractAnalyzerSubConfig
    virtual AbstractAnalyzerSubConfig *clone() = 0;
};

/**
 * Shared interface for the global and per-project settings.
 *
 * Use this to get the subConfig for your tool.
 */
class ANALYZER_EXPORT AnalyzerSettings : public QObject
{
    Q_OBJECT

public:
    template<class T>
    T *subConfig() const
    {
        foreach (AbstractAnalyzerSubConfig *subConfig, subConfigs()) {
            if (T *config = qobject_cast<T *>(subConfig))
                return config;
        }
        return 0;
    }

    QList<AbstractAnalyzerSubConfig *> subConfigs() const
    {
        return m_subConfigs;
    }

    QVariantMap defaults() const;
    virtual QVariantMap toMap() const;

protected:
    virtual void fromMap(const QVariantMap &map);

    QVariantMap toMap(const QList<AbstractAnalyzerSubConfig *> &subConfigs) const;
    void fromMap(const QVariantMap &map, QList<AbstractAnalyzerSubConfig *> *subConfigs);

    AnalyzerSettings(QObject *parent);
    AnalyzerSettings(AnalyzerSettings *other);
    QList<AbstractAnalyzerSubConfig *> m_subConfigs;
};


// global and local settings are loaded and saved differently, and they also handle suppressions
// differently.
/**
 * Global settings
 *
 * To access your custom configuration use:
 * @code
 * AnalyzerGlobalSettings::instance()->subConfig<YourGlobalConfig>()->...
 * @endcode
 */
class ANALYZER_EXPORT AnalyzerGlobalSettings : public AnalyzerSettings
{
    Q_OBJECT

public:
    static AnalyzerGlobalSettings *instance();
    ~AnalyzerGlobalSettings();

    void writeSettings() const;
    void readSettings();

    void registerTool(IAnalyzerTool *tool);

private:
    AnalyzerGlobalSettings(QObject *parent);
    static AnalyzerGlobalSettings *m_instance;
};

/**
 * Settings associated with a single project/run configuration
 *
 * To access your custom configuration use:
 * @code
 * ProjectExplorer::RunConfiguration *rc = ...;
 * rc->extraAspect<AnalyzerRunConfigurationAspect>()->subConfig<YourProjectConfig>()->...
 * @endcode
 */
class ANALYZER_EXPORT AnalyzerRunConfigurationAspect
    : public AnalyzerSettings, public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    AnalyzerRunConfigurationAspect();
    AnalyzerRunConfigurationAspect(AnalyzerRunConfigurationAspect *other);
    ~AnalyzerRunConfigurationAspect();

    QString displayName() const;
    virtual QVariantMap toMap() const;

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetCustomToGlobalSettings();

    QList<AbstractAnalyzerSubConfig *> customSubConfigs() const { return m_customConfigurations; }

protected:
    virtual void fromMap(const QVariantMap &map);

private:
    bool m_useGlobalSettings;
    QList<AbstractAnalyzerSubConfig *> m_customConfigurations;
};

} // namespace Analyzer

#endif // ANALYZER_INTERNAL_ANALYZERSETTINGS_H
