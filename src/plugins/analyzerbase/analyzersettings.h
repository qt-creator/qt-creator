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
class ANALYZER_EXPORT ISettingsAspect : public QObject
{
    Q_OBJECT

public:
    ISettingsAspect() {}

    /// Converts current object into map for storage.
    virtual void toMap(QVariantMap &map) const = 0;
    /// Read object state from @p map.
    virtual void fromMap(const QVariantMap &map) = 0;

    /// Create a configuration widget for this settings aspect.
    virtual QWidget *createConfigWidget(QWidget *parent) = 0;
    /// "Virtual default constructor"
    virtual ISettingsAspect *create() const = 0;
    /// "Virtual copy constructor"
    ISettingsAspect *clone() const;
};


/**
 * Settings associated with a single project/run configuration
 *
 */
class ANALYZER_EXPORT AnalyzerRunConfigurationAspect
    : public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    AnalyzerRunConfigurationAspect(ISettingsAspect *projectSettings,
                                   ISettingsAspect *globalSettings);

    ~AnalyzerRunConfigurationAspect();

    AnalyzerRunConfigurationAspect *clone(ProjectExplorer::RunConfiguration *parent) const;

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetCustomToGlobalSettings();

    ISettingsAspect *projectSettings() const { return m_projectSettings; }
    ISettingsAspect *globalSettings() const { return m_globalSettings; }
    ISettingsAspect *currentSettings() const;
    ProjectExplorer::RunConfigWidget *createConfigurationWidget();

protected:
    void fromMap(const QVariantMap &map);
    void toMap(QVariantMap &map) const;

private:
    bool m_useGlobalSettings;
    ISettingsAspect *m_projectSettings;
    ISettingsAspect *m_globalSettings;
};

} // namespace Analyzer

#endif // ANALYZER_INTERNAL_ANALYZERSETTINGS_H
