/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ANALYZER_INTERNAL_CALLGRINDSETTINGS_H
#define ANALYZER_INTERNAL_CALLGRINDSETTINGS_H

#include <analyzerbase/analyzersettings.h>

#include <QString>
#include "callgrindcostdelegate.h"

namespace Callgrind {
namespace Internal {

/**
 * Generic callgrind settings
 */
class AbstractCallgrindSettings : public Analyzer::AbstractAnalyzerSubConfig
{
    Q_OBJECT

public:
    AbstractCallgrindSettings(QObject *parent = 0);
    virtual ~AbstractCallgrindSettings();

    inline bool enableCacheSim() const { return m_enableCacheSim; }
    inline bool enableBranchSim() const { return m_enableBranchSim; }
    inline bool collectSystime() const { return m_collectSystime; }
    inline bool collectBusEvents() const { return m_collectBusEvents; }

    // abstract virtual methods from base class
    virtual bool fromMap(const QVariantMap &map);

    virtual QVariantMap defaults() const;

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QWidget *createConfigWidget(QWidget *parent);

public Q_SLOTS:
    void setEnableCacheSim(bool enable);
    void setEnableBranchSim(bool enable);
    void setCollectSystime(bool collect);
    void setCollectBusEvents(bool collect);

Q_SIGNALS:
    void enableCacheSimChanged(bool);
    void enableBranchSimChanged(bool);
    void collectSystimeChanged(bool);
    void collectBusEventsChanged(bool);

protected:
    virtual QVariantMap toMap() const;

private:
    bool m_enableCacheSim;
    bool m_collectSystime;
    bool m_collectBusEvents;
    bool m_enableBranchSim;
};

/**
 * Global callgrind settings
 */
class CallgrindGlobalSettings : public AbstractCallgrindSettings
{
    Q_OBJECT

public:
    CallgrindGlobalSettings(QObject *parent = 0);
    virtual ~CallgrindGlobalSettings();

    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap defaults() const;

    CostDelegate::CostFormat costFormat() const;
    bool detectCycles() const;
    double minimumInclusiveCostRatio() const;

public slots:
    void setCostFormat(Callgrind::Internal::CostDelegate::CostFormat format);
    void setDetectCycles(bool detect);
    void setMinimumInclusiveCostRatio(double minimumInclusiveCost);

protected:
    virtual QVariantMap toMap() const;

private:
    CostDelegate::CostFormat m_costFormat;
    bool m_detectCycles;
    double m_minimumInclusiveCostRatio;
};

/**
 * Per-project callgrind settings, saves a diff to the global suppression files list
 */
class CallgrindProjectSettings : public AbstractCallgrindSettings
{
    Q_OBJECT

public:
    CallgrindProjectSettings(QObject *parent = 0);
    virtual ~CallgrindProjectSettings();
};

}
}

#endif // ANALYZER_INTERNAL_CALLGRINDSETTINGS_H
