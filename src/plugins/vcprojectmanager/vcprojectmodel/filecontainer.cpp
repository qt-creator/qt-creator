#include "filecontainer.h"

#include <QFileInfo>
#include <utils/qtcassert.h>

#include "vcprojectdocument.h"
#include "generalattributecontainer.h"
#include "../vcprojectmanagerconstants.h"

namespace VcProjectManager {
namespace Internal {

FileContainer::FileContainer(const QString &containerType, IVisualStudioProject *parentProjectDoc)
    : m_parentProjectDoc(parentProjectDoc),
      m_containerType(containerType)
{
    m_attributeContainer = new GeneralAttributeContainer;
}

FileContainer::FileContainer(const FileContainer &fileContainer)
{
    m_parentProjectDoc = fileContainer.m_parentProjectDoc;
    m_name = fileContainer.m_name;
    m_attributeContainer = new GeneralAttributeContainer;
    *m_attributeContainer = *(fileContainer.m_attributeContainer);

    foreach (IFile *file, fileContainer.m_files)
        m_files.append(file->clone());

    foreach (IFileContainer *filter, fileContainer.m_fileContainers)
        m_fileContainers.append(filter->clone());
}

FileContainer &FileContainer::operator=(const FileContainer &fileContainer)
{
    if (this != &fileContainer) {
        m_parentProjectDoc = fileContainer.m_parentProjectDoc;
        m_name = fileContainer.m_name;
        *m_attributeContainer = *(fileContainer.m_attributeContainer);

        qDeleteAll(m_files);
        qDeleteAll(m_fileContainers);
        m_files.clear();
        m_fileContainers.clear();

        foreach (IFile *file, fileContainer.m_files)
            m_files.append(file->clone());

        foreach (IFileContainer *filter, fileContainer.m_fileContainers)
            m_fileContainers.append(filter->clone());
    }

    return *this;
}

FileContainer::~FileContainer()
{
    qDeleteAll(m_files);
    qDeleteAll(m_fileContainers);
    delete m_attributeContainer;
}

QString FileContainer::containerType() const
{
    return m_containerType;
}

void FileContainer::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull()) {
            if (firstChild.nodeName() == QLatin1String("Filter"))
                processFilter(firstChild);
            else if (firstChild.nodeName() == QLatin1String("File"))
                processFile(firstChild);
            else if (firstChild.nodeName() == QLatin1String("Folder"))
                processFolder(firstChild);
        }
    }
}

VcNodeWidget *FileContainer::createSettingsWidget()
{
    return 0;
}

QDomNode FileContainer::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(m_containerType);

    fileNode.setAttribute(QLatin1String("Name"), m_name);

    m_attributeContainer->appendToXMLNode(fileNode);

    foreach (IFile *file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (IFileContainer *filter, m_fileContainers)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    return fileNode;
}

void FileContainer::addFile(IFile *file)
{
    if (m_files.contains(file))
        return;

    foreach (IFile *f, m_files) {
        if (f->relativePath() == file->relativePath())
            return;
    }
    m_files.append(file);
}

void FileContainer::removeFile(IFile *file)
{
    m_files.removeAll(file);
    delete file;
}

IFile *FileContainer::file(int index) const
{
    QTC_ASSERT(0 <= index && index < m_files.size(), return 0);
    return m_files[index];
}

int FileContainer::fileCount() const
{
    return m_files.size();
}

void FileContainer::addFileContainer(IFileContainer *fileContainer)
{
    if (!fileContainer && m_fileContainers.contains(fileContainer))
        return;

    m_fileContainers.append(fileContainer);
}

int FileContainer::childCount() const
{
    return m_fileContainers.size();
}

IFileContainer *FileContainer::fileContainer(int index) const
{
    QTC_ASSERT(0 <= index && index < m_fileContainers.size(), return 0);
    return m_fileContainers[index];
}

void FileContainer::removeFileContainer(IFileContainer *fileContainer)
{
    m_fileContainers.removeAll(fileContainer);
}

IAttributeContainer *FileContainer::attributeContainer() const
{
    return m_attributeContainer;
}

bool FileContainer::fileExists(const QString &relativeFilePath) const
{
    foreach (IFile *filePtr, m_files) {
        if (filePtr->relativePath() == relativeFilePath)
            return true;
    }

    foreach (IFileContainer *filterPtr, m_fileContainers) {
        if (filterPtr->fileExists(relativeFilePath))
            return true;
    }

    return false;
}

IFileContainer *FileContainer::clone() const
{
    return new FileContainer(*this);
}

QString FileContainer::displayName() const
{
    return m_name;
}

void FileContainer::setDisplayName(const QString &name)
{
    m_name = name;
}

void FileContainer::allFiles(QStringList &sl) const
{
    foreach (IFileContainer *filter, m_fileContainers)
        filter->allFiles(sl);

    foreach (IFile *file, m_files)
        sl.append(file->canonicalPath());
}

void FileContainer::processFile(const QDomNode &fileNode)
{
    IFile *file = new File(m_parentProjectDoc);
    file->processNode(fileNode);
    m_files.append(file);

    // process next sibling
    QDomNode nextSibling = fileNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Folder"))
            processFolder(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void FileContainer::processFilter(const QDomNode &filterNode)
{
    IFileContainer *filter = new FileContainer(QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FILTER),
                                               m_parentProjectDoc);
    filter->processNode(filterNode);
    m_fileContainers.append(filter);

    // process next sibling
    QDomNode nextSibling = filterNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Folder"))
            processFolder(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void FileContainer::processFolder(const QDomNode &folderNode)
{
    IFileContainer *folder = new FileContainer(QLatin1String(Constants::VC_PROJECT_FILE_CONTAINER_FOLDER),
                                               m_parentProjectDoc);
    folder->processNode(folderNode);
    m_fileContainers.append(folder);

    // process next sibling
    QDomNode nextSibling = folderNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Folder"))
            processFolder(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void FileContainer::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                setDisplayName(domElement.value());

            else
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
