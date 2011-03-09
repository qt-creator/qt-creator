/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ANALYZER_INTERNAL_VALGRINDSETTINGS_H
#define ANALYZER_INTERNAL_VALGRINDSETTINGS_H

#include <analyzerbase/analyzersettings.h>

#include "valgrindtoolbase_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace Analyzer {
namespace Internal {

/**
 * Generic Valgrind settings shared by all tools.
 */
class VALGRINDTOOLBASE_EXPORT ValgrindSettings : public AbstractAnalyzerSubConfig
{
    Q_OBJECT
public:
    ValgrindSettings(QObject *parent);
    virtual ~ValgrindSettings();

    virtual QVariantMap toMap() const;
    virtual QVariantMap defaults() const;

    QString valgrindExecutable() const;

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QWidget *createConfigWidget(QWidget *parent);

public slots:
    void setValgrindExecutable(const QString &);

signals:
    void valgrindExecutableChanged(const QString &);

protected:
    virtual bool fromMap(const QVariantMap &map);

private:
    QString m_valgrindExecutable;
};

}
}

#endif // VALGRIND_INTERNAL_ANALZYZERSETTINGS_H
