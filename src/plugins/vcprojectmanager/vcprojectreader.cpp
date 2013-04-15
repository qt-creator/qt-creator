#include "vcprojectreader.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectnodes.h>

#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QDir>
#include <QDebug>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

#define STRING(s) static const char str##s[] = #s

STRING(Configuration);
STRING(Configurations);
STRING(File);
STRING(Files);
STRING(Filter);
STRING(FileConfiguration);
STRING(Globals);
STRING(Name);
STRING(Platform);
STRING(Platforms);
STRING(PublishingData);
STRING(References);
STRING(RelativePath);
STRING(Tool);
STRING(ToolFiles);
STRING(VisualStudioProject);
STRING(Version);
STRING(PreprocessorDefinitions);
STRING(WarningLevel);
STRING(VCCLCompilerTool);
STRING(OpenMP);

using namespace VcProjectInfo;

VcProjectInfo::Filter::~Filter()
{
    qDeleteAll(filters);
    qDeleteAll(files);
}

ConfigurationInfo::Flags::Flags()
    : useOpenMP(false)
    , warningLevel(1)
{
}

Project::Project()
    : files(0)
{
}

Project::~Project()
{
    delete files;
}

VcProjectReader::VcProjectReader()
    : m_project(0)
    , m_currentFilter(0)
{
}

Project *VcProjectReader::parse(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return 0;

    setDevice(&file);

    m_project = new Project;
    m_project->projectFilePath = QFileInfo(filePath).canonicalPath();

    if (!atEnd()) {
        // read xml declaration
        readNextNonSpace();
        if (isStartDocument())
            readNextNonSpace();
        else
            customError(QObject::tr("XML Declaration missing"));

        if (elementStarts(QLatin1String(strVisualStudioProject))) {
            readVisualStudioProject();
            readNextNonSpace();
        }
        else
            missingMandatoryTag(QLatin1String(strVisualStudioProject));

        if (!isEndDocument())
            unexpectedElement();
    } else {
        customError(QObject::tr("Project file is empty"));
    }

    if (hasError()) {
        // TODO: error
        qDebug() << errorString() << lineNumber() << columnNumber();
        return 0;
    }

    qDebug() << "parsing successful";
    return m_project;
}

void VcProjectReader::readVisualStudioProject()
{
    m_project->displayName = attrStr(QLatin1String(strName));

    // Platforms is mandatory element
    readNextNonSpace();
    if (elementStarts(QLatin1String(strPlatforms))) {
        readPlatforms();
        readNextNonSpace();
    } else {
        missingMandatoryTag(QLatin1String(strPlatforms));
    }

    // ToolFiles is optional element
    if (elementStarts(QLatin1String(strToolFiles))) {
        readToolFiles();
        readNextNonSpace();
    }

    // PublishingData is optional element
    if (elementStarts(QLatin1String(strPublishingData))) {
        readPublishingData();
        readNextNonSpace();
    }

    // Configurations is mandatory element
    if (elementStarts(QLatin1String(strConfigurations))) {
        readConfigurations();
        readNextNonSpace();
    } else {
        missingMandatoryTag(QLatin1String(strConfigurations));
    }

    // References is optional element
    if (elementStarts(QLatin1String(strReferences))) {
        readReferences();
        readNextNonSpace();
    }

    // Files is optional element
    if (elementStarts(QLatin1String(strFiles))) {
        readFiles();
        readNextNonSpace();
    }

    // Globals is optional element
    if (elementStarts(QLatin1String(strGlobals))) {
        readGlobals();
        readNextNonSpace();
    }

    if (!elementEnds(QLatin1String(strVisualStudioProject)))
        unexpectedElement();
}

void VcProjectReader::readPlatforms()
{
    while (!elementEnds(QLatin1String(strPlatforms)) && !hasError()) {
        readNextNonSpace();
    }
}

void VcProjectReader::readToolFiles()
{
    while (!elementEnds(QLatin1String(strToolFiles)) && !hasError()) {
        readNextNonSpace();
    }
}

void VcProjectReader::readPublishingData()
{
    while (!elementEnds(QLatin1String(strPublishingData)) && !hasError()) {
        readNextNonSpace();
    }
}

void VcProjectReader::readConfigurations()
{
    while (!elementEnds(QLatin1String(strConfigurations)) && !hasError()) {
        if (elementStarts(QLatin1String(strConfiguration))) {
            readConfiguration();
        }
        readNextNonSpace();
    }
}

void VcProjectReader::readConfiguration()
{
    QString saved = m_currentConfigurationName;
    m_currentConfigurationName = attrStr(QLatin1String(strName));
    readNextNonSpace();
    while (!elementEnds(QLatin1String(strConfiguration)) && !hasError()) {
        if (elementStarts(QLatin1String(strTool))) {
            readTool();
            readNextNonSpace();
        } else {
            unexpectedElement();
        }
    }
    m_currentConfigurationName = saved;
}

void VcProjectReader::readTool()
{
    if (attrStr(QLatin1String(strName)) == QLatin1String(strVCCLCompilerTool)) {
        ConfigurationInfo &info = m_project->configurations[m_currentConfigurationName];
        info.flags.useOpenMP = (attrStr(QLatin1String(strOpenMP)) == QLatin1String("true"));
        bool ok = true;
        const unsigned warningLevel = attrStr(QLatin1String(strWarningLevel)).toUInt(&ok);
        if (ok)
            info.flags.warningLevel = warningLevel;
        info.defines = attrStr(QLatin1String(strPreprocessorDefinitions)).split(
                    QLatin1String(";"), QString::SkipEmptyParts);
    }
    while (!elementEnds(QLatin1String(strTool)) && !hasError()) {
        readNextNonSpace();
    }
}

void VcProjectReader::readReferences()
{
    while (!elementEnds(QLatin1String(strReferences)) && !hasError()) {
        readNextNonSpace();
    }
}

void VcProjectReader::readFiles()
{
    m_project->files = new Filter;
    m_currentFilter = m_project->files;
    readNextNonSpace();
    while (!elementEnds(QLatin1String(strFiles)) && !hasError()) {
        if (elementStarts(QLatin1String(strFile))) {
            readFile();
            readNextNonSpace();
        } else if (elementStarts(QLatin1String(strFilter))) {
            readFilter();
            readNextNonSpace();
        } else {
            unexpectedElement();
        }
    }
}

void VcProjectReader::readFile()
{
    File *file = new File;
    file->relativePath = attrStr(QLatin1String(strRelativePath));
#if !defined(Q_OS_WIN)
    file->relativePath.replace(QLatin1Char('\\'), QDir::separator());
#endif
    m_currentFilter->files.append(file);

    readNextNonSpace();
    while (!elementEnds(QLatin1String(strFile)) && !hasError()) {
        if (elementStarts(QLatin1String(strFileConfiguration))) {
            readFileConfiguration();
            readNextNonSpace();
        } else {
            unexpectedElement();
        }
    }
}

void VcProjectReader::readFileConfiguration()
{
    while (!elementEnds(QLatin1String(strFileConfiguration)) && !hasError()) {
        readNextNonSpace();
    }
}

void VcProjectReader::readFilter()
{
    Filter *filter = new Filter;
    filter->name = attrStr(QLatin1String(strName));
    m_currentFilter->filters.append(filter);

    Filter *prevFilter = m_currentFilter;
    m_currentFilter = filter;

    readNextNonSpace();
    while (!elementEnds(QLatin1String(strFilter)) && !hasError()) {
        if (elementStarts(QLatin1String(strFile))) {
            readFile();
            readNextNonSpace();
        } else if (elementStarts(QLatin1String(strFilter))) {
            readFilter();
            readNextNonSpace();
        } else {
            unexpectedElement();
        }
    }

    m_currentFilter = prevFilter;
}

void VcProjectReader::readGlobals()
{
    while (!elementEnds(QLatin1String(strGlobals)) && !hasError()) {
        readNextNonSpace();
    }
}

QString VcProjectReader::attrStr(const QString &attrName)
{
    // TODO: errors?
    return attributes().value(attrName).toString();
}

bool VcProjectReader::elementStarts(const QString &str) const
{
    return isStartElement() && name() == str;
}

bool VcProjectReader::elementEnds(const QString &str) const
{
    return isEndElement() && name() == str;
}

void VcProjectReader::readNextNonSpace()
{
    if (!hasError()) {
        do {
            readNext();
        } while (isCharacters() && text().toString().trimmed().isEmpty());
    }
    m_currentElement = name().toString();
//    qDebug() << "current element:" << (isEndElement() ? "\\": "") << m_currentElement;
}

void VcProjectReader::customError(const QString &message)
{
    if (!hasError())
        raiseError(message);
}

void VcProjectReader::missingMandatoryTag(const QString &tagName)
{
    customError(QLatin1String("<") + tagName + QLatin1String(">") + QLatin1String(" expected"));
}

void VcProjectReader::unexpectedElement()
{
    customError(QLatin1String("Unexpected ") + name());
}

} // namespace Internal
} // namespace VcProjectManager
