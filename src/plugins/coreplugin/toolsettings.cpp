/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "toolsettings.h"

#include "externaltool.h"
#include "coreconstants.h"

#include <QtCore/QCoreApplication>

using namespace Core;
using namespace Core::Internal;

ToolSettings::ToolSettings(QObject *parent) :
    IOptionsPage(parent)
{
}

QString ToolSettings::id() const
{
    return QLatin1String("G.ExternalTools");
}


QString ToolSettings::displayName() const
{
    return tr("External Tools");
}


QString ToolSettings::category() const
{
    return QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE);
}


QString ToolSettings::displayCategory() const
{
    return QCoreApplication::translate("Core", Core::Constants::SETTINGS_TR_CATEGORY_CORE);
}


QIcon ToolSettings::categoryIcon() const
{
    return QIcon(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE_ICON));
}


bool ToolSettings::matches(const QString & searchKeyWord) const
{
    return m_searchKeywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *ToolSettings::createPage(QWidget *parent)
{
    m_widget = new ExternalToolConfig(parent);
    m_widget->setTools(ExternalToolManager::instance()->toolsByCategory());
    if (m_searchKeywords.isEmpty()) {
        m_searchKeywords = m_widget->searchKeywords();
    }
    return m_widget;
}


void ToolSettings::apply()
{
    if (!m_widget)
        return;
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
                // check if the tool has changed
                if ((*originalTool) == (*tool)) {
                    toolToAdd = originalTool;
                } else {
                    toolToAdd = new ExternalTool(tool);
                }
            }
            items.append(toolToAdd);
        }
        resultMap.insert(it.key(), items);
    }
    ExternalToolManager::instance()->setToolsByCategory(resultMap);
}


void ToolSettings::finish()
{
}
