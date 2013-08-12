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

#include "analyzerbase_global.h"

#include <projectexplorer/runconfiguration.h>

namespace Analyzer {

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

    /// convert current configuration into map for storage
    virtual void toMap(QVariantMap &map) const = 0;
    /// read configuration from @p map
    virtual void fromMap(const QVariantMap &map) = 0;

    /// create a configuration widget for this configuration
    virtual QWidget *createConfigWidget(QWidget *parent) = 0;
    /// clones s AbstractAnalyzerSubConfig
    virtual AbstractAnalyzerSubConfig *clone() = 0;
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
    : public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    AnalyzerRunConfigurationAspect(AbstractAnalyzerSubConfig *customConfiguration,
                                   AbstractAnalyzerSubConfig *globalConfiguration);

    ~AnalyzerRunConfigurationAspect();

    AnalyzerRunConfigurationAspect *clone(ProjectExplorer::RunConfiguration *parent) const;

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetCustomToGlobalSettings();

    AbstractAnalyzerSubConfig *customSubConfig() const { return m_customConfiguration; }
    AbstractAnalyzerSubConfig *globalSubConfig() const { return m_globalConfiguration; }
    AbstractAnalyzerSubConfig *currentConfig() const;
    ProjectExplorer::RunConfigWidget *createConfigurationWidget();

protected:
    void fromMap(const QVariantMap &map);
    void toMap(QVariantMap &map) const;

private:
    bool m_useGlobalSettings;
    AbstractAnalyzerSubConfig *m_customConfiguration;
    AbstractAnalyzerSubConfig *m_globalConfiguration;
};

} // namespace Analyzer

#endif // ANALYZER_INTERNAL_ANALYZERSETTINGS_H
