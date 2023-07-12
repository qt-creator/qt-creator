// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Valgrind::Internal {

const char ANALYZER_VALGRIND_SETTINGS[] = "Analyzer.Valgrind.Settings";

class SuppressionAspectPrivate;

class SuppressionAspect final : public Utils::TypedAspect<Utils::FilePaths>
{
    Q_OBJECT

public:
    SuppressionAspect(Utils::AspectContainer *container, bool global);
    ~SuppressionAspect() final;

    void addToLayout(Layouting::LayoutItem &parent) final;

    void fromMap(const QVariantMap &map) final;
    void toMap(QVariantMap &map) const final;

    void addSuppressionFile(const Utils::FilePath &suppressionFile);

private:
    void bufferToGui() override;
    void guiToBuffer() override;

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

/**
 * Base valgrind settings
 */
public:
    Utils::FilePathAspect valgrindExecutable{this};
    Utils::StringAspect valgrindArguments{this};
    Utils::SelectionAspect selfModifyingCodeDetection{this};

    SuppressionAspect suppressions;

/**
 * Base memcheck settings
 */
public:
    Utils::StringAspect memcheckArguments{this};
    Utils::IntegerAspect numCallers{this};
    Utils::SelectionAspect leakCheckOnFinish{this};
    Utils::BoolAspect showReachable{this};
    Utils::BoolAspect trackOrigins{this};
    Utils::BoolAspect filterExternalIssues{this};
    Utils::IntegersAspect visibleErrorKinds{this};

    void setVisibleErrorKinds(const QList<int> &);

/**
 * Base callgrind settings
 */
public:
    Utils::StringAspect callgrindArguments{this};
    Utils::FilePathAspect kcachegrindExecutable{this};

    Utils::BoolAspect enableCacheSim{this};
    Utils::BoolAspect enableBranchSim{this};
    Utils::BoolAspect collectSystime{this};
    Utils::BoolAspect collectBusEvents{this};
    Utils::BoolAspect enableEventToolTips{this};
    Utils::DoubleAspect minimumInclusiveCostRatio{this};
    Utils::DoubleAspect visualizationMinimumInclusiveCostRatio{this};

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

    Utils::FilePathAspect lastSuppressionDirectory{this};
    Utils::StringAspect lastSuppressionHistory{this};


    /**
     * Global callgrind settings
     */
    Utils::SelectionAspect costFormat{this};
    Utils::BoolAspect detectCycles{this};
    Utils::BoolAspect shortenTemplates{this};
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
