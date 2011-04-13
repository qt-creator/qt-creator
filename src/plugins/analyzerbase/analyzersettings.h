/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ANALYZER_INTERNAL_ANALYZERSETTINGS_H
#define ANALYZER_INTERNAL_ANALYZERSETTINGS_H

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "analyzerbase_global.h"

#include <projectexplorer/runconfiguration.h>

namespace Analyzer {

class AnalyzerSettings;

/**
 * Utility function to set @p val if @p key is present in @p map.
 */
template <typename T> static void setIfPresent(const QVariantMap &map, const QString &key, T *val)
{
    if (!map.contains(key))
        return;
    *val = map.value(key).template value<T>();
}

/**
 * Subclass this to add configuration to your analyzer tool.
 */
class ANALYZER_EXPORT AbstractAnalyzerSubConfig : public QObject
{
    Q_OBJECT
public:
    AbstractAnalyzerSubConfig(QObject *parent);
    virtual ~AbstractAnalyzerSubConfig();

    virtual QVariantMap defaults() const = 0;
    virtual QVariantMap toMap() const = 0;
    virtual bool fromMap(const QVariantMap &map) = 0;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QWidget *createConfigWidget(QWidget *parent) = 0;
};

/**
 * Interface for registering configuration to the Manager.
 * You probably want to use the template class below.
 */
class ANALYZER_EXPORT AbstractAnalyzerSubConfigFactory
{
public:
    AbstractAnalyzerSubConfigFactory(){}
    ~AbstractAnalyzerSubConfigFactory(){}

    virtual AbstractAnalyzerSubConfig *createGlobalSubConfig(QObject *parent) = 0;
    virtual AbstractAnalyzerSubConfig *createProjectSubConfig(QObject *parent) = 0;
};

/**
 * Makes it easy to register configuration for a tool:
 *
 * @code
 * manager->registerSubConfigFactory(new AnalyzerSubConfigFactory<MemcheckGlobalSettings, MemcheckProjectSettings>);
 * @endcode
 */
template<class GlobalConfigT, class ProjectConfigT>
class ANALYZER_EXPORT AnalyzerSubConfigFactory : public AbstractAnalyzerSubConfigFactory
{
public:
    AnalyzerSubConfigFactory(){}

    AbstractAnalyzerSubConfig *createGlobalSubConfig(QObject *parent)
    {
        return new GlobalConfigT(parent);
    }

    AbstractAnalyzerSubConfig *createProjectSubConfig(QObject *parent)
    {
        return new ProjectConfigT(parent);
    }
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
    virtual ~AnalyzerSettings();

    template<class T>
    T* subConfig() const
    {
        return findChild<T *>();
    }

    QList<AbstractAnalyzerSubConfig *> subConfigs() const
    {
        return findChildren<AbstractAnalyzerSubConfig *>();
    }

    QVariantMap defaults() const;
    virtual QVariantMap toMap() const;

protected:
    void addSubConfig(AbstractAnalyzerSubConfig *config)
    {
        config->setParent(this);
    }

    virtual bool fromMap(const QVariantMap &map);

    AnalyzerSettings(QObject *parent);
};


// global and local settings are loaded and saved differently, and they also handle suppressions
// differently.
/**
 * Global settings
 */
class ANALYZER_EXPORT AnalyzerGlobalSettings : public AnalyzerSettings
{
    Q_OBJECT
public:
    static AnalyzerGlobalSettings *instance();
    ~AnalyzerGlobalSettings();

    void writeSettings() const;
    void readSettings();

    void registerSubConfigFactory(AbstractAnalyzerSubConfigFactory *factory);
    QList<AbstractAnalyzerSubConfigFactory *> subConfigFactories() const;

private:
    AnalyzerGlobalSettings(QObject *parent);
    static AnalyzerGlobalSettings *m_instance;
    QList<AbstractAnalyzerSubConfigFactory *> m_subConfigFactories;
};

/**
 * Settings associated with a single project/run configuration
 */
class ANALYZER_EXPORT AnalyzerProjectSettings : public AnalyzerSettings, public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT
public:
    AnalyzerProjectSettings(QObject *parent = 0);
    virtual ~AnalyzerProjectSettings();

    QString displayName() const;

    virtual QVariantMap toMap() const;

protected:
    virtual bool fromMap(const QVariantMap &map);
};

}

#endif // ANALYZER_INTERNAL_ANALYZERSETTINGS_H
