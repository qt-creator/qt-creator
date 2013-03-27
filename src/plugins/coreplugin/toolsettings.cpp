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

#include "toolsettings.h"
#include "dialogs/externaltoolconfig.h"
#include "externaltool.h"
#include "externaltoolmanager.h"
#include "coreconstants.h"
#include "icore.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

#include <QDebug>

using namespace Core;
using namespace Core::Internal;

ToolSettings::ToolSettings(QObject *parent) :
    IOptionsPage(parent)
{
    setId(Core::Constants::SETTINGS_ID_TOOLS);
    setDisplayName(tr("External Tools"));
    setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE_ICON));
}


bool ToolSettings::matches(const QString & searchKeyWord) const
{
    return m_searchKeywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *ToolSettings::createPage(QWidget *parent)
{
    m_widget = new ExternalToolConfig(parent);
    m_widget->setTools(ExternalToolManager::instance()->toolsByCategory());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}


static QString getUserFilePath(const QString &proposalFileName)
{
    QDir resourceDir(ICore::userResourcePath());
    if (!resourceDir.exists(QLatin1String("externaltools")))
        resourceDir.mkpath(QLatin1String("externaltools"));
    QFileInfo fi(proposalFileName);
    const QString &suffix = QLatin1String(".") + fi.completeSuffix();
    const QString &newFilePath = ICore::userResourcePath()
            + QLatin1String("/externaltools/") + fi.baseName();
    int count = 0;
    QString tryPath = newFilePath + suffix;
    while (QFile::exists(tryPath)) {
        if (count > 15)
            return QString();
        // add random number
        int number = qrand() % 1000;
        tryPath = newFilePath + QString::number(number) + suffix;
    }
    return tryPath;
}

static QString idFromDisplayName(const QString &displayName)
{
    QString id = displayName;
    QChar *c = id.data();
    while (!c->isNull()) {
        if (!c->isLetterOrNumber())
            *c = QLatin1Char('_');
        ++c;
    }
    return id;
}

static QString findUnusedId(const QString &proposal, const QMap<QString, QList<ExternalTool *> > &tools)
{
    int number = 0;
    QString result;
    bool found = false;
    do {
        result = proposal + (number > 0 ? QString::number(number) : QString::fromLatin1(""));
        ++number;
        found = false;
        QMapIterator<QString, QList<ExternalTool *> > it(tools);
        while (!found && it.hasNext()) {
            it.next();
            foreach (ExternalTool *tool, it.value()) {
                if (tool->id() == result) {
                    found = true;
                    break;
                }
            }
        }
    } while (found);
    return result;
}

void ToolSettings::apply()
{
    if (!m_widget)
        return;
    m_widget->apply();
    QMap<QString, ExternalTool *> originalTools = ExternalToolManager::instance()->toolsById();
    QMap<QString, QList<ExternalTool *> > newToolsMap = m_widget->tools();
    QMap<QString, QList<ExternalTool *> > resultMap;
    QMapIterator<QString, QList<ExternalTool *> > it(newToolsMap);
    while (it.hasNext()) {
        it.next();
        QList<ExternalTool *> items;
        foreach (ExternalTool *tool, it.value()) {
            ExternalTool *toolToAdd = 0;
            if (ExternalTool *originalTool = originalTools.take(tool->id())) {
                // check if it has different category and is custom tool
                if (tool->displayCategory() != it.key() && !tool->preset())
                    tool->setDisplayCategory(it.key());
                // check if the tool has changed
                if ((*originalTool) == (*tool)) {
                    toolToAdd = originalTool;
                } else {
                    // case 1: tool is changed preset
                    if (tool->preset() && (*tool) != (*(tool->preset()))) {
                        // check if we need to choose a new file name
                        if (tool->preset()->fileName() == tool->fileName()) {
                            const QString &fileName = QFileInfo(tool->preset()->fileName()).fileName();
                            const QString &newFilePath = getUserFilePath(fileName);
                            // TODO error handling if newFilePath.isEmpty() (i.e. failed to find a unused name)
                            tool->setFileName(newFilePath);
                        }
                        // TODO error handling
                        tool->save();
                    // case 2: tool is previously changed preset but now same as preset
                    } else if (tool->preset() && (*tool) == (*(tool->preset()))) {
                        // check if we need to delete the changed description
                        if (originalTool->fileName() != tool->preset()->fileName()
                                && QFile::exists(originalTool->fileName())) {
                            // TODO error handling
                            QFile::remove(originalTool->fileName());
                        }
                        tool->setFileName(tool->preset()->fileName());
                        // no need to save, it's the same as the preset
                    // case 3: tool is custom tool
                    } else {
                        // TODO error handling
                        tool->save();
                    }

                     // 'tool' is deleted by config page, 'originalTool' is deleted by setToolsByCategory
                    toolToAdd = new ExternalTool(tool);
                }
            } else {
                // new tool. 'tool' is deleted by config page
                QString id = idFromDisplayName(tool->displayName());
                id = findUnusedId(id, newToolsMap);
                tool->setId(id);
                // TODO error handling if newFilePath.isEmpty() (i.e. failed to find a unused name)
                tool->setFileName(getUserFilePath(id + QLatin1String(".xml")));
                // TODO error handling
                tool->save();
                toolToAdd = new ExternalTool(tool);
            }
            items.append(toolToAdd);
        }
        if (!items.isEmpty())
            resultMap.insert(it.key(), items);
    }
    // Remove tools that have been deleted from the settings (and are no preset)
    foreach (ExternalTool *tool, originalTools) {
        QTC_ASSERT(!tool->preset(), continue);
        // TODO error handling
        QFile::remove(tool->fileName());
    }

    ExternalToolManager::instance()->setToolsByCategory(resultMap);
}


void ToolSettings::finish()
{
}
