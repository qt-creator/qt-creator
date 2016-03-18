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

#include <QObject>
#include <QString>
#include <QVariant>

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

    ValgrindBaseSettings() {}

    void toMap(QVariantMap &map) const;
    void fromMap(const QVariantMap &map);

signals:
    void changed(); // sent when multiple values have changed simulatenously (e.g. fromMap)

/**
 * Base valgrind settings
 */
public:
    QString valgrindExecutable() const;
    SelfModifyingCodeDetection selfModifyingCodeDetection() const;

public slots:
    void setValgrindExecutable(const QString &);
    void setSelfModifyingCodeDetection(int);

signals:
    void valgrindExecutableChanged(const QString &);
    void selfModifyingCodeDetectionChanged(int);

private:
    QString m_valgrindExecutable;
    SelfModifyingCodeDetection m_selfModifyingCodeDetection;


/**
 * Base memcheck settings
 */
public:
    int numCallers() const { return m_numCallers; }
    LeakCheckOnFinish leakCheckOnFinish() const { return m_leakCheckOnFinish; }
    bool showReachable() const { return m_showReachable; }
    bool trackOrigins() const { return m_trackOrigins; }
    bool filterExternalIssues() const { return m_filterExternalIssues; }
    QList<int> visibleErrorKinds() const { return m_visibleErrorKinds; }

    virtual QStringList suppressionFiles() const = 0;
    virtual void addSuppressionFiles(const QStringList &) = 0;
    virtual void removeSuppressionFiles(const QStringList &) = 0;

public slots:
    void setNumCallers(int);
    void setLeakCheckOnFinish(int);
    void setShowReachable(bool);
    void setTrackOrigins(bool);
    void setFilterExternalIssues(bool);
    void setVisibleErrorKinds(const QList<int> &);

signals:
    void numCallersChanged(int);
    void leakCheckOnFinishChanged(int);
    void showReachableChanged(bool);
    void trackOriginsChanged(bool);
    void filterExternalIssuesChanged(bool);
    void visibleErrorKindsChanged(const QList<int> &);
    void suppressionFilesRemoved(const QStringList &);
    void suppressionFilesAdded(const QStringList &);

protected:
    int m_numCallers;
    LeakCheckOnFinish m_leakCheckOnFinish;
    bool m_showReachable;
    bool m_trackOrigins;
    bool m_filterExternalIssues;
    QList<int> m_visibleErrorKinds;

/**
 * Base callgrind settings
 */
public:
    bool enableCacheSim() const { return m_enableCacheSim; }
    bool enableBranchSim() const { return m_enableBranchSim; }
    bool collectSystime() const { return m_collectSystime; }
    bool collectBusEvents() const { return m_collectBusEvents; }
    bool enableEventToolTips() const { return m_enableEventToolTips; }

    /// \return Minimum cost ratio, range [0.0..100.0]
    double minimumInclusiveCostRatio() const { return m_minimumInclusiveCostRatio; }

    /// \return Minimum cost ratio, range [0.0..100.0]
    double visualisationMinimumInclusiveCostRatio() const { return m_visualisationMinimumInclusiveCostRatio; }

public slots:
    void setEnableCacheSim(bool enable);
    void setEnableBranchSim(bool enable);
    void setCollectSystime(bool collect);
    void setCollectBusEvents(bool collect);
    void setEnableEventToolTips(bool enable);

    /// \param minimumInclusiveCostRatio Minimum inclusive cost ratio, valid values are [0.0..100.0]
    void setMinimumInclusiveCostRatio(double minimumInclusiveCostRatio);

    /// \param minimumInclusiveCostRatio Minimum inclusive cost ratio, valid values are [0.0..100.0]
    void setVisualisationMinimumInclusiveCostRatio(double minimumInclusiveCostRatio);

signals:
    void enableCacheSimChanged(bool);
    void enableBranchSimChanged(bool);
    void collectSystimeChanged(bool);
    void collectBusEventsChanged(bool);
    void enableEventToolTipsChanged(bool);
    void minimumInclusiveCostRatioChanged(double);
    void visualisationMinimumInclusiveCostRatioChanged(double);

private:
    bool m_enableCacheSim;
    bool m_collectSystime;
    bool m_collectBusEvents;
    bool m_enableBranchSim;
    bool m_enableEventToolTips;
    double m_minimumInclusiveCostRatio;
    double m_visualisationMinimumInclusiveCostRatio;
};


/**
 * Global valgrind settings
 */
class ValgrindGlobalSettings : public ValgrindBaseSettings
{
    Q_OBJECT

public:
    ValgrindGlobalSettings();

    QWidget *createConfigWidget(QWidget *parent);
    void toMap(QVariantMap &map) const;
    void fromMap(const QVariantMap &map);
    ISettingsAspect *create() const { return new ValgrindGlobalSettings; }


    /*
     * Global memcheck settings
     */
public:
    QStringList suppressionFiles() const;
    // in the global settings we change the internal list directly
    void addSuppressionFiles(const QStringList &);
    void removeSuppressionFiles(const QStringList &);

    // internal settings which don't require any UI
    void setLastSuppressionDialogDirectory(const QString &directory);
    QString lastSuppressionDialogDirectory() const;

    void setLastSuppressionDialogHistory(const QStringList &history);
    QStringList lastSuppressionDialogHistory() const;

    void writeSettings() const;
    void readSettings();

private:
    QStringList m_suppressionFiles;
    QString m_lastSuppressionDirectory;
    QStringList m_lastSuppressionHistory;


    /**
     * Global callgrind settings
     */
public:
    CostDelegate::CostFormat costFormat() const;
    bool detectCycles() const;
    bool shortenTemplates() const;

public slots:
    void setCostFormat(Valgrind::Internal::CostDelegate::CostFormat format);
    void setDetectCycles(bool on);
    void setShortenTemplates(bool on);

private:
    CostDelegate::CostFormat m_costFormat;
    bool m_detectCycles;
    bool m_shortenTemplates;
};


/**
 * Per-project valgrind settings.
 */
class ValgrindProjectSettings : public ValgrindBaseSettings
{
    Q_OBJECT

public:
    ValgrindProjectSettings() {}

    QWidget *createConfigWidget(QWidget *parent);
    void toMap(QVariantMap &map) const;
    void fromMap(const QVariantMap &map);
    ISettingsAspect *create() const { return new ValgrindProjectSettings; }

    /**
     * Per-project memcheck settings, saves a diff to the global suppression files list
     */
public:
    QStringList suppressionFiles() const;
    // in the project-specific settings we store a diff to the global list
    void addSuppressionFiles(const QStringList &suppressions);
    void removeSuppressionFiles(const QStringList &suppressions);

private:
    QStringList m_disabledGlobalSuppressionFiles;
    QStringList m_addedSuppressionFiles;


    /**
     * Per-project callgrind settings, saves a diff to the global suppression files list
     */
};

} // namespace Internal
} // namespace Valgrind
