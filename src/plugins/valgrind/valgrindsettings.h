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

class SuppressionAspectPrivate;

class SuppressionAspect final : public Utils::BaseAspect
{
    Q_DECLARE_TR_FUNCTIONS(Valgrind::Internal::SuppressionAspect)

public:
    explicit SuppressionAspect(bool global);
    ~SuppressionAspect() final;

    QStringList value() const;
    void setValue(const QStringList &val);

    void addToLayout(Utils::LayoutBuilder &builder) final;

    void fromMap(const QVariantMap &map) final;
    void toMap(QVariantMap &map) const final;

    QVariant volatileValue() const final;
    void setVolatileValue(const QVariant &val) final;

    void cancel() final;
    void apply() final;
    void finish() final;

    void addSuppressionFile(const QString &suppressionFile);

private:
    friend class ValgrindBaseSettings;
    SuppressionAspectPrivate *d = nullptr;
};

/**
 * Valgrind settings shared for global and per-project.
 */
class ValgrindBaseSettings : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT

public:
    explicit ValgrindBaseSettings(bool global);

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

signals:
    void changed(); // sent when multiple values have changed simulatenously (e.g. fromMap)

/**
 * Base valgrind settings
 */
public:
    Utils::StringAspect valgrindExecutable;
    Utils::StringAspect valgrindArguments;
    Utils::SelectionAspect selfModifyingCodeDetection;

    SuppressionAspect suppressions;

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

    void setVisibleErrorKinds(const QList<int> &);

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

    void writeSettings() const;
    void readSettings();

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
};

} // namespace Internal
} // namespace Valgrind
