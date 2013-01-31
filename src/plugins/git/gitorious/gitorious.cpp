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

#include "gitorious.h"

#include <QDebug>
#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QSettings>

#include <QNetworkReply>

#include <utils/qtcassert.h>
#include <utils/networkaccessmanager.h>

enum { debug = 0 };

enum Protocol { ListCategoriesProtocol, ListProjectsProtocol };

static const char protocolPropertyC[] = "gitoriousProtocol";
static const char hostNamePropertyC[] = "gitoriousHost";
static const char pagePropertyC[] = "requestPage";

static const char settingsKeyC[] = "GitoriousHosts";

// Gitorious paginates projects as 20 per page. It starts with page 1.
enum { ProjectsPageSize = 20 };

// Format an URL for a http request
static inline QUrl httpRequest(const QString &host, const QString &request)
{
    QUrl url;
    url.setScheme(QLatin1String("http"));
    const QStringList hostList = host.split(QLatin1Char(':'), QString::SkipEmptyParts);
    if (hostList.size() > 0)
    {
        url.setHost(hostList.at(0));
        if (hostList.size() > 1)
            url.setPort(hostList.at(1).toInt());
    }
    url.setPath(QLatin1Char('/') + request);
    return url;
}

namespace Gitorious {
namespace Internal {

GitoriousRepository::GitoriousRepository() :
    type(BaselineRepository),
    id(0)
{
}

static inline GitoriousRepository::Type repositoryType(const QString &nspace)
{
    if (nspace == QLatin1String("Repository::Namespace::BASELINE"))
        return GitoriousRepository::BaselineRepository;
    if (nspace == QLatin1String("Repository::Namespace::SHARED"))
        return GitoriousRepository::SharedRepository;
    if (nspace == QLatin1String("Repository::Namespace::PERSONAL"))
        return GitoriousRepository::PersonalRepository;
    return GitoriousRepository::BaselineRepository;
}

GitoriousCategory::GitoriousCategory(const QString &n) :
    name(n)
{
}

GitoriousHost::GitoriousHost(const QString &h, const QString &d) :
    hostName(h),
    description(d),
    state(ProjectsQueryRunning)
{
}

int GitoriousHost::findCategory(const QString &n) const
{
    const int count = categories.size();
    for (int i = 0; i < count; i++)
        if (categories.at(i)->name == n)
            return i;
    return -1;
}

QDebug operator<<(QDebug d, const GitoriousRepository &r)
{
    QDebug nospace = d.nospace();
    nospace << "name=" << r.name <<  '/' << r.id << '/' << r.type << r.owner
            <<" push=" << r.pushUrl << " clone=" << r.cloneUrl << " descr=" << r.description;
    return d;
}

QDebug operator<<(QDebug d, const GitoriousProject &p)
{
    QDebug nospace = d.nospace();
    nospace << "  project=" << p.name << " description=" << p.description << '\n';
    foreach (const GitoriousRepository &r, p.repositories)
        nospace << "    " << r << '\n';
    return d;
}

QDebug operator<<(QDebug d, const GitoriousCategory &c)
{
    d.nospace() << "  category=" << c.name <<  '\n';
    return d;
}

QDebug operator<<(QDebug d, const GitoriousHost &h)
{
    QDebug nospace = d.nospace();
    nospace << "  Host=" << h.hostName << " description=" << h.description << '\n';
    foreach (const QSharedPointer<GitoriousCategory> &c, h.categories)
        nospace << *c;
    foreach (const QSharedPointer<GitoriousProject> &p, h.projects)
        nospace << *p;
    return d;
}

/* GitoriousProjectReader: Helper class for parsing project list output
 * \code
projects...>
  <project>
    <bugtracker-url>
    <created-at>
    <description>... </description>
    <home-url> (rarely set)
    <license>
    <mailinglist-url>
    <slug> (name)
    <title>MuleFTW</title>
    <owner>
    <repositories>
      <mainlines> // Optional
        <repository>
          <id>
          <name>
          <owner>
          <clone_url>
        </repository>
      </mainlines>
      <clones> // Optional
      </clones>
    </repositories>
  </project>

 * \endcode  */

class GitoriousProjectReader
{
    Q_DISABLE_COPY(GitoriousProjectReader)
public:
    typedef GitoriousCategory::ProjectList ProjectList;

    GitoriousProjectReader();
    ProjectList read(const QByteArray &a, QString *errorMessage);

private:
    void readProjects(QXmlStreamReader &r);
    QSharedPointer<GitoriousProject> readProject(QXmlStreamReader &r);
    QList<GitoriousRepository> readRepositories(QXmlStreamReader &r);
    GitoriousRepository readRepository(QXmlStreamReader &r, int defaultType = -1);
    void readUnknownElement(QXmlStreamReader &r);

    const QString m_mainLinesElement;
    const QString m_clonesElement;
    ProjectList m_projects;
};

GitoriousProjectReader::GitoriousProjectReader() :
    m_mainLinesElement(QLatin1String("mainlines")),
    m_clonesElement(QLatin1String("clones"))
{
}

GitoriousProjectReader::ProjectList GitoriousProjectReader::read(const QByteArray &a, QString *errorMessage)
{
    m_projects.clear();
    QXmlStreamReader reader(a);

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("projects"))
                readProjects(reader);
            else
                readUnknownElement(reader);
        }
    }

    if (reader.hasError()) {
        *errorMessage = QString::fromLatin1("Error at %1:%2: %3").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(reader.errorString());
        m_projects.clear();
    }

    return m_projects;
}

bool gitoriousProjectLessThan(const QSharedPointer<GitoriousProject> &p1, const QSharedPointer<GitoriousProject> &p2)
{
    return p1->name.compare(p2->name, Qt::CaseInsensitive) < 0;
}

void GitoriousProjectReader::readProjects(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isEndElement())
            break;

        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("project")) {
                const QSharedPointer<GitoriousProject> p = readProject(reader);
                if (!p->name.isEmpty())
                    m_projects.push_back(p);
            } else {
                readUnknownElement(reader);
            }
        }
    }
}

QSharedPointer<GitoriousProject> GitoriousProjectReader::readProject(QXmlStreamReader &reader)
{
    QSharedPointer<GitoriousProject> project(new GitoriousProject);

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isEndElement())
            break;

        if (reader.isStartElement()) {
            const QStringRef name = reader.name();
            if (name == QLatin1String("description"))
                project->description = reader.readElementText();
            else if (name == QLatin1String("title"))
                project->name = reader.readElementText();
            else if (name == QLatin1String("slug") && project->name.isEmpty())
                project->name = reader.readElementText();
            else if (name == QLatin1String("repositories"))
                project->repositories = readRepositories(reader);
            else
                readUnknownElement(reader);
        }
    }
    return project;
}

QList<GitoriousRepository> GitoriousProjectReader::readRepositories(QXmlStreamReader &reader)
{
    QList<GitoriousRepository> repositories;
    int defaultType = -1;

    // The "mainlines"/"clones" elements are not used in the
    // QtProject setup, handle them optionally.
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isEndElement()) {
            const QStringRef name = reader.name();
            if (name == m_mainLinesElement || name == m_clonesElement)
                defaultType = -1;
            else
                break;
        }

        if (reader.isStartElement()) {
            const QStringRef name = reader.name();
            if (reader.name() == QLatin1String("repository"))
                repositories.push_back(readRepository(reader, defaultType));
            else if (name == m_mainLinesElement)
                defaultType = GitoriousRepository::MainLineRepository;
            else if (name == m_clonesElement)
                defaultType = GitoriousRepository::CloneRepository;
            else
                readUnknownElement(reader);
        }
    }
    return repositories;
}

GitoriousRepository GitoriousProjectReader::readRepository(QXmlStreamReader &reader, int defaultType)
{
    GitoriousRepository repository;
    if (defaultType >= 0)
        repository.type = static_cast<GitoriousRepository::Type>(defaultType);

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isEndElement())
            break;

        if (reader.isStartElement()) {
            const QStringRef name = reader.name();
            if (name == QLatin1String("name"))
                repository.name = reader.readElementText();
            else if (name == QLatin1String("owner"))
                repository.owner = reader.readElementText();
            else if (name == QLatin1String("id"))
                repository.id = reader.readElementText().toInt();
            else if (name == QLatin1String("description"))
                repository.description = reader.readElementText();
            else if (name == QLatin1String("push_url"))
                repository.pushUrl = reader.readElementText();
            else if (name == QLatin1String("clone_url"))
                repository.cloneUrl = reader.readElementText();
            else if (name == QLatin1String("namespace"))
                repository.type = repositoryType(reader.readElementText());
            else
                readUnknownElement(reader);
        }
    }
    return repository;
}

void GitoriousProjectReader::readUnknownElement(QXmlStreamReader &reader)
{
    QTC_ASSERT(reader.isStartElement(), return);

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isEndElement())
            break;

        if (reader.isStartElement())
            readUnknownElement(reader);
    }
}

// --- Gitorious

Gitorious::Gitorious() :
    m_networkManager(0)
{
}

Gitorious &Gitorious::instance()
{
    static Gitorious gitorious;
    return gitorious;
}

void Gitorious::emitError(const QString &e)
{
    qWarning("%s\n", qPrintable(e));
    emit error(e);
}

void Gitorious::addHost(const QString &addr, const QString &description)
{
    addHost(GitoriousHost(addr, description));
}

void Gitorious::addHost(const GitoriousHost &host)
{
    if (debug)
        qDebug() << host;
    const int index = m_hosts.size();
    m_hosts.push_back(host);
    if (host.categories.empty()) {
        updateCategories(index);
        m_hosts.back().state = GitoriousHost::ProjectsQueryRunning;
    } else {
        m_hosts.back().state = GitoriousHost::ProjectsComplete;
    }
    if (host.projects.empty())
        updateProjectList(index);
    emit hostAdded(index);
}

void Gitorious::removeAt(int index)
{
    m_hosts.removeAt(index);
    emit hostRemoved(index);
}

int Gitorious::findByHostName(const QString &hostName) const
{
    const int size = m_hosts.size();
    for (int i = 0; i < size; i++)
        if (m_hosts.at(i).hostName == hostName)
            return i;
    return -1;
}

void Gitorious::setHostDescription(int index, const QString &s)
{
    m_hosts[index].description = s;
}

QString Gitorious::hostDescription(int index) const
{
    return m_hosts.at(index).description;
}

void Gitorious::listCategoriesReply(int index, QByteArray dataB)
{
    /* For now, parse the HTML of the projects site for "Popular Categories":
     * \code
     * <h4>Popular Categories:</h4>
     *  <ul class="...">
     * <li class="..."><a href="..."><category></a> </li>
     * \endcode */
    do {
        const int catIndex = dataB.indexOf("Popular Categories:");
        const int endIndex = catIndex != -1 ? dataB.indexOf("</ul>", catIndex) : -1;
        if (debug)
            qDebug() << "listCategoriesReply cat pos=" << catIndex << endIndex;
        if (endIndex == -1)
            break;
        dataB.truncate(endIndex);
        dataB.remove(0, catIndex);
        const QString data = QString::fromUtf8(dataB);
        // Cut out the contents of the anchors
        QRegExp pattern = QRegExp(QLatin1String("<a href=[^>]+>([^<]+)</a>"));
        QTC_CHECK(pattern.isValid());
        GitoriousHost::CategoryList &categories = m_hosts[index].categories;
        for (int pos = pattern.indexIn(data) ; pos != -1; ) {
            const QString cat = pattern.cap(1);
            categories.push_back(QSharedPointer<GitoriousCategory>(new GitoriousCategory(cat)));
            pos = pattern.indexIn(data, pos + pattern.matchedLength());
        }
    } while (false);

    emit categoryListReceived(index);
}

void Gitorious::listProjectsReply(int hostIndex, int page, const QByteArray &data)
{
    // Receive projects.
    QString errorMessage;
    GitoriousCategory::ProjectList projects = GitoriousProjectReader().read(data, &errorMessage);

    if (debug) {
        qDebug() << "listProjectsReply" << hostName(hostIndex)
                << "page=" << page << " got" << projects.size();
        if (debug > 1)
            qDebug() << '\n' <<data;
    }

    if (!errorMessage.isEmpty()) {
        emitError(tr("Error parsing reply from '%1': %2").arg(hostName(hostIndex), errorMessage));
        if (projects.empty())
            m_hosts[hostIndex].state = GitoriousHost::Error;
    }

    // Add the projects and start next request if 20 projects received
    GitoriousCategory::ProjectList &hostProjects = m_hosts[hostIndex].projects;
    if (!projects.empty())
        hostProjects.append(projects);

    if (projects.size() == ProjectsPageSize) {
        startProjectsRequest(hostIndex, page + 1);
        emit projectListPageReceived(hostIndex, page);
    } else {
        // We are done
        m_hosts[hostIndex].state = GitoriousHost::ProjectsComplete;
        emit projectListReceived(hostIndex);
    }
}

static inline int replyPage(const QNetworkReply *reply)
{ return reply->property(pagePropertyC).toInt(); }

void Gitorious::slotReplyFinished()
{
    // Dispatch the answers via dynamic properties
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        const int protocol = reply->property(protocolPropertyC).toInt();
        // Locate host by name (in case one was deleted in the meantime)
        const QString hostName = reply->property(hostNamePropertyC).toString();
        const int hostIndex = findByHostName(hostName);
        if (hostIndex == -1) // Entry deleted in-between?
            return;
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray data = reply->readAll();
            switch (protocol) {
                case ListProjectsProtocol:
                    listProjectsReply(hostIndex, replyPage(reply), data);
                    break;
                case ListCategoriesProtocol:
                    listCategoriesReply(hostIndex, data);
                    break;

            } // switch protocol
        } else {
            const QString msg = tr("Request failed for '%1': %2").arg(m_hosts.at(hostIndex).hostName, reply->errorString());
            emitError(msg);
        }
        reply->deleteLater();
    }
}

// Create a network request. Set dynamic properties on it to be able to
// dispatch. Use host name in case an entry is removed in-between
QNetworkReply *Gitorious::createRequest(const QUrl &url, int protocol, int hostIndex, int page)
{
    if (!m_networkManager)
        m_networkManager = new Utils::NetworkAccessManager(this);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(slotReplyFinished()));
    reply->setProperty(protocolPropertyC, QVariant(protocol));
    reply->setProperty(hostNamePropertyC, QVariant(hostName(hostIndex)));
    if (page >= 0)
        reply->setProperty(pagePropertyC, QVariant(page));
    if (debug)
        qDebug() << "createRequest" << url;
    return reply;
}

void Gitorious::updateCategories(int index)
{
    // For now, parse the HTML of the projects site for "Popular Categories":
    const QUrl url = httpRequest(hostName(index), QLatin1String("projects"));
    createRequest(url, ListCategoriesProtocol, index);
}

void Gitorious::updateProjectList(int hostIndex)
{
    startProjectsRequest(hostIndex);
}

void Gitorious::startProjectsRequest(int hostIndex, int page)
{
    QUrl url = httpRequest(hostName(hostIndex), QLatin1String("projects"));
    url.addQueryItem(QLatin1String("format"), QLatin1String("xml"));
    if (page >= 0)
        url.addQueryItem(QLatin1String("page"), QString::number(page));
    createRequest(url, ListProjectsProtocol, hostIndex, page);
}

// Serialize hosts/descriptions as a list of "<host>|descr".
void Gitorious::saveSettings(const QString &group, QSettings *s)
{
    const QChar separator = QLatin1Char('|');
    QStringList hosts;
    foreach (const GitoriousHost &h, m_hosts) {
        QString entry = h.hostName;
        if (!h.description.isEmpty()) {
            entry += separator;
            entry += h.description;
        }
        hosts.push_back(entry);
    }
    s->beginGroup(group);
    s->setValue(QLatin1String(settingsKeyC), hosts);
    s->endGroup();
}

void Gitorious::restoreSettings(const QString &group, const QSettings *s)
{
    m_hosts.clear();
    const QChar separator = QLatin1Char('|');
    const QStringList hosts = s->value(group + QLatin1Char('/') + QLatin1String(settingsKeyC), QStringList()).toStringList();
    foreach (const QString &h, hosts) {
        const int sepPos = h.indexOf(separator);
        if (sepPos == -1)
            addHost(GitoriousHost(h));
        else
            addHost(GitoriousHost(h.mid(0, sepPos), h.mid(sepPos + 1)));
    }
}

GitoriousHost Gitorious::gitoriousOrg()
{
    return GitoriousHost(QLatin1String("gitorious.org"), tr("Open source projects that use Git."));
}

} // namespace Internal
} // namespace Gitorious
