/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "callgrindcostdelegate.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Valgrind {
namespace Internal {

const char ANALYZER_VALGRIND_SETTINGS[] = "Analyzer.Valgrind.Settings";

/**
 * Valgrind settings shared for global and per-project.
 */
class ValgrindBaseSettings : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT

public:
    ValgrindBaseSettings();

    enum SelfModifyingCodeDetection {
        DetectSmcNo,
        DetectSmcStackOnly,
        DetectSmcEverywhere,
        DetectSmcEverywhereButFile
    };

    enum LeakCheckOnFinish {
        LeakCheckOnFinishNo,
        LeakCheckOnFinishSummaryOnly,
        LeakCheckOnFinishYes
    };

    void toMap(QVariantMap &map) const override;
    void fromMap(const QVariantMap &map) override;

signals:
    void changed(); // sent when multiple values have changed simulatenously (e.g. fromMap)

/**
 * Base valgrind settings
 */
public:
    Utils::StringAspect valgrindExecutable;
    Utils::StringAspect valgrindArguments;
    Utils::SelectionAspect selfModifyingCodeDetection;

/**
 * Base memcheck settings
 */
public:
    Utils::StringAspect memcheckArguments;
    Utils::IntegerAspect numCallers;
    Utils::SelectionAspect leakCheckOnFinish;
    Utils::BoolAspect showReachable;
    Utils::BoolAspect trackOrigins;
    Utils::BoolAspect filterExternalIssues;
    Utils::IntegersAspect visibleErrorKinds;

    virtual QStringList suppressionFiles() const = 0;
    virtual void addSuppressionFiles(const QStringList &) = 0;
    virtual void removeSuppressionFiles(const QStringList &) = 0;

    void setVisibleErrorKinds(const QList<int> &);

signals:
    void suppressionFilesRemoved(const QStringList &);
    void suppressionFilesAdded(const QStringList &);

/**
 * Base callgrind settings
 */
public:
    Utils::StringAspect callgrindArguments;
    Utils::StringAspect kcachegrindExecutable;

    Utils::BoolAspect enableCacheSim;
    Utils::BoolAspect enableBranchSim;
    Utils::BoolAspect collectSystime;
    Utils::BoolAspect collectBusEvents;
    Utils::BoolAspect enableEventToolTips;
    Utils::DoubleAspect minimumInclusiveCostRatio;
    Utils::DoubleAspect visualizationMinimumInclusiveCostRatio;

    Utils::AspectContainer group;
    QVariantMap defaultSettings() const;
};


/**
 * Global valgrind settings
 */
class ValgrindGlobalSettings : public ValgrindBaseSettings
{
    Q_OBJECT

public:
    ValgrindGlobalSettings();

    static ValgrindGlobalSettings *instance();

    /**
     * Global memcheck settings
     */
    QStringList suppressionFiles() const override;
    // in the global settings we change the internal list directly
    void addSuppressionFiles(const QStringList &) override;
    void removeSuppressionFiles(const QStringList &) override;

    void writeSettings() const;
    void readSettings();

    Utils::StringListAspect suppressionFiles_;
    Utils::StringAspect lastSuppressionDirectory;
    Utils::StringAspect lastSuppressionHistory;


    /**
     * Global callgrind settings
     */
    Utils::SelectionAspect costFormat;
    Utils::BoolAspect detectCycles;
    Utils::BoolAspect shortenTemplates;
};


/**
 * Per-project valgrind settings.
 */
class ValgrindProjectSettings : public ValgrindBaseSettings
{
    Q_OBJECT

public:
    ValgrindProjectSettings();

    /**
     * Per-project memcheck settings, saves a diff to the global suppression files list
     */
    QStringList suppressionFiles() const override;
    // in the project-specific settings we store a diff to the global list
    void addSuppressionFiles(const QStringList &suppressions) override;
    void removeSuppressionFiles(const QStringList &suppressions) override;

private:
    Utils::StringListAspect disabledGlobalSuppressionFiles;
    Utils::StringListAspect addedSuppressionFiles;
};

} // namespace Internal
} // namespace Valgrind
