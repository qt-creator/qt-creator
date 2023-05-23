// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractsettings.h"

#include "beautifierconstants.h"
#include "beautifierplugin.h"
#include "beautifiertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/genericconstants.h>
#include <utils/mimeutils.h>
#include <utils/process.h>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QXmlStreamReader>

using namespace Utils;

namespace Beautifier::Internal {

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
                     .pathAppended(name)
                     .toString())
    , m_versionUpdater(new VersionUpdater)
{
    setSettingsGroups(Utils::Constants::BEAUTIFIER_SETTINGS_GROUP, name);

    registerAspect(&command);
    command.setSettingsKey("command");
    command.setExpectedKind(Utils::PathChooser::ExistingCommand);
    command.setCommandVersionArguments({"--version"});
    command.setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle("Clang Format"));

    registerAspect(&supportedMimeTypes);
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

    connect(&command, &BaseAspect::changed, this, [this] {
        m_versionUpdater->update(command());
    });
}

AbstractSettings::~AbstractSettings() = default;

QStringList AbstractSettings::completerWords()
{
    return QStringList();
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
    const QFileInfo fi(m_styleDir.absoluteFilePath(key + m_ending));
    if (!fi.exists()) {
        // newly added style which was not saved yet., thus it is not read only.
        //TODO In a later version when we have predefined styles in Core::ICore::resourcePath()
        //     we need to check if it is a read only global config file...
        return false;
    }

    return !fi.isWritable();
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

QString AbstractSettings::styleFileName(const QString &key) const
{
    return m_styleDir.absoluteFilePath(key + m_ending);
}

QVersionNumber AbstractSettings::version() const
{
    return m_versionUpdater->version();
}

void AbstractSettings::setVersionRegExp(const QRegularExpression &versionRegExp)
{
    m_versionUpdater->setVersionRegExp(versionRegExp);
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
    QSettings *s = Core::ICore::settings();

    AspectContainer::writeSettings(s);

    // Save styles
    if (m_stylesToRemove.isEmpty() && m_styles.isEmpty())
        return;

    // remove old files and possible subfolder
    for (const QString &key : std::as_const(m_stylesToRemove)) {
        const QFileInfo fi(styleFileName(key));
        QFile::remove(fi.absoluteFilePath());
        if (fi.absoluteDir() != m_styleDir)
            m_styleDir.rmdir(fi.absolutePath());
    }
    m_stylesToRemove.clear();

    QMap<QString, QString>::const_iterator iStyles = m_styles.constBegin();
    while (iStyles != m_styles.constEnd()) {
        // Only save changed styles.
        if (!m_changedStyles.contains(iStyles.key())) {
            ++iStyles;
            continue;
        }

        const QFileInfo fi(styleFileName(iStyles.key()));
        if (!(m_styleDir.mkpath(fi.absolutePath()))) {
            BeautifierPlugin::showError(Tr::tr("Cannot save styles. %1 does not exist.")
                                        .arg(fi.absolutePath()));
            continue;
        }

        FileSaver saver(FilePath::fromUserInput(fi.absoluteFilePath()));
        if (saver.hasError()) {
            BeautifierPlugin::showError(Tr::tr("Cannot open file \"%1\": %2.")
                                        .arg(saver.filePath().toUserOutput())
                                        .arg(saver.errorString()));
        } else {
            saver.write(iStyles.value().toLocal8Bit());
            if (!saver.finalize()) {
                BeautifierPlugin::showError(Tr::tr("Cannot save file \"%1\": %2.")
                                            .arg(saver.filePath().toUserOutput())
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
    QSettings *s = Core::ICore::settings();
    AspectContainer::readSettings(s);

    m_styles.clear();
    m_changedStyles.clear();
    m_stylesToRemove.clear();
    readStyles();
}

void AbstractSettings::readDocumentation()
{
    const FilePath filename = documentationFilePath();
    if (filename.isEmpty()) {
        BeautifierPlugin::showError(Tr::tr("No documentation file specified."));
        return;
    }

    QFile file(filename.toFSPathString());
    if (!file.exists())
        createDocumentationFile();

    if (!file.open(QIODevice::ReadOnly)) {
        BeautifierPlugin::showError(Tr::tr("Cannot open documentation file \"%1\".")
            .arg(filename.toUserOutput()));
        return;
    }

    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement())
        return;
    if (xml.name() != QLatin1String(Constants::DOCUMENTATION_XMLROOT)) {
        BeautifierPlugin::showError(Tr::tr("The file \"%1\" is not a valid documentation file.")
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
        BeautifierPlugin::showError(Tr::tr("Cannot read documentation file \"%1\": %2.")
                                    .arg(filename.toUserOutput()).arg(xml.errorString()));
    }
}

void AbstractSettings::readStyles()
{
    if (!m_styleDir.exists())
        return;

    const QStringList files
            = m_styleDir.entryList({'*' + m_ending},
                                   QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    for (const QString &filename : files) {
        // do not allow empty file names
        if (filename == m_ending)
            continue;

        QFile file(m_styleDir.absoluteFilePath(filename));
        if (file.open(QIODevice::ReadOnly)) {
            m_styles.insert(filename.left(filename.length() - m_ending.length()),
                            QString::fromLocal8Bit(file.readAll()));
        }
    }
}

} // Beautifier::Internal
