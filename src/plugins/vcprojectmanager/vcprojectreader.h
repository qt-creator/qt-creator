#ifndef VCPROJECTREADER_H
#define VCPROJECTREADER_H

#include "vcprojectnodes.h"

#include <QXmlStreamReader>

class QFileInfo;
class QString;

namespace Core {
class MimeDatabase;
}

namespace ProjectExplorer {
class FolderNode;
}

namespace VcProjectManager {
namespace Internal {

namespace VcProjectInfo {

struct File
{
    QString relativePath;
};

struct Filter
{
    ~Filter();

    QString name;
    QList<Filter *> filters;
    QList<File *> files;
};

struct Project
{
    Project();
    ~Project();

    QString projectFilePath; // TODO: redundant?
    QString displayName;

    Filter *files;
};

}

class VcProjectNode;

class VcProjectReader : public QXmlStreamReader
{
public:
    VcProjectReader();

    VcProjectInfo::Project *parse(const QString &filePath);

private:
    void readVisualStudioProject();
    void readPlatforms();
    void readToolFiles();
    void readPublishingData();
    void readConfigurations();
    void readReferences();
    void readFiles();
    void readFile();
    void readFileConfiguration();
    void readFilter();
    void readGlobals();

    QString attrStr(const QString &attrName);
    bool elementStarts(const QString &str) const;
    bool elementEnds(const QString &str) const;
    void readNextNonSpace();

    void customError(const QString &message);
    void missingMandatoryTag(const QString &tagName);
    void unexpectedElement();

private:
    VcProjectInfo::Project *m_project;
    VcProjectInfo::Filter *m_currentFilter;

    QString m_currentElement;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTREADER_H
