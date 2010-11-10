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

#include "externaltool.h"

#include <QtCore/QXmlStreamReader>

#include <QtDebug>

using namespace Core::Internal;

namespace {
    const char * const kExternalTool = "externaltool";
    const char * const kDescription = "description";
    const char * const kDisplayName = "displayname";
    const char * const kCategory = "category";
    const char * const kOrder = "order";
    const char * const kExecutable = "executable";
    const char * const kPath = "path";
    const char * const kArguments = "arguments";
    const char * const kWorkingDirectory = "workingdirectory";

    const char * const kXmlLang = "xml:lang";
    const char * const kOutput = "output";
    const char * const kOutputShowInPane = "showinpane";
    const char * const kOutputReplaceSelection = "replaceselection";
    const char * const kOutputReloadDocument = "reloaddocument";
}

ExternalTool::ExternalTool() :
    m_order(-1),
    m_outputHandling(ShowInPane)
{
}

QString ExternalTool::description() const
{
    return m_description;
}

QString ExternalTool::displayName() const
{
    return m_displayName;
}

QString ExternalTool::displayCategory() const
{
    return m_displayCategory;
}

int ExternalTool::order() const
{
    return m_order;
}

QStringList ExternalTool::executables() const
{
    return m_executables;
}

QString ExternalTool::arguments() const
{
    return m_arguments;
}

QString ExternalTool::workingDirectory() const
{
    return m_workingDirectory;
}

ExternalTool::OutputHandling ExternalTool::outputHandling() const
{
    return m_outputHandling;
}

ExternalTool * ExternalTool::createFromXml(const QString &xml, QString *errorMessage)
{
    ExternalTool *tool = new ExternalTool;
    QXmlStreamReader reader(xml);
    if (!reader.readNextStartElement() || reader.name() != QLatin1String(kExternalTool))
        reader.raiseError(QLatin1String("Missing start element <externaltool>"));
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String(kDescription)) {
            // TODO locale check
            if (!reader.attributes().hasAttribute(QLatin1String(kXmlLang))
                    && tool->m_description.isEmpty()) {
                tool->m_description = reader.readElementText();
            } else {
                reader.skipCurrentElement();
            }
        } else if (reader.name() == QLatin1String(kDisplayName)) {
            // TODO locale check
            if (!reader.attributes().hasAttribute(QLatin1String(kXmlLang))
                    && tool->m_displayName.isEmpty()) {
                tool->m_displayName = reader.readElementText();
            } else {
                reader.skipCurrentElement();
            }
        } else if (reader.name() == QLatin1String(kCategory)) {
            // TODO locale check
            if (!reader.attributes().hasAttribute(QLatin1String(kXmlLang))
                    && tool->m_displayCategory.isEmpty()) {
                tool->m_displayCategory = reader.readElementText();
            } else {
                reader.skipCurrentElement();
            }
        } else if (reader.name() == QLatin1String(kOrder)) {
            if (tool->m_order >= 0) {
                reader.raiseError(QLatin1String("only one <order> element allowed"));
                break;
            }
            bool ok;
            tool->m_order = reader.readElementText().toInt(&ok);
            if (!ok || tool->m_order < 0)
                reader.raiseError(QLatin1String("<order> element requires non-negative integer value"));
        } else if (reader.name() == QLatin1String(kExecutable)) {
            if (reader.attributes().hasAttribute(QLatin1String(kOutput))) {
                const QString output = reader.attributes().value(QLatin1String(kOutput)).toString();
                if (output == QLatin1String(kOutputShowInPane)) {
                    tool->m_outputHandling = ExternalTool::ShowInPane;
                } else if (output == QLatin1String(kOutputReplaceSelection)) {
                    tool->m_outputHandling = ExternalTool::ReplaceSelection;
                } else if (output == QLatin1String(kOutputReloadDocument)) {
                    tool->m_outputHandling = ExternalTool::ReloadDocument;
                } else {
                    reader.raiseError(QLatin1String("Allowed values for output attribute are 'showinpane','replaceselection','reloaddocument'"));
                    break;
                }
            }
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String(kPath)) {
                    tool->m_executables.append(reader.readElementText());
                } else if (reader.name() == QLatin1String(kArguments)) {
                    if (!tool->m_arguments.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <arguments> element allowed"));
                        break;
                    }
                    tool->m_arguments = reader.readElementText();
                } else if (reader.name() == QLatin1String(kWorkingDirectory)) {
                    if (!tool->m_workingDirectory.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <workingdirectory> element allowed"));
                        break;
                    }
                    tool->m_workingDirectory = reader.readElementText();
                }
            }
        } else {
            reader.raiseError(QString::fromLatin1("Unknown element <%1>").arg(reader.qualifiedName().toString()));
        }
    }
    if (reader.hasError()) {
        if (errorMessage)
            *errorMessage = reader.errorString();
        qDebug() << reader.errorString();
        delete tool;
        return 0;
    }
    return tool;
}
