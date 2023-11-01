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

    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

    void addSuppressionFile(const Utils::FilePath &suppressionFile);

private:
    void bufferToGui() override;
    bool guiToBuffer() override;

    friend class ValgrindSettings;
    SuppressionAspectPrivate *d = nullptr;
};

/**
 * Valgrind settings shared for global and per-project.
 */
class ValgrindSettings : public Utils::AspectContainer
{
    Q_OBJECT

public:
    // These exists once globally, and once per project
    explicit ValgrindSettings(bool global);

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

    // Generic valgrind settings
    Utils::FilePathAspect valgrindExecutable{this};
    Utils::StringAspect valgrindArguments{this};
    Utils::SelectionAspect selfModifyingCodeDetection{this};

    SuppressionAspect suppressions;

    // Memcheck
    Utils::StringAspect memcheckArguments{this};
    Utils::IntegerAspect numCallers{this};
    Utils::SelectionAspect leakCheckOnFinish{this};
    Utils::BoolAspect showReachable{this};
    Utils::BoolAspect trackOrigins{this};
    Utils::BoolAspect filterExternalIssues{this};
    Utils::IntegersAspect visibleErrorKinds{this};

    Utils::FilePathAspect lastSuppressionDirectory{this}; // Global only
    Utils::StringAspect lastSuppressionHistory{this}; // Global only

    void setVisibleErrorKinds(const QList<int> &);

    // Callgrind
    Utils::StringAspect callgrindArguments{this};
    Utils::FilePathAspect kcachegrindExecutable{this};

    Utils::BoolAspect enableCacheSim{this};
    Utils::BoolAspect enableBranchSim{this};
    Utils::BoolAspect collectSystime{this};
    Utils::BoolAspect collectBusEvents{this};
    Utils::BoolAspect enableEventToolTips{this};
    Utils::DoubleAspect minimumInclusiveCostRatio{this};
    Utils::DoubleAspect visualizationMinimumInclusiveCostRatio{this};

    Utils::SelectionAspect costFormat{this}; // Global only
    Utils::BoolAspect detectCycles{this}; // Global only
    Utils::BoolAspect shortenTemplates{this}; // Global only
};

ValgrindSettings &globalSettings();

} // Valgrind::Internal
