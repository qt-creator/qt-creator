// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "artisticstylesettings.h"

#include "artisticstyleconstants.h"
#include "../beautifierconstants.h"
#include "../beautifierplugin.h"
#include "../beautifiertr.h"
#include "../configurationpanel.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QRegularExpression>
#include <QXmlStreamWriter>

using namespace Utils;

namespace Beautifier::Internal {

const char SETTINGS_NAME[]            = "artisticstyle";

ArtisticStyleSettings::ArtisticStyleSettings()
    : AbstractSettings(SETTINGS_NAME, ".astyle")
{
    setVersionRegExp(QRegularExpression("([2-9]{1})\\.([0-9]{1,2})(\\.[1-9]{1})?$"));
    command.setLabelText(Tr::tr("Artistic Style command:"));
    command.setDefaultValue("astyle");
    command.setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle(
                                     Tr::tr(Constants::ARTISTICSTYLE_DISPLAY_NAME)));

    registerAspect(&useOtherFiles);
    useOtherFiles.setSettingsKey("useOtherFiles");
    useOtherFiles.setLabelText(Tr::tr("Use file *.astylerc defined in project files"));
    useOtherFiles.setDefaultValue(true);

    registerAspect(&useSpecificConfigFile);
    useSpecificConfigFile.setSettingsKey("useSpecificConfigFile");
    useSpecificConfigFile.setLabelText(Tr::tr("Use specific config file:"));

    registerAspect(&specificConfigFile);
    specificConfigFile.setSettingsKey("specificConfigFile");
    specificConfigFile.setExpectedKind(PathChooser::File);
    specificConfigFile.setPromptDialogFilter(Tr::tr("AStyle (*.astylerc)"));

    registerAspect(&useHomeFile);
    useHomeFile.setSettingsKey("useHomeFile");
    useHomeFile.setLabelText(Tr::tr("Use file .astylerc or astylerc in HOME").
               replace("HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));

    registerAspect(&useCustomStyle);
    useCustomStyle.setSettingsKey("useCustomStyle");
    useCustomStyle.setLabelText(Tr::tr("Use customized style:"));

    registerAspect(&customStyle);
    customStyle.setSettingsKey("customStyle");

    documentationFilePath.setFilePath(
        Core::ICore::userResourcePath(Beautifier::Constants::SETTINGS_DIRNAME)
            .pathAppended(Beautifier::Constants::DOCUMENTATION_DIRNAME)
            .pathAppended(SETTINGS_NAME)
            .stringAppended(".xml"));

    read();
}

void ArtisticStyleSettings::createDocumentationFile() const
{
    Process process;
    process.setTimeoutS(2);
    process.setCommand({command(), {"-h"}});
    process.runBlocking();
    if (process.result() != ProcessResult::FinishedWithSuccess)
        return;

    QFile file(documentationFilePath().toFSPathString());
    const QFileInfo fi(file);
    if (!fi.exists())
        fi.dir().mkpath(fi.absolutePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    bool contextWritten = false;
    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument("1.0", true);
    stream.writeComment("Created " + QDateTime::currentDateTime().toString(Qt::ISODate));
    stream.writeStartElement(Constants::DOCUMENTATION_XMLROOT);

    // astyle writes its output to 'error'...
    const QStringList lines = process.cleanedStdErr().split(QLatin1Char('\n'));
    QStringList keys;
    QStringList docu;
    for (QString line : lines) {
        line = line.trimmed();
        if ((line.startsWith("--") && !line.startsWith("---")) || line.startsWith("OR ")) {
            const QStringList rawKeys = line.split(" OR ", Qt::SkipEmptyParts);
            for (QString k : rawKeys) {
                k = k.trimmed();
                k.remove('#');
                keys << k;
                if (k.startsWith("--"))
                    keys << k.right(k.size() - 2);
            }
        } else {
            if (line.isEmpty()) {
                if (!keys.isEmpty()) {
                    // Write entry
                    stream.writeStartElement(Constants::DOCUMENTATION_XMLENTRY);
                    stream.writeStartElement(Constants::DOCUMENTATION_XMLKEYS);
                    for (const QString &key : std::as_const(keys))
                        stream.writeTextElement(Constants::DOCUMENTATION_XMLKEY, key);
                    stream.writeEndElement();
                    const QString text = "<p><span class=\"option\">"
                            + keys.filter(QRegularExpression("^\\-")).join(", ") + "</span></p><p>"
                            + (docu.join(' ').toHtmlEscaped()) + "</p>";
                    stream.writeTextElement(Constants::DOCUMENTATION_XMLDOC, text);
                    stream.writeEndElement();
                    contextWritten = true;
                }
                keys.clear();
                docu.clear();
            } else if (!keys.isEmpty()) {
                docu << line;
            }
        }
    }

    stream.writeEndElement();
    stream.writeEndDocument();

    // An empty file causes error messages and a contextless file preventing this function to run
    // again in order to generate the documentation successfully. Thus delete the file.
    if (!contextWritten) {
        file.close();
        file.remove();
    }
}

class ArtisticStyleOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    explicit ArtisticStyleOptionsPageWidget(ArtisticStyleSettings *settings)
    {
        QGroupBox *options = nullptr;

        auto configurations = new ConfigurationPanel(this);
        configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        configurations->setSettings(settings);
        configurations->setCurrentConfiguration(settings->customStyle());

        using namespace Layouting;

        ArtisticStyleSettings &s = *settings;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Form {
                    s.command, br,
                    s.supportedMimeTypes
                }
            },
            Group {
                title(Tr::tr("Options")),
                bindTo(&options),
                Column {
                    s.useOtherFiles,
                    Row { s.useSpecificConfigFile, s.specificConfigFile },
                    s.useHomeFile,
                    Row { s.useCustomStyle, configurations },
                }
            },
            st
        }.attachTo(this);

        setOnApply([&s, configurations] {
            s.customStyle.setValue(configurations->currentConfiguration());
            s.save();
        });

        s.read();

        connect(s.command.pathChooser(), &PathChooser::validChanged, options, &QWidget::setEnabled);
        options->setEnabled(s.command.pathChooser()->isValid());
    }
};

ArtisticStyleOptionsPage::ArtisticStyleOptionsPage(ArtisticStyleSettings *settings)
{
    setId("ArtisticStyle");
    setDisplayName(Tr::tr("Artistic Style"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new ArtisticStyleOptionsPageWidget(settings); });
}

} // Beautifier::Internal
