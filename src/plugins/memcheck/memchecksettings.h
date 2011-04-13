/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ANALYZER_INTERNAL_MEMCHECKSETTINGS_H
#define ANALYZER_INTERNAL_MEMCHECKSETTINGS_H

#include <analyzerbase/analyzersettings.h>

namespace Analyzer {
namespace Internal {

/**
 * Generic memcheck settings
 */
class AbstractMemcheckSettings : public AbstractAnalyzerSubConfig
{
    Q_OBJECT
public:
    AbstractMemcheckSettings(QObject *parent);
    virtual ~AbstractMemcheckSettings();

    virtual bool fromMap(const QVariantMap &map);

    int numCallers() const { return m_numCallers; }
    bool trackOrigins() const { return m_trackOrigins; }
    bool filterExternalIssues() const { return m_filterExternalIssues; }
    QList<int> visibleErrorKinds() const { return m_visibleErrorKinds; }

    virtual QStringList suppressionFiles() const = 0;
    virtual void addSuppressionFiles(const QStringList &) = 0;
    virtual void removeSuppressionFiles(const QStringList &) = 0;

    virtual QVariantMap defaults() const;

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QWidget *createConfigWidget(QWidget *parent);

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
    virtual QVariantMap toMap() const;

    int m_numCallers;
    bool m_trackOrigins;
    bool m_filterExternalIssues;
    QList<int> m_visibleErrorKinds;
};

/**
 * Global memcheck settings
 */
class MemcheckGlobalSettings : public AbstractMemcheckSettings
{
    Q_OBJECT
public:
    MemcheckGlobalSettings(QObject *parent);
    virtual ~MemcheckGlobalSettings();

    QStringList suppressionFiles() const;
    // in the global settings we change the internal list directly
    void addSuppressionFiles(const QStringList &);
    void removeSuppressionFiles(const QStringList &);

    QVariantMap toMap() const;
    QVariantMap defaults() const;

    // internal settings which don't require any UI
    void setLastSuppressionDialogDirectory(const QString &directory);
    QString lastSuppressionDialogDirectory() const;

    void setLastSuppressionDialogHistory(const QStringList &history);
    QStringList lastSuppressionDialogHistory() const;

protected:
    bool fromMap(const QVariantMap &map);

private:
    QStringList m_suppressionFiles;
    QString m_lastSuppressionDirectory;
    QStringList m_lastSuppressionHistory;
};

/**
 * Per-project memcheck settings, saves a diff to the global suppression files list
 */
class MemcheckProjectSettings : public AbstractMemcheckSettings
{
    Q_OBJECT
public:
    MemcheckProjectSettings(QObject *parent);
    virtual ~MemcheckProjectSettings();

    QStringList suppressionFiles() const;
    // in the project-specific settings we store a diff to the global list
    void addSuppressionFiles(const QStringList &suppressions);
    void removeSuppressionFiles(const QStringList &suppressions);

    QVariantMap toMap() const;
    QVariantMap defaults() const;

protected:
    bool fromMap(const QVariantMap &map);

private:
    QStringList m_disabledGlobalSuppressionFiles;
    QStringList m_addedSuppressionFiles;
};

}
}

#endif // ANALYZER_INTERNAL_MEMCHECKSETTINGS_H
