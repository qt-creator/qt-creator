// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Valgrind::Internal {

const char ANALYZER_VALGRIND_SETTINGS[] = "Analyzer.Valgrind.Settings";

class SuppressionAspectPrivate;

class SuppressionAspect final : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit SuppressionAspect(bool global);
    ~SuppressionAspect() final;

    Utils::FilePaths value() const;
    void setValue(const Utils::FilePaths &val);

    void addToLayout(Layouting::LayoutItem &parent) final;

    void fromMap(const QVariantMap &map) final;
    void toMap(QVariantMap &map) const final;

    QVariant volatileValue() const final;
    void setVolatileValue(const QVariant &val) final;

    void addSuppressionFile(const Utils::FilePath &suppressionFile);

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
    Utils::FilePathAspect valgrindExecutable;
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

} // Valgrind::Internal
