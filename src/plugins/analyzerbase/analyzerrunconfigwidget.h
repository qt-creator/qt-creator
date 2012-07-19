/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef ANALYZER_INTERNAL_ANALYZERRUNCONFIGWIDGET_H
#define ANALYZER_INTERNAL_ANALYZERRUNCONFIGWIDGET_H

#include "analyzersettings.h"

#include <projectexplorer/runconfiguration.h>

#include <utils/detailswidget.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
}

namespace Analyzer {

class AnalyzerSettings;
class AbstractAnalyzerSubConfig;

namespace Internal {

class AnalyzerToolDetailWidget : public Utils::DetailsWidget
{
    Q_OBJECT

public:
    explicit AnalyzerToolDetailWidget(AbstractAnalyzerSubConfig *config, QWidget *parent=0);
};

class AnalyzerRunConfigWidget : public ProjectExplorer::RunConfigWidget
{
    Q_OBJECT

public:
    AnalyzerRunConfigWidget();

    virtual QString displayName() const;

    void setRunConfiguration(ProjectExplorer::RunConfiguration *rc);

private:
    void setDetailEnabled(bool value);

private slots:
    void chooseSettings(int setting);
    void restoreGlobal();

private:
    QWidget *m_subConfigWidget;
    AnalyzerRunConfigurationAspect *m_aspect;
    QComboBox *m_settingsCombo;
    QPushButton *m_restoreButton;
};

} // namespace Internal
} // namespace Analyzer

#endif // ANALYZER_INTERNAL_ANALYZERRUNCONFIGWIDGET_H
