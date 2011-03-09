/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef IANALYZERTOOL_H
#define IANALYZERTOOL_H

#include "analyzerbase_global.h"

#include <QtCore/QObject>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace ExtensionSystem {
class IPlugin;
}

namespace Analyzer {

class IAnalyzerEngine;

class ANALYZER_EXPORT IAnalyzerTool : public QObject
{
    Q_OBJECT
public:
    explicit IAnalyzerTool(QObject *parent = 0);

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    /**
     * The mode in which this tool should be run preferrably
     *
     * e.g. memcheck requires debug symbols, hence DebugMode is prefferred.
     * otoh callgrind should look at optimized code, hence ReleaseMode.
     */
    enum ToolMode {
        DebugMode,
        ReleaseMode,
        AnyMode
    };
    virtual ToolMode mode() const = 0;

    QString modeString();

    virtual void initialize(ExtensionSystem::IPlugin *plugin) = 0;

    virtual IAnalyzerEngine *createEngine(ProjectExplorer::RunConfiguration *runConfiguration) = 0;
};

} // namespace Analyzer

#endif // IANALYZERTOOL_H
