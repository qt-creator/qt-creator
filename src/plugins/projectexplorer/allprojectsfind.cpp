/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "allprojectsfind.h"

#include "project.h"
#include "session.h"
#include "projectexplorer.h"
#include "editorconfiguration.h"

#include <texteditor/itexteditor.h>
#include <utils/filesearch.h>

#include <QSettings>
#include <QRegExp>

#include <QGridLayout>
#include <QLabel>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

AllProjectsFind::AllProjectsFind(ProjectExplorerPlugin *plugin)
    : m_plugin(plugin),
      m_configWidget(0)
{
    connect(m_plugin, SIGNAL(fileListChanged()), this, SLOT(handleFileListChanged()));
}

QString AllProjectsFind::id() const
{
    return QLatin1String("All Projects");
}

QString AllProjectsFind::displayName() const
{
    return tr("All Projects");
}

bool AllProjectsFind::isEnabled() const
{
    return BaseFileFind::isEnabled()
            && m_plugin->session()->projects().count() > 0;
}

Utils::FileIterator *AllProjectsFind::files(const QStringList &nameFilters,
                                            const QVariant &additionalParameters) const
{
    Q_UNUSED(additionalParameters)
    return filesForProjects(nameFilters, m_plugin->session()->projects());
}

Utils::FileIterator *AllProjectsFind::filesForProjects(const QStringList &nameFilters,
                                            const QList<Project *> &projects) const
{
    QList<QRegExp> filterRegs;
    foreach (const QString &filter, nameFilters) {
        filterRegs << QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QMap<QString, QTextCodec *> openEditorEncodings = TextEditor::ITextEditor::openedTextEditorsEncodings();
    QMap<QString, QTextCodec *> encodings;
    foreach (const Project *project, projects) {
        QStringList projectFiles = project->files(Project::AllFiles);
        QStringList filteredFiles;
        if (!filterRegs.isEmpty()) {
            foreach (const QString &file, projectFiles) {
                foreach (QRegExp reg, filterRegs) {
                    if (reg.exactMatch(file) || reg.exactMatch(QFileInfo(file).fileName())) {
                        filteredFiles.append(file);
                        break;
                    }
                }
            }
        } else {
            filteredFiles = projectFiles;
        }
        foreach (const QString &fileName, filteredFiles) {
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = project->editorConfiguration()->textCodec();
            encodings.insert(fileName, codec);
        }
    }
    return new Utils::FileIterator(encodings.keys(), encodings.values());
}

QVariant AllProjectsFind::additionalParameters() const
{
    return QVariant();
}

QString AllProjectsFind::label() const
{
    return tr("All Projects:");
}

QString AllProjectsFind::toolTip() const
{
    // %2 is filled by BaseFileFind::runNewSearch
    return tr("Filter: %1\n%2").arg(fileNameFilters().join(QLatin1String(",")));
}

void AllProjectsFind::handleFileListChanged()
{
    emit enabledChanged(isEnabled());
}

QWidget *AllProjectsFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        QLabel * const filePatternLabel = new QLabel(tr("Fi&le pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        gridLayout->addWidget(filePatternLabel, 0, 0, Qt::AlignRight);
        gridLayout->addWidget(patternWidget, 0, 1);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void AllProjectsFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("AllProjectsFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void AllProjectsFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("AllProjectsFind"));
    readCommonSettings(settings, QString(QLatin1Char('*')));
    settings->endGroup();
}
