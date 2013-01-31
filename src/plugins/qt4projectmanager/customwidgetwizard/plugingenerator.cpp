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

#include "plugingenerator.h"
#include "pluginoptions.h"

#include <coreplugin/basefilewizard.h>

#include <utils/fileutils.h>

#include <cpptools/abstracteditorsupport.h>

#include <QFileInfo>
#include <QDir>
#include <QSet>

static QString headerGuard(const QString &header)
{
    return header.toUpper().replace(QRegExp(QLatin1String("[^A-Z0-9]+")), QLatin1String("_"));
}

namespace Qt4ProjectManager {
namespace Internal {

struct ProjectContents {
    QString tmpl;
    QString library;
    QString headers;
    QString sources;
};

// Create a binary icon file
static inline Core::GeneratedFile  generateIconFile(const QString &source, const QString &target, QString *errorMessage)
{
    // Read out source
    Utils::FileReader reader;
    if (!reader.fetch(source, errorMessage))
        return Core::GeneratedFile();
    Core::GeneratedFile rc(target);
    rc.setBinaryContents(reader.data());
    rc.setBinary(true);
    return rc;
}

QList<Core::GeneratedFile>  PluginGenerator::generatePlugin(const GenerationParameters& p, const PluginOptions &options,
                                                            QString *errorMessage)
{
    const QChar slash = QLatin1Char('/');
    const QChar blank = QLatin1Char(' ');
    QList<Core::GeneratedFile> rc;

    QString baseDir = p.path;
    baseDir += slash;
    baseDir += p.fileName;
    const QString slashLessBaseDir = baseDir;
    baseDir += slash;

    QSet<QString> widgetLibraries;
    QSet<QString> widgetProjects;
    QMap<QString,ProjectContents> widgetProjectContents;
    QString pluginIncludes;
    QString pluginAdditions;
    QString pluginHeaders;
    QString pluginSources;
    QSet<QString> pluginIcons;

    SubstitutionMap sm;

    // First create the widget wrappers (plugins) and - if requested - skeletons
    // for the widgets.
    const int widgetCount = options.widgetOptions.size();
    for (int i = 0; i < widgetCount; i++) {
        const PluginOptions::WidgetOptions &wo = options.widgetOptions.at(i);
        sm.clear();
        sm.insert(QLatin1String("SINGLE_INCLUDE_GUARD"), headerGuard(wo.pluginHeaderFile));
        sm.insert(QLatin1String("PLUGIN_CLASS"), wo.pluginClassName);
        const QString pluginHeaderContents = processTemplate(p.templatePath + QLatin1String("/tpl_single.h"), sm, errorMessage);
        if (pluginHeaderContents.isEmpty())
            return QList<Core::GeneratedFile>();
        Core::GeneratedFile pluginHeader(baseDir + wo.pluginHeaderFile);
        pluginHeader.setContents(CppTools::AbstractEditorSupport::licenseTemplate(wo.pluginHeaderFile, wo.pluginClassName)
                                 + pluginHeaderContents);
        rc.push_back(pluginHeader);

        sm.remove(QLatin1String("SINGLE_INCLUDE_GUARD"));
        sm.insert(QLatin1String("PLUGIN_HEADER"), wo.pluginHeaderFile);
        sm.insert(QLatin1String("WIDGET_CLASS"), wo.widgetClassName);
        sm.insert(QLatin1String("WIDGET_HEADER"), wo.widgetHeaderFile);
        sm.insert(QLatin1String("WIDGET_GROUP"), wo.group);
        QString iconResource;
        if (!wo.iconFile.isEmpty()) {
            iconResource = QLatin1String("QLatin1String(\":/");
            iconResource += QFileInfo(wo.iconFile).fileName();
            iconResource += QLatin1String("\")");
        }
        sm.insert(QLatin1String("WIDGET_ICON"),iconResource);
        sm.insert(QLatin1String("WIDGET_TOOLTIP"), cStringQuote(wo.toolTip));
        sm.insert(QLatin1String("WIDGET_WHATSTHIS"), cStringQuote(wo.whatsThis));
        sm.insert(QLatin1String("WIDGET_ISCONTAINER"), wo.isContainer ? QLatin1String("true") : QLatin1String("false"));
        sm.insert(QLatin1String("WIDGET_DOMXML"), cStringQuote(wo.domXml));
        sm.insert(QLatin1String("SINGLE_PLUGIN_EXPORT"),
            options.widgetOptions.count() == 1 ?
                QLatin1String("\nQ_EXPORT_PLUGIN2(") +
                    options.pluginName +
                    QLatin1String(", ") +
                    wo.pluginClassName +
                    QLatin1Char(')') :
                QString());
        const QString pluginSourceContents = processTemplate(p.templatePath + QLatin1String("/tpl_single.cpp"), sm, errorMessage);
        if (pluginSourceContents.isEmpty())
            return QList<Core::GeneratedFile>();
        Core::GeneratedFile pluginSource(baseDir + wo.pluginSourceFile);
        pluginSource.setContents(CppTools::AbstractEditorSupport::licenseTemplate(wo.pluginSourceFile, wo.pluginClassName)
                                 + pluginSourceContents);
        if (i == 0 && widgetCount == 1) // Open first widget unless collection
            pluginSource.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
        rc.push_back(pluginSource);

        if (wo.sourceType == PluginOptions::WidgetOptions::LinkLibrary)
            widgetLibraries.insert(QLatin1String("-l") + wo.widgetLibrary);
        else
            widgetProjects.insert(QLatin1String("include(") + wo.widgetProjectFile + QLatin1String(")"));
        pluginIncludes += QLatin1String("#include \"") + wo.pluginHeaderFile + QLatin1String("\"\n");
        pluginAdditions +=
            QLatin1String("    m_widgets.append(new ") + wo.pluginClassName + QLatin1String("(this));\n");
        pluginHeaders += QLatin1Char(' ') + wo.pluginHeaderFile;
        pluginSources += QLatin1Char(' ') + wo.pluginSourceFile;
        if (!wo.iconFile.isEmpty())
            pluginIcons.insert(wo.iconFile);

        if (wo.createSkeleton) {
            ProjectContents &pc = widgetProjectContents[wo.widgetProjectFile];
            if (pc.headers.isEmpty()) {
                if (wo.sourceType == PluginOptions::WidgetOptions::LinkLibrary) {
                    pc.library = wo.widgetLibrary;
                    pc.tmpl = p.templatePath + QLatin1String("/tpl_widget_lib.pro");
                } else {
                    pc.tmpl = p.templatePath + QLatin1String("/tpl_widget_include.pri");
                }
                widgetProjectContents.insert(wo.widgetProjectFile, pc);
            } else {
                if (pc.library != wo.widgetLibrary) {
                    *errorMessage = tr("Creating multiple widget libraries (%1, %2) in one project (%3) is not supported.")
                        .arg(pc.library, wo.widgetLibrary, wo.widgetProjectFile);
                    return QList<Core::GeneratedFile>();
                }
            }
            pc.headers += blank + wo.widgetHeaderFile;
            pc.sources += blank + wo.widgetSourceFile;

            sm.clear();
            sm.insert(QLatin1String("WIDGET_INCLUDE_GUARD"), headerGuard(wo.widgetHeaderFile));
            sm.insert(QLatin1String("WIDGET_BASE_CLASS"), wo.widgetBaseClassName);
            sm.insert(QLatin1String("WIDGET_CLASS"), wo.widgetClassName);
            const QString widgetHeaderContents = processTemplate(p.templatePath + QLatin1String("/tpl_widget.h"), sm, errorMessage);
            if (widgetHeaderContents.isEmpty())
                return QList<Core::GeneratedFile>();
            Core::GeneratedFile widgetHeader(baseDir + wo.widgetHeaderFile);
            widgetHeader.setContents(CppTools::AbstractEditorSupport::licenseTemplate(wo.widgetHeaderFile, wo.widgetClassName)
                                     + widgetHeaderContents);
            rc.push_back(widgetHeader);

            sm.remove(QLatin1String("WIDGET_INCLUDE_GUARD"));
            sm.insert(QLatin1String("WIDGET_HEADER"), wo.widgetHeaderFile);
            const QString widgetSourceContents = processTemplate(p.templatePath + QLatin1String("/tpl_widget.cpp"), sm, errorMessage);
            if (widgetSourceContents.isEmpty())
                return QList<Core::GeneratedFile>();
            Core::GeneratedFile widgetSource(baseDir + wo.widgetSourceFile);
            widgetSource.setContents(CppTools::AbstractEditorSupport::licenseTemplate(wo.widgetSourceFile, wo.widgetClassName)
                                     + widgetSourceContents);
            rc.push_back(widgetSource);
        }
    }

    // Then create the project files for the widget skeletons.
    // These might create widgetLibraries or be included into the plugin's project.
    QMap<QString,ProjectContents>::const_iterator it = widgetProjectContents.constBegin();
    const QMap<QString,ProjectContents>::const_iterator end = widgetProjectContents.constEnd();
    for (; it != end; ++it) {
        const ProjectContents &pc = it.value();
        sm.clear();
        sm.insert(QLatin1String("WIDGET_HEADERS"), pc.headers);
        sm.insert(QLatin1String("WIDGET_SOURCES"), pc.sources);
        if (!pc.library.isEmpty())
            sm.insert(QLatin1String("WIDGET_LIBRARY"), pc.library);
        const QString widgetPriContents = processTemplate(pc.tmpl, sm, errorMessage);
        if (widgetPriContents.isEmpty())
            return QList<Core::GeneratedFile>();
        Core::GeneratedFile widgetPri(baseDir + it.key());
        widgetPri.setContents(widgetPriContents);
        rc.push_back(widgetPri);
    }

    // Create the sources for the collection if necessary.
    if (widgetCount > 1) {
        sm.clear();
        sm.insert(QLatin1String("COLLECTION_INCLUDE_GUARD"), headerGuard(options.collectionHeaderFile));
        sm.insert(QLatin1String("COLLECTION_PLUGIN_CLASS"), options.collectionClassName);
        const QString collectionHeaderContents = processTemplate(p.templatePath + QLatin1String("/tpl_collection.h"), sm, errorMessage);
        if (collectionHeaderContents.isEmpty())
            return QList<Core::GeneratedFile>();
        Core::GeneratedFile collectionHeader(baseDir + options.collectionHeaderFile);
        collectionHeader.setContents(CppTools::AbstractEditorSupport::licenseTemplate(options.collectionHeaderFile, options.collectionClassName)
                                     + collectionHeaderContents);
        rc.push_back(collectionHeader);

        sm.remove(QLatin1String("COLLECTION_INCLUDE_GUARD"));
        sm.insert(QLatin1String("PLUGIN_INCLUDES"),
            pluginIncludes +
            QLatin1String("#include \"") +
                options.collectionHeaderFile +
                QLatin1String("\""));
        sm.insert(QLatin1String("PLUGIN_ADDITIONS"), pluginAdditions);
        sm.insert(QLatin1String("COLLECTION_PLUGIN_EXPORT"),
            QLatin1String("Q_EXPORT_PLUGIN2(") +
                options.pluginName +
                QLatin1String(", ") +
                options.collectionClassName +
                QLatin1Char(')'));
        const QString collectionSourceFileContents = processTemplate(p.templatePath + QLatin1String("/tpl_collection.cpp"), sm, errorMessage);
        if (collectionSourceFileContents.isEmpty())
            return QList<Core::GeneratedFile>();
        Core::GeneratedFile collectionSource(baseDir + options.collectionSourceFile);
        collectionSource.setContents(CppTools::AbstractEditorSupport::licenseTemplate(options.collectionSourceFile, options.collectionClassName)
                                     + collectionSourceFileContents);
        collectionSource.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
        rc.push_back(collectionSource);

        pluginHeaders += blank + options.collectionHeaderFile;
        pluginSources += blank + options.collectionSourceFile;
    }

    // Copy icons that are not in the plugin source base directory yet (that is,
    // probably all), add them to the resource file
    QString iconFiles;
    foreach (QString icon, pluginIcons) {
        const QFileInfo qfi(icon);
        if (qfi.dir() != slashLessBaseDir) {
            const QString newIcon = baseDir + qfi.fileName();
            const Core::GeneratedFile iconFile = generateIconFile(icon, newIcon, errorMessage);
            if (iconFile.path().isEmpty())
                return QList<Core::GeneratedFile>();
            rc.push_back(iconFile);
            icon = qfi.fileName();
        }
        iconFiles += QLatin1String("        <file>") + icon + QLatin1String("</file>\n");
    }
    // Create the resource file with the icons.
    sm.clear();
    sm.insert(QLatin1String("ICON_FILES"), iconFiles);
    const QString resourceFileContents = processTemplate(p.templatePath + QLatin1String("/tpl_resources.qrc"), sm, errorMessage);
    if (resourceFileContents.isEmpty())
        return QList<Core::GeneratedFile>();
    Core::GeneratedFile resourceFile(baseDir + options.resourceFile);
    resourceFile.setContents(resourceFileContents);
    rc.push_back(resourceFile);

    // Finally create the project for the plugin itself.
    sm.clear();
    sm.insert(QLatin1String("PLUGIN_NAME"), options.pluginName);
    sm.insert(QLatin1String("PLUGIN_HEADERS"), pluginHeaders);
    sm.insert(QLatin1String("PLUGIN_SOURCES"), pluginSources);
    sm.insert(QLatin1String("PLUGIN_RESOURCES"), options.resourceFile);
    sm.insert(QLatin1String("WIDGET_LIBS"), QStringList(widgetLibraries.toList()).join(QString(blank)));
    sm.insert(QLatin1String("INCLUSIONS"), QStringList(widgetProjects.toList()).join(QLatin1String("\n")));
    const QString proFileContents = processTemplate(p.templatePath + QLatin1String("/tpl_plugin.pro"), sm, errorMessage);
    if (proFileContents.isEmpty())
        return QList<Core::GeneratedFile>();
    Core::GeneratedFile proFile(baseDir + p.fileName + QLatin1String(".pro"));
    proFile.setContents(proFileContents);
    proFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    rc.push_back(proFile);
    return rc;
}

QString PluginGenerator::processTemplate(const QString &tmpl,
                                         const SubstitutionMap &substMap,
                                         QString *errorMessage)
{
    Utils::FileReader reader;
    if (!reader.fetch(tmpl, errorMessage))
        return QString();

    QString cont = QString::fromUtf8(reader.data());
    const QChar atChar = QLatin1Char('@');
    int offset = 0;
    for (;;) {
        const int start = cont.indexOf(atChar, offset);
        if (start < 0)
            break;
        const int end = cont.indexOf(atChar, start + 1);
        Q_ASSERT(end);
        const QString keyword = cont.mid(start + 1, end - start - 1);
        const QString replacement = substMap.value(keyword);
        cont.replace(start, end - start + 1, replacement);
        offset = start + replacement.length();
    }
    return cont;
}

QString PluginGenerator::cStringQuote(QString s)
{
    s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    s.replace(QLatin1Char('"'), QLatin1String("\\\""));
    s.replace(QLatin1Char('\t'), QLatin1String("\\t"));
    s.replace(QLatin1Char('\n'), QLatin1String("\\n"));
    return s;
}

}
}
