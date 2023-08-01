// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "beautifiertool.h"

#include "beautifierconstants.h"
#include "beautifiertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/genericconstants.h>
#include <utils/mimeutils.h>
#include <utils/process.h>

#include <QFile>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QXmlStreamReader>

using namespace Utils;

namespace Beautifier::Internal {

static QList<BeautifierTool *> &theTools()
{
    static QList<BeautifierTool *> tools;
    return tools;
}

BeautifierTool::BeautifierTool()
{
    theTools().append(this);
}

const QList<BeautifierTool *> &BeautifierTool::allTools()
{
    return theTools();
}

void BeautifierTool::showError(const QString &error)
{
    Core::MessageManager::writeFlashing(Tr::tr("Error in Beautifier: %1").arg(error.trimmed()));
}

QString BeautifierTool::msgCannotGetConfigurationFile(const QString &command)
{
    return Tr::tr("Cannot get configuration file for %1.").arg(command);
}

QString BeautifierTool::msgFormatCurrentFile()
{
    //: Menu entry
    return Tr::tr("Format &Current File");
}

QString BeautifierTool::msgFormatSelectedText()
{
    //: Menu entry
    return Tr::tr("Format &Selected Text");
}

QString BeautifierTool::msgFormatAtCursor()
{
    //: Menu entry
    return Tr::tr("&Format at Cursor");
}

QString BeautifierTool::msgFormatLines()
{
    //: Menu entry
    return Tr::tr("Format &Line(s)");
}

QString BeautifierTool::msgDisableFormattingSelectedText()
{
    //: Menu entry
    return Tr::tr("&Disable Formatting for Selected Text");
}

QString BeautifierTool::msgCommandPromptDialogTitle(const QString &command)
{
    //: File dialog title for path chooser when choosing binary
    return Tr::tr("%1 Command").arg(command);
}

class VersionUpdater
{
public:
    VersionUpdater()
    {
        QObject::connect(&m_process, &Process::done, &m_process, [this] {
            if (m_process.result() != ProcessResult::FinishedWithSuccess)
                return;

            m_versionNumber = parseVersion(m_process.cleanedStdOut());
            if (m_versionNumber.isNull())
                m_versionNumber = parseVersion(m_process.cleanedStdErr());
        });
    }

    void setVersionRegExp(const QRegularExpression &versionRegExp)
    {
        m_versionRegExp = versionRegExp;
    }

    void update(const FilePath &executable)
    {
        m_versionNumber = {};
        if (m_versionRegExp.pattern().isEmpty())
            return;
        m_process.close();
        m_process.setCommand({executable, {"--version"}});
        m_process.start();
    }

    QVersionNumber version() const
    {
        if (m_process.state() != QProcess::NotRunning)
            m_process.waitForFinished(-1);
        return m_versionNumber;
    }

private:
    QVersionNumber parseVersion(const QString &text) const
    {
        const QRegularExpressionMatch match = m_versionRegExp.match(text);
        if (!match.hasMatch())
            return {};

        return {match.captured(1).toInt(), match.captured(2).toInt()};
    }

    QRegularExpression m_versionRegExp;
    mutable Process m_process;
    QVersionNumber m_versionNumber;
};

AbstractSettings::AbstractSettings(const QString &name, const QString &ending)
    : m_ending(ending)
    , m_styleDir(Core::ICore::userResourcePath(Beautifier::Constants::SETTINGS_DIRNAME)
                     .pathAppended(name))
{
    setSettingsGroups(Utils::Constants::BEAUTIFIER_SETTINGS_GROUP, name);
    setAutoApply(false);

    command.setSettingsKey("command");
    command.setExpectedKind(PathChooser::ExistingCommand);
    command.setCommandVersionArguments({"--version"});
    command.setPromptDialogTitle(BeautifierTool::msgCommandPromptDialogTitle("Clang Format"));
    command.setValidatePlaceHolder(true);

    supportedMimeTypes.setDisplayStyle(StringAspect::LineEditDisplay);
    supportedMimeTypes.setSettingsKey("supportedMime");
    supportedMimeTypes.setLabelText(Tr::tr("Restrict to MIME types:"));
    supportedMimeTypes.setDefaultValue("text/x-c++src; text/x-c++hdr; text/x-csrc;text/x-chdr; "
                                       "text/x-objcsrc; text/x-objc++src");

    supportedMimeTypes.setValueAcceptor([](const QString &, const QString &value) -> std::optional<QString> {
        const QStringList stringTypes = value.split(';');
        QStringList types;
        for (const QString &type : stringTypes) {
            const MimeType mime = mimeTypeForName(type.trimmed());
            if (!mime.isValid())
                continue;
            const QString canonicalName = mime.name();
            if (!types.contains(canonicalName))
                types << canonicalName;
        }
        return types.join("; ");
    });

    connect(&command, &BaseAspect::changed, this, [this] { m_version = {}; version(); });
}

AbstractSettings::~AbstractSettings() = default;

QStringList AbstractSettings::completerWords()
{
    return {};
}

QStringList AbstractSettings::styles() const
{
    QStringList list = m_styles.keys();
    list.sort(Qt::CaseInsensitive);
    return list;
}

QString AbstractSettings::style(const QString &key) const
{
    return m_styles.value(key);
}

bool AbstractSettings::styleExists(const QString &key) const
{
    return m_styles.contains(key);
}

bool AbstractSettings::styleIsReadOnly(const QString &key)
{
    const FilePath filePath = m_styleDir.pathAppended(key + m_ending);
    if (!filePath.exists()) {
        // newly added style which was not saved yet., thus it is not read only.
        //TODO In a later version when we have predefined styles in Core::ICore::resourcePath()
        //     we need to check if it is a read only global config file...
        return false;
    }

    return !filePath.isWritableFile();
}

void AbstractSettings::setStyle(const QString &key, const QString &value)
{
    m_styles.insert(key, value);
    m_changedStyles.insert(key);
}

void AbstractSettings::removeStyle(const QString &key)
{
    m_styles.remove(key);
    m_stylesToRemove << key;
}

void AbstractSettings::replaceStyle(const QString &oldKey, const QString &newKey,
                                    const QString &value)
{
    // Set value regardles if keys are equal
    m_styles.insert(newKey, value);

    if (oldKey != newKey)
        removeStyle(oldKey);

    m_changedStyles.insert(newKey);
}

FilePath AbstractSettings::styleFileName(const QString &key) const
{
    return m_styleDir.pathAppended(key + m_ending);
}

QVersionNumber AbstractSettings::version() const
{
    if (m_version.isNull()) {
        VersionUpdater updater;
        updater.setVersionRegExp(m_versionRegExp);
        updater.update(command());
        m_version = updater.version();
    }
    return m_version;
}

void AbstractSettings::setVersionRegExp(const QRegularExpression &versionRegExp)
{
    m_versionRegExp = versionRegExp;
}

bool AbstractSettings::isApplicable(const Core::IDocument *document) const
{
    if (!document)
        return false;

    if (m_supportedMimeTypes.isEmpty())
        return true;

    const MimeType documentMimeType = mimeTypeForName(document->mimeType());
    return anyOf(m_supportedMimeTypes, [&documentMimeType](const QString &mime) {
        return documentMimeType.inherits(mime);
    });
}

QStringList AbstractSettings::options()
{
    if (m_options.isEmpty())
        readDocumentation();

    return m_options.keys();
}

QString AbstractSettings::documentation(const QString &option) const
{
    const int index = m_options.value(option, -1);
    if (index != -1)
        return m_docu.at(index);
    else
        return QString();
}

void AbstractSettings::save()
{
    // Save settings, except styles
    AspectContainer::writeSettings();

    // Save styles
    if (m_stylesToRemove.isEmpty() && m_styles.isEmpty())
        return;

    // remove old files and possible subfolder
    for (const QString &key : std::as_const(m_stylesToRemove)) {
        FilePath filePath = styleFileName(key);
        filePath.removeFile();
        QTC_ASSERT(m_styleDir.isAbsolutePath(), break);
        QTC_ASSERT(!m_styleDir.needsDevice(), break);
        if (filePath.parentDir() != m_styleDir) {
            // FIXME: Missing in FilePath
            QDir(m_styleDir.toString()).rmdir(filePath.parentDir().toString());
        }
    }
    m_stylesToRemove.clear();

    QMap<QString, QString>::const_iterator iStyles = m_styles.constBegin();
    while (iStyles != m_styles.constEnd()) {
        // Only save changed styles.
        if (!m_changedStyles.contains(iStyles.key())) {
            ++iStyles;
            continue;
        }

        const FilePath filePath = styleFileName(iStyles.key());
        if (!filePath.parentDir().ensureWritableDir()) {
            BeautifierTool::showError(Tr::tr("Cannot save styles. %1 does not exist.")
                                          .arg(filePath.toUserOutput()));
            continue;
        }

        FileSaver saver(filePath);
        if (saver.hasError()) {
            BeautifierTool::showError(Tr::tr("Cannot open file \"%1\": %2.")
                                          .arg(filePath.toUserOutput())
                                          .arg(saver.errorString()));
        } else {
            saver.write(iStyles.value().toLocal8Bit());
            if (!saver.finalize()) {
                BeautifierTool::showError(Tr::tr("Cannot save file \"%1\": %2.")
                                              .arg(filePath.toUserOutput())
                                              .arg(saver.errorString()));
            }
        }
        ++iStyles;
    }

    m_changedStyles.clear();
}

void AbstractSettings::createDocumentationFile() const
{
    // Could be reimplemented to create a documentation file.
}

void AbstractSettings::read()
{
    // Read settings, except styles
    AspectContainer::readSettings();

    m_styles.clear();
    m_changedStyles.clear();
    m_stylesToRemove.clear();
    readStyles();
}

void AbstractSettings::readDocumentation()
{
    const FilePath filename = documentationFilePath;
    if (filename.isEmpty()) {
        BeautifierTool::showError(Tr::tr("No documentation file specified."));
        return;
    }

    QFile file(filename.toFSPathString());
    if (!file.exists())
        createDocumentationFile();

    if (!file.open(QIODevice::ReadOnly)) {
        BeautifierTool::showError(Tr::tr("Cannot open documentation file \"%1\".")
                                      .arg(filename.toUserOutput()));
        return;
    }

    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement())
        return;
    if (xml.name() != QLatin1String(Constants::DOCUMENTATION_XMLROOT)) {
        BeautifierTool::showError(Tr::tr("The file \"%1\" is not a valid documentation file.")
                                      .arg(filename.toUserOutput()));
        return;
    }

    // We do not use QHash<QString, QString> since e.g. in Artistic Style there are many different
    // keys with the same (long) documentation text. By splitting the keys from the documentation
    // text we save a huge amount of memory.
    m_options.clear();
    m_docu.clear();
    QStringList keys;
    while (!(xml.atEnd() || xml.hasError())) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            QStringView name = xml.name();
            if (name == QLatin1String(Constants::DOCUMENTATION_XMLENTRY)) {
                keys.clear();
            } else if (name == QLatin1String(Constants::DOCUMENTATION_XMLKEY)) {
                if (xml.readNext() == QXmlStreamReader::Characters)
                    keys << xml.text().toString();
            } else if (name == QLatin1String(Constants::DOCUMENTATION_XMLDOC)) {
                if (xml.readNext() == QXmlStreamReader::Characters) {
                    m_docu << xml.text().toString();
                    const int index = m_docu.size() - 1;
                    for (const QString &key : std::as_const(keys))
                        m_options.insert(key, index);
                }
            }
        }
    }

    if (xml.hasError()) {
        BeautifierTool::showError(Tr::tr("Cannot read documentation file \"%1\": %2.")
                                      .arg(filename.toUserOutput()).arg(xml.errorString()));
    }
}

void AbstractSettings::readStyles()
{
    if (!m_styleDir.exists())
        return;

    const FileFilter filter = {{'*' + m_ending}, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot};
    const FilePaths files = m_styleDir.dirEntries(filter);
    for (const FilePath &filePath : files) {
        // do not allow empty file names
        if (filePath.fileName() == m_ending)
            continue;

        if (auto contents = filePath.fileContents()) {
            const QString filename = filePath.fileName();
            m_styles.insert(filename.left(filename.length() - m_ending.length()),
                            QString::fromLocal8Bit(*contents));
        }
    }
}

} // Beautifier::Internal
