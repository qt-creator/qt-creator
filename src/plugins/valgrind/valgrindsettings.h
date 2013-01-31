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

#ifndef ANALYZER_INTERNAL_VALGRINDSETTINGS_H
#define ANALYZER_INTERNAL_VALGRINDSETTINGS_H

#include <analyzerbase/analyzersettings.h>
#include "callgrindcostdelegate.h"

#include <QObject>
#include <QString>
#include <QVariant>

namespace Valgrind {
namespace Internal {

/**
 * Valgrind settings shared for global and per-project.
 */
class ValgrindBaseSettings : public Analyzer::AbstractAnalyzerSubConfig
{
    Q_OBJECT

public:
    ValgrindBaseSettings() {}

    virtual QVariantMap toMap() const;
    virtual QVariantMap defaults() const;
    virtual void fromMap(const QVariantMap &map);

    virtual Core::Id id() const;
    virtual QString displayName() const;

signals:
    void changed(); // sent when multiple values have changed simulatenously (e.g. fromMap)

/**
 * Base valgrind settings
 */
public:
    QString valgrindExecutable() const;

public slots:
    void setValgrindExecutable(const QString &);

signals:
    void valgrindExecutableChanged(const QString &);

private:
    QString m_valgrindExecutable;


/**
 * Base memcheck settings
 */
public:
    int numCallers() const { return m_numCallers; }
    bool trackOrigins() const { return m_trackOrigins; }
    bool filterExternalIssues() const { return m_filterExternalIssues; }
    QList<int> visibleErrorKinds() const { return m_visibleErrorKinds; }

    virtual QStringList suppressionFiles() const = 0;
    virtual void addSuppressionFiles(const QStringList &) = 0;
    virtual void removeSuppressionFiles(const QStringList &) = 0;

public slots:
    void setNumCallers(int);
    void setTrackOrigins(bool);
    void setFilterExternalIssues(bool);
    void setVisibleErrorKinds(const QList<int> &);

signals:
    void numCallersChanged(int);
    void trackOriginsChanged(bool);
    void filterExternalIssuesChanged(bool);
    void visibleErrorKindsChanged(const QList<int> &);
    void suppressionFilesRemoved(const QStringList &);
    void suppressionFilesAdded(const QStringList &);

protected:
    int m_numCallers;
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
    ValgrindGlobalSettings() {}

    QWidget *createConfigWidget(QWidget *parent);
    QVariantMap toMap() const;
    QVariantMap defaults() const;
    void fromMap(const QVariantMap &map);
    virtual AbstractAnalyzerSubConfig *clone();

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
    QVariantMap toMap() const;
    QVariantMap defaults() const;
    void fromMap(const QVariantMap &map);
    virtual AbstractAnalyzerSubConfig *clone();

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

#endif // VALGRIND_INTERNAL_ANALZYZERSETTINGS_H
