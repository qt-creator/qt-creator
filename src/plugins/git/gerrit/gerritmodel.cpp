/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "gerritmodel.h"
#include "../gitplugin.h"
#include "../gitclient.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <vcsbase/vcsoutputwindow.h>

#include <utils/algorithm.h>
#include <utils/asconst.h>
#include <utils/synchronousprocess.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <QProcess>
#include <QVariant>
#include <QTextStream>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QScopedPointer>
#include <QTimer>
#include <QApplication>
#include <QFutureWatcher>
#include <QUrl>

enum { debug = 0 };

using namespace VcsBase;

namespace Gerrit {
namespace Internal {

QDebug operator<<(QDebug d, const GerritApproval &a)
{
    d.nospace() << a.reviewer.fullName << ": " << a.approval << " ("
                << a.type << ", " << a.description << ')';
    return d;
}

// Sort approvals by type and reviewer
bool gerritApprovalLessThan(const GerritApproval &a1, const GerritApproval &a2)
{
    return a1.type.compare(a2.type) < 0 || a1.reviewer.fullName.compare(a2.reviewer.fullName) < 0;
}

QDebug operator<<(QDebug d, const GerritPatchSet &p)
{
    d.nospace() << " Patch set: " << p.ref << ' ' << p.patchSetNumber
                << ' ' << p.approvals;
    return d;
}

QDebug operator<<(QDebug d, const GerritChange &c)
{
    d.nospace() << c.fullTitle() << " by " << c.owner.email
                << ' ' << c.lastUpdated << ' ' <<  c.currentPatchSet;
    return d;
}

// Format default Url for a change
static inline QString defaultUrl(const QSharedPointer<GerritParameters> &p,
                                 const GerritServer &server,
                                 int gerritNumber)
{
    QString result = QLatin1String(p->https ? "https://" : "http://");
    result += server.host;
    result += '/';
    result += QString::number(gerritNumber);
    return result;
}

// Format (sorted) approvals as separate HTML table
// lines by type listing the revievers:
// "<tr><td>Code Review</td><td>John Doe: -1, ...</tr><tr>...Sanity Review: ...".
QString GerritPatchSet::approvalsToHtml() const
{
    if (approvals.isEmpty())
        return QString();

    QString result;
    QTextStream str(&result);
    QString lastType;
    for (const GerritApproval &a : approvals) {
        if (a.type != lastType) {
            if (!lastType.isEmpty())
                str << "</tr>\n";
            str << "<tr><td>"
                << (a.description.isEmpty() ? a.type : a.description)
                << "</td><td>";
            lastType = a.type;
        } else {
            str << ", ";
        }
        str << a.reviewer.fullName;
        if (!a.reviewer.email.isEmpty())
            str << " <a href=\"mailto:" << a.reviewer.email << "\">" << a.reviewer.email << "</a>";
        str << ": " << forcesign << a.approval << noforcesign;
    }
    str << "</tr>\n";
    return result;
}

// Determine total approval level. Negative values take preference
// and stay.
static inline void applyApproval(int approval, int *total)
{
    if (approval < *total || (*total >= 0 && approval > *total))
        *total = approval;
}

// Format the approvals similar to the columns in the Web view
// by a type character followed by the approval level: "C: -2, S: 1"
QString GerritPatchSet::approvalsColumn() const
{
    typedef QMap<QChar, int> TypeReviewMap;
    typedef TypeReviewMap::iterator TypeReviewMapIterator;
    typedef TypeReviewMap::const_iterator TypeReviewMapConstIterator;

    QString result;
    if (approvals.isEmpty())
        return result;

    TypeReviewMap reviews; // Sort approvals into a map by type character
    for (const GerritApproval &a : approvals) {
        if (a.type != "STGN") { // Qt-Project specific: Ignore "STGN" (Staged)
            const QChar typeChar = a.type.at(0);
            TypeReviewMapIterator it = reviews.find(typeChar);
            if (it == reviews.end())
                it = reviews.insert(typeChar, 0);
            applyApproval(a.approval, &it.value());
        }
    }

    QTextStream str(&result);
    const TypeReviewMapConstIterator cend = reviews.constEnd();
    for (TypeReviewMapConstIterator it = reviews.constBegin(); it != cend; ++it) {
        if (!result.isEmpty())
            str << ' ';
        str << it.key() << ": " << forcesign << it.value() << noforcesign;
    }
    return result;
}

bool GerritPatchSet::hasApproval(const GerritUser &user) const
{
    return Utils::contains(approvals, [&user](const GerritApproval &a) {
        return a.reviewer.isSameAs(user);
    });
}

int GerritPatchSet::approvalLevel() const
{
    int value = 0;
    for (const GerritApproval &a : approvals)
        applyApproval(a.approval, &value);
    return value;
}

QString GerritChange::filterString() const
{
    const QChar blank = ' ';
    QString result = QString::number(number) + blank + title + blank
            + owner.fullName + blank + project + blank
            + branch + blank + status;
    for (const GerritApproval &a : currentPatchSet.approvals) {
        result += blank;
        result += a.reviewer.fullName;
    }
    return result;
}

QStringList GerritChange::gitFetchArguments(const GerritServer &server) const
{
    const QString url = currentPatchSet.url.isEmpty()
            ? server.url(GerritServer::UrlWithHttpUser) + '/' + project
            : currentPatchSet.url;
    return {"fetch", url, currentPatchSet.ref};
}

QString GerritChange::fullTitle() const
{
    QString res = title;
    if (status == "DRAFT")
        res += GerritModel::tr(" (Draft)");
    return res;
}

// Helper class that runs ssh gerrit queries from a list of query argument
// string lists,
// see http://gerrit.googlecode.com/svn/documentation/2.1.5/cmd-query.html
// In theory, querying uses a continuation/limit protocol, but we assume
// we will never reach a limit with those queries.

class QueryContext : public QObject {
    Q_OBJECT
public:
    QueryContext(const QString &query,
                 const QSharedPointer<GerritParameters> &p,
                 const GerritServer &server,
                 QObject *parent = nullptr);

    ~QueryContext();
    void start();

signals:
    void resultRetrieved(const QByteArray &);
    void errorText(const QString &text);
    void finished();

private:
    void processError(QProcess::ProcessError);
    void processFinished(int exitCode, QProcess::ExitStatus);
    void timeout();

    void errorTermination(const QString &msg);
    void terminate();

    QProcess m_process;
    QTimer m_timer;
    QString m_binary;
    QByteArray m_output;
    QString m_error;
    QFutureInterface<void> m_progress;
    QFutureWatcher<void> m_watcher;
    QStringList m_arguments;
};

enum { timeOutMS = 30000 };

QueryContext::QueryContext(const QString &query,
                           const QSharedPointer<GerritParameters> &p,
                           const GerritServer &server,
                           QObject *parent)
    : QObject(parent)
{
    if (server.type == GerritServer::Ssh) {
        m_binary = p->ssh;
        if (server.port)
            m_arguments << p->portFlag << QString::number(server.port);
        m_arguments << server.hostArgument() << "gerrit"
                    << "query" << "--dependencies"
                    << "--current-patch-set"
                    << "--format=JSON" << query;
    } else {
        m_binary = p->curl;
        const QString url = server.url(GerritServer::RestUrl) + "/changes/?q="
                + QString::fromUtf8(QUrl::toPercentEncoding(query))
                + "&o=CURRENT_REVISION&o=DETAILED_LABELS&o=DETAILED_ACCOUNTS";
        m_arguments = GerritServer::curlArguments() << url;
    }
    connect(&m_process, &QProcess::readyReadStandardError, this, [this] {
        const QString text = QString::fromLocal8Bit(m_process.readAllStandardError());
        VcsOutputWindow::appendError(text);
        m_error.append(text);
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_output.append(m_process.readAllStandardOutput());
    });
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &QueryContext::processFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &QueryContext::processError);
    connect(&m_watcher, &QFutureWatcherBase::canceled, this, &QueryContext::terminate);
    m_watcher.setFuture(m_progress.future());
    m_process.setProcessEnvironment(Git::Internal::GitPlugin::client()->processEnvironment());
    m_progress.setProgressRange(0, 1);

    m_timer.setInterval(timeOutMS);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &QueryContext::timeout);
}

QueryContext::~QueryContext()
{
    if (m_progress.isRunning())
        m_progress.reportFinished();
    if (m_timer.isActive())
        m_timer.stop();
    m_process.disconnect(this);
    terminate();
}

void QueryContext::start()
{
    Core::FutureProgress *fp = Core::ProgressManager::addTask(m_progress.future(), tr("Querying Gerrit"),
                                           "gerrit-query");
    fp->setKeepOnFinish(Core::FutureProgress::HideOnFinish);
    m_progress.reportStarted();
    // Order: synchronous call to error handling if something goes wrong.
    VcsOutputWindow::appendCommand(
                m_process.workingDirectory(), Utils::FileName::fromString(m_binary), m_arguments);
    m_timer.start();
    m_process.start(m_binary, m_arguments);
    m_process.closeWriteChannel();
}

void QueryContext::errorTermination(const QString &msg)
{
    if (!m_progress.isCanceled())
        VcsOutputWindow::appendError(msg);
    m_progress.reportCanceled();
    m_progress.reportFinished();
    emit finished();
}

void QueryContext::terminate()
{
    Utils::SynchronousProcess::stopProcess(m_process);
}

void QueryContext::processError(QProcess::ProcessError e)
{
    const QString msg = tr("Error running %1: %2").arg(m_binary, m_process.errorString());
    if (e == QProcess::FailedToStart)
        errorTermination(msg);
    else
        VcsOutputWindow::appendError(msg);
}

void QueryContext::processFinished(int exitCode, QProcess::ExitStatus es)
{
    if (m_timer.isActive())
        m_timer.stop();
    emit errorText(m_error);
    if (es != QProcess::NormalExit) {
        errorTermination(tr("%1 crashed.").arg(m_binary));
        return;
    } else if (exitCode) {
        errorTermination(tr("%1 returned %2.").arg(m_binary).arg(exitCode));
        return;
    }
    emit resultRetrieved(m_output);
    m_progress.reportFinished();
    emit finished();
}

void QueryContext::timeout()
{
    if (m_process.state() != QProcess::Running)
        return;

    QWidget *parent = QApplication::activeModalWidget();
    if (!parent)
        parent = QApplication::activeWindow();
    QMessageBox box(QMessageBox::Question, tr("Timeout"),
                    tr("The gerrit process has not responded within %1 s.\n"
                       "Most likely this is caused by problems with SSH authentication.\n"
                       "Would you like to terminate it?").
                    arg(timeOutMS / 1000), QMessageBox::NoButton, parent);
    QPushButton *terminateButton = box.addButton(tr("Terminate"), QMessageBox::YesRole);
    box.addButton(tr("Keep Running"), QMessageBox::NoRole);
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            &box, &QDialog::reject);
    box.exec();
    if (m_process.state() != QProcess::Running)
        return;
    if (box.clickedButton() == terminateButton)
        terminate();
    else
        m_timer.start();
}

GerritModel::GerritModel(const QSharedPointer<GerritParameters> &p, QObject *parent)
    : QStandardItemModel(0, ColumnCount, parent)
    , m_parameters(p)
{
    QStringList headers; // Keep in sync with GerritChange::toHtml()
    headers << "#" << tr("Subject") << tr("Owner")
            << tr("Updated") << tr("Project")
            << tr("Approvals") << tr("Status");
    setHorizontalHeaderLabels(headers);
}

GerritModel::~GerritModel()
{ }

QVariant GerritModel::data(const QModelIndex &index, int role) const
{
    QVariant value = QStandardItemModel::data(index, role);
    if (role == SortRole && value.isNull())
        return QStandardItemModel::data(index, Qt::DisplayRole);
    return value;
}

static inline GerritChangePtr changeFromItem(const QStandardItem *item)
{
    return qvariant_cast<GerritChangePtr>(item->data(GerritModel::GerritChangeRole));
}

GerritChangePtr GerritModel::change(const QModelIndex &index) const
{
    if (index.isValid())
        return changeFromItem(itemFromIndex(index));
    return GerritChangePtr(new GerritChange);
}

QString GerritModel::dependencyHtml(const QString &header, const int changeNumber,
                                    const QString &serverPrefix) const
{
    QString res;
    if (!changeNumber)
        return res;
    QTextStream str(&res);
    str << "<tr><td>" << header << "</td><td><a href="
        << serverPrefix << "r/" << changeNumber << '>' << changeNumber << "</a>";
    if (const QStandardItem *item = itemForNumber(changeNumber))
        str << " (" << changeFromItem(item)->fullTitle() << ')';
    str << "</td></tr>";
    return res;
}

QString GerritModel::toHtml(const QModelIndex& index) const
{
    static const QString subjectHeader = GerritModel::tr("Subject");
    static const QString numberHeader = GerritModel::tr("Number");
    static const QString ownerHeader = GerritModel::tr("Owner");
    static const QString projectHeader = GerritModel::tr("Project");
    static const QString statusHeader = GerritModel::tr("Status");
    static const QString patchSetHeader = GerritModel::tr("Patch set");
    static const QString urlHeader = GerritModel::tr("URL");
    static const QString dependsOnHeader = GerritModel::tr("Depends on");
    static const QString neededByHeader = GerritModel::tr("Needed by");

    if (!index.isValid())
        return QString();
    const GerritChangePtr c = change(index);
    const QString serverPrefix = c->url.left(c->url.lastIndexOf('/') + 1);
    QString result;
    QTextStream str(&result);
    str << "<html><head/><body><table>"
        << "<tr><td>" << subjectHeader << "</td><td>" << c->fullTitle() << "</td></tr>"
        << "<tr><td>" << numberHeader << "</td><td><a href=\"" << c->url << "\">" << c->number << "</a></td></tr>"
        << "<tr><td>" << ownerHeader << "</td><td>" << c->owner.fullName << ' '
        << "<a href=\"mailto:" << c->owner.email << "\">" << c->owner.email << "</a></td></tr>"
        << "<tr><td>" << projectHeader << "</td><td>" << c->project << " (" << c->branch << ")</td></tr>"
        << dependencyHtml(dependsOnHeader, c->dependsOnNumber, serverPrefix)
        << dependencyHtml(neededByHeader, c->neededByNumber, serverPrefix)
        << "<tr><td>" << statusHeader << "</td><td>" << c->status
        << ", " << c->lastUpdated.toString(Qt::DefaultLocaleShortDate) << "</td></tr>"
        << "<tr><td>" << patchSetHeader << "</td><td>" << "</td></tr>" << c->currentPatchSet.patchSetNumber << "</td></tr>"
        << c->currentPatchSet.approvalsToHtml()
        << "<tr><td>" << urlHeader << "</td><td><a href=\"" << c->url << "\">" << c->url << "</a></td></tr>"
        << "</table></body></html>";
    return result;
}

static QStandardItem *numberSearchRecursion(QStandardItem *item, int number)
{
    if (changeFromItem(item)->number == number)
        return item;
    const int rowCount = item->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        if (QStandardItem *i = numberSearchRecursion(item->child(r, 0), number))
            return i;
    }
    return 0;
}

QStandardItem *GerritModel::itemForNumber(int number) const
{
    if (!number)
        return 0;
    const int numRows = rowCount();
    for (int r = 0; r < numRows; ++r) {
        if (QStandardItem *i = numberSearchRecursion(item(r, 0), number))
            return i;
    }
    return 0;
}

void GerritModel::refresh(const QSharedPointer<GerritServer> &server, const QString &query)
{
    if (m_query) {
        qWarning("%s: Another query is still running", Q_FUNC_INFO);
        return;
    }
    clearData();
    m_server = server;

    QString realQuery = query.trimmed();
    if (realQuery.isEmpty()) {
        realQuery = "status:open";
        const QString user = m_server->user.userName;
        if (!user.isEmpty())
            realQuery += QString(" (owner:%1 OR reviewer:%1)").arg(user);
    }

    m_query = new QueryContext(realQuery, m_parameters, *m_server, this);
    connect(m_query, &QueryContext::resultRetrieved, this, &GerritModel::resultRetrieved);
    connect(m_query, &QueryContext::errorText, this, &GerritModel::errorText);
    connect(m_query, &QueryContext::finished, this, &GerritModel::queryFinished);
    emit refreshStateChanged(true);
    m_query->start();
    setState(Running);
}

void GerritModel::clearData()
{
    if (const int rows = rowCount())
        removeRows(0, rows);
}

void GerritModel::setState(GerritModel::QueryState s)
{
    if (s == m_state)
        return;
    m_state = s;
    emit stateChanged();
}

// {"name":"Hans Mustermann","email":"hm@acme.com","username":"hansm"}
static GerritUser parseGerritUser(const QJsonObject &object)
{
    GerritUser user;
    user.userName = object.value("username").toString();
    user.fullName = object.value("name").toString();
    user.email = object.value("email").toString();
    return user;
}

static int numberValue(const QJsonObject &object)
{
    return object.value("number").toString().toInt();
}

/* Parse gerrit query Json output.
 * See http://gerrit.googlecode.com/svn/documentation/2.1.5/cmd-query.html
 * Note: The url will be present only if  "canonicalWebUrl" is configured
 * in gerrit.config.
\code
{"project":"qt/qtbase","branch":"master","id":"I6601ca68c427b909680423ae81802f1ed5cd178a",
"number":"24143","subject":"bla","owner":{"name":"Hans Mustermann","email":"hm@acme.com"},
"url":"https://...","lastUpdated":1335127888,"sortKey":"001c8fc300005e4f",
"open":true,"status":"NEW","currentPatchSet":
  {"number":"1","revision":"0a1e40c78ef16f7652472f4b4bb4c0addeafbf82",
   "ref":"refs/changes/43/24143/1",
   "uploader":{"name":"Hans Mustermann","email":"hm@acme.com"},
   "approvals":[{"type":"SRVW","description":"Sanity Review","value":"1",
                 "grantedOn":1335127888,"by":{
                 "name":"Qt Sanity Bot","email":"qt_sanity_bot@ovi.com"}}]}}
\endcode
*/

static GerritChangePtr parseSshOutput(const QJsonObject &object)
{
    GerritChangePtr change(new GerritChange);
    // Read current patch set.
    const QJsonObject patchSet = object.value("currentPatchSet").toObject();
    change->currentPatchSet.patchSetNumber = qMax(1, numberValue(patchSet));
    change->currentPatchSet.ref = patchSet.value("ref").toString();
    const QJsonArray approvalsJ = patchSet.value("approvals").toArray();
    const int ac = approvalsJ.size();
    for (int a = 0; a < ac; ++a) {
        const QJsonObject ao = approvalsJ.at(a).toObject();
        GerritApproval approval;
        approval.reviewer = parseGerritUser(ao.value("by").toObject());
        approval.approval = ao.value("value").toString().toInt();
        approval.type = ao.value("type").toString();
        approval.description = ao.value("description").toString();
        change->currentPatchSet.approvals.push_back(approval);
    }
    std::stable_sort(change->currentPatchSet.approvals.begin(),
                     change->currentPatchSet.approvals.end(),
                     gerritApprovalLessThan);
    // Remaining
    change->number = numberValue(object);
    change->url = object.value("url").toString();
    change->title = object.value("subject").toString();
    change->owner = parseGerritUser(object.value("owner").toObject());
    change->project = object.value("project").toString();
    change->branch = object.value("branch").toString();
    change->status =  object.value("status").toString();
    if (const int timeT = object.value("lastUpdated").toInt())
        change->lastUpdated = QDateTime::fromTime_t(uint(timeT));
    // Read out dependencies
    const QJsonValue dependsOnValue = object.value("dependsOn");
    if (dependsOnValue.isArray()) {
        const QJsonArray dependsOnArray = dependsOnValue.toArray();
        if (!dependsOnArray.isEmpty()) {
            const QJsonValue first = dependsOnArray.at(0);
            if (first.isObject())
                change->dependsOnNumber = numberValue(first.toObject());
        }
    }
    // Read out needed by
    const QJsonValue neededByValue = object.value("neededBy");
    if (neededByValue.isArray()) {
        const QJsonArray neededByArray = neededByValue.toArray();
        if (!neededByArray.isEmpty()) {
            const QJsonValue first = neededByArray.at(0);
            if (first.isObject())
                change->neededByNumber = numberValue(first.toObject());
        }
    }
    return change;
}

/*
  {
    "kind": "gerritcodereview#change",
    "id": "qt-creator%2Fqt-creator~master~Icc164b9d84abe4efc34deaa5d19dca167fdb14e1",
    "project": "qt-creator/qt-creator",
    "branch": "master",
    "change_id": "Icc164b9d84abe4efc34deaa5d19dca167fdb14e1",
    "subject": "WIP: Gerrit: Support REST query for HTTP servers",
    "status": "NEW",
    "created": "2017-02-22 21:23:39.403000000",
    "updated": "2017-02-23 21:03:51.055000000",
    "reviewed": true,
    "mergeable": false,
    "_sortkey": "004368cf0002d84f",
    "_number": 186447,
    "owner": {
      "_account_id": 1000534,
      "name": "Orgad Shaneh",
      "email": "orgads@gmail.com"
    },
    "labels": {
      "Code-Review": {
        "all": [
          {
            "value": 0,
            "_account_id": 1000009,
            "name": "Tobias Hunger",
            "email": "tobias.hunger@qt.io"
          },
          {
            "value": 0,
            "_account_id": 1000528,
            "name": "André Hartmann",
            "email": "aha_1980@gmx.de"
          },
          {
            "value": 0,
            "_account_id": 1000049,
            "name": "Qt Sanity Bot",
            "email": "qt_sanitybot@qt-project.org"
          }
        ],
        "values": {
          "-2": "This shall not be merged",
          "-1": "I would prefer this is not merged as is",
          " 0": "No score",
          "+1": "Looks good to me, but someone else must approve",
          "+2": "Looks good to me, approved"
        }
      },
      "Sanity-Review": {
        "all": [
          {
            "value": 0,
            "_account_id": 1000009,
            "name": "Tobias Hunger",
            "email": "tobias.hunger@qt.io"
          },
          {
            "value": 0,
            "_account_id": 1000528,
            "name": "André Hartmann",
            "email": "aha_1980@gmx.de"
          },
          {
            "value": 1,
            "_account_id": 1000049,
            "name": "Qt Sanity Bot",
            "email": "qt_sanitybot@qt-project.org"
          }
        ],
        "values": {
          "-2": "Major sanity problems found",
          "-1": "Sanity problems found",
          " 0": "No sanity review",
          "+1": "Sanity review passed"
        }
      }
    },
    "permitted_labels": {
      "Code-Review": [
        "-2",
        "-1",
        " 0",
        "+1",
        "+2"
      ],
      "Sanity-Review": [
        "-2",
        "-1",
        " 0",
        "+1"
      ]
    },
    "current_revision": "87916545e2974913d56f56c9f06fc3822a876aca",
    "revisions": {
      "87916545e2974913d56f56c9f06fc3822a876aca": {
        "draft": true,
        "_number": 2,
        "fetch": {
          "http": {
            "url": "https://codereview.qt-project.org/qt-creator/qt-creator",
            "ref": "refs/changes/47/186447/2"
          },
          "ssh": {
            "url": "ssh:// *:29418/qt-creator/qt-creator",
            "ref": "refs/changes/47/186447/2"
          }
        }
      }
    }
  }
*/

static int restNumberValue(const QJsonObject &object)
{
    return object.value("_number").toInt();
}

static GerritChangePtr parseRestOutput(const QJsonObject &object, const GerritServer &server)
{
    GerritChangePtr change(new GerritChange);
    change->number = restNumberValue(object);
    change->url = QString("%1/%2").arg(server.url()).arg(change->number);
    change->title = object.value("subject").toString();
    change->owner = parseGerritUser(object.value("owner").toObject());
    change->project = object.value("project").toString();
    change->branch = object.value("branch").toString();
    change->status =  object.value("status").toString();
    change->lastUpdated = QDateTime::fromString(object.value("updated").toString() + "Z",
                                                Qt::DateFormat::ISODate).toLocalTime();
    // Read current patch set.
    const QJsonObject patchSet = object.value("revisions").toObject().begin().value().toObject();
    change->currentPatchSet.patchSetNumber = qMax(1, restNumberValue(patchSet));
    const QJsonObject fetchInfo = patchSet.value("fetch").toObject().value("http").toObject();
    change->currentPatchSet.ref = fetchInfo.value("ref").toString();
    // Replace * in ssh://*:29418/qt-creator/qt-creator with the hostname
    change->currentPatchSet.url = fetchInfo.value("url").toString().replace('*', server.host);
    const QJsonObject labels = object.value("labels").toObject();
    for (auto it = labels.constBegin(), end = labels.constEnd(); it != end; ++it) {
        const QJsonArray all = it.value().toObject().value("all").toArray();
        for (const QJsonValue &av : all) {
            const QJsonObject ao = av.toObject();
            const int value = ao.value("value").toInt();
            if (!value)
                continue;
            GerritApproval approval;
            approval.reviewer = parseGerritUser(ao);
            approval.approval = value;
            approval.type = it.key();
            change->currentPatchSet.approvals.push_back(approval);
        }
    }
    std::stable_sort(change->currentPatchSet.approvals.begin(),
                     change->currentPatchSet.approvals.end(),
                     gerritApprovalLessThan);
    return change;
}

static bool parseOutput(const QSharedPointer<GerritParameters> &parameters,
                        const GerritServer &server,
                        const QByteArray &output,
                        QList<GerritChangePtr> &result)
{
    QByteArray adaptedOutput;
    if (server.type == GerritServer::Ssh) {
        // The output consists of separate lines containing a document each
        // Add a comma after each line (except the last), and enclose it as an array
        adaptedOutput = '[' + output + ']';
        adaptedOutput.replace('\n', ',');
        const int lastComma = adaptedOutput.lastIndexOf(',');
        if (lastComma >= 0)
            adaptedOutput[lastComma] = '\n';
    } else {
        adaptedOutput = output;
        // Strip first line, which is )]}'
        adaptedOutput.remove(0, adaptedOutput.indexOf("\n"));
    }
    bool res = true;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(adaptedOutput, &error);
    if (doc.isNull()) {
        QString errorMessage = GerritModel::tr("Parse error: \"%1\" -> %2")
                .arg(QString::fromUtf8(output))
                .arg(error.errorString());
        qWarning() << errorMessage;
        VcsOutputWindow::appendError(errorMessage);
        res = false;
    }
    const QJsonArray rootArray = doc.array();
    result.clear();
    result.reserve(rootArray.count());
    for (const QJsonValue &value : rootArray) {
        const QJsonObject object = value.toObject();
        // Skip stats line: {"type":"stats","rowCount":9,"runTimeMilliseconds":13}
        if (object.contains("type"))
            continue;
        GerritChangePtr change =
                (server.type == GerritServer::Ssh ? parseSshOutput(object)
                                                  : parseRestOutput(object, server));
        if (change->isValid()) {
            if (change->url.isEmpty()) //  No "canonicalWebUrl" is in gerrit.config.
                change->url = defaultUrl(parameters, server, change->number);
            result.push_back(change);
        } else {
            const QByteArray jsonObject = QJsonDocument(object).toJson();
            qWarning("%s: Parse error: '%s'.", Q_FUNC_INFO, jsonObject.constData());
            VcsOutputWindow::appendError(GerritModel::tr("Parse error: \"%1\"")
                                  .arg(QString::fromUtf8(jsonObject)));
            res = false;
        }
    }
    return res;
}

QList<QStandardItem *> GerritModel::changeToRow(const GerritChangePtr &c) const
{
    QList<QStandardItem *> row;
    const QVariant filterV = QVariant(c->filterString());
    const QVariant changeV = qVariantFromValue(c);
    for (int i = 0; i < GerritModel::ColumnCount; ++i) {
        QStandardItem *item = new QStandardItem;
        item->setData(changeV, GerritModel::GerritChangeRole);
        item->setData(filterV, GerritModel::FilterRole);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        row.append(item);
    }
    row[NumberColumn]->setData(c->number, Qt::DisplayRole);
    row[TitleColumn]->setText(c->fullTitle());
    row[OwnerColumn]->setText(c->owner.fullName);
    // Shorten columns: Display time if it is today, else date
    const QString dateString = c->lastUpdated.date() == QDate::currentDate() ?
                c->lastUpdated.time().toString(Qt::SystemLocaleShortDate) :
                c->lastUpdated.date().toString(Qt::SystemLocaleShortDate);
    row[DateColumn]->setData(dateString, Qt::DisplayRole);
    row[DateColumn]->setData(c->lastUpdated, SortRole);

    QString project = c->project;
    if (c->branch != "master")
        project += " (" + c->branch  + ')';
    row[ProjectColumn]->setText(project);
    row[StatusColumn]->setText(c->status);
    row[ApprovalsColumn]->setText(c->currentPatchSet.approvalsColumn());
    // Mark changes awaiting action using a bold font.
    bool bold = false;
    if (c->owner.isSameAs(m_server->user)) { // Owned changes: Review != 0,1. Submit or amend.
        const int level = c->currentPatchSet.approvalLevel();
        bold = level != 0 && level != 1;
    } else { // Changes pending for review: No review yet.
        bold = !c->currentPatchSet.hasApproval(m_server->user);
    }
    if (bold) {
        QFont font = row.first()->font();
        font.setBold(true);
        for (int i = 0; i < GerritModel::ColumnCount; ++i)
            row[i]->setFont(font);
    }

    return row;
}

bool gerritChangeLessThan(const GerritChangePtr &c1, const GerritChangePtr &c2)
{
    if (c1->depth != c2->depth)
        return c1->depth < c2->depth;
    return c1->lastUpdated > c2->lastUpdated;
}

void GerritModel::resultRetrieved(const QByteArray &output)
{
    QList<GerritChangePtr> changes;
    setState(parseOutput(m_parameters, *m_server, output, changes) ? Ok : Error);

    // Populate a hash with indices for faster access.
    QHash<int, int> numberIndexHash;
    const int count = changes.size();
    for (int i = 0; i < count; ++i)
        numberIndexHash.insert(changes.at(i)->number, i);
    // Mark root nodes: Changes that do not have a dependency, depend on a change
    // not in the list or on a change that is not "NEW".
    for (int i = 0; i < count; ++i) {
        if (!changes.at(i)->dependsOnNumber) {
            changes.at(i)->depth = 0;
        } else {
            const int dependsOnIndex = numberIndexHash.value(changes.at(i)->dependsOnNumber, -1);
            if (dependsOnIndex < 0 || changes.at(dependsOnIndex)->status != "NEW")
                changes.at(i)->depth = 0;
        }
    }
    // Indicate depth of dependent changes by using that of the parent + 1 until no more
    // changes occur.
    for (bool changed = true; changed; ) {
        changed = false;
        for (int i = 0; i < count; ++i) {
            if (changes.at(i)->depth < 0) {
                const int dependsIndex = numberIndexHash.value(changes.at(i)->dependsOnNumber);
                const int dependsOnDepth = changes.at(dependsIndex)->depth;
                if (dependsOnDepth >= 0) {
                    changes.at(i)->depth = dependsOnDepth + 1;
                    changed = true;
                }
            }
        }
    }
    // Sort by depth (root nodes first) and by date.
    std::stable_sort(changes.begin(), changes.end(), gerritChangeLessThan);
    numberIndexHash.clear();

    for (const GerritChangePtr &c : Utils::asConst(changes)) {
        // Avoid duplicate entries for example in the (unlikely)
        // case people do self-reviews.
        if (!itemForNumber(c->number)) {
            const QList<QStandardItem *> newRow = changeToRow(c);
            if (c->depth) {
                QStandardItem *parent = itemForNumber(c->dependsOnNumber);
                // Append changes with depth > 1 to the parent with depth=1 to avoid
                // too-deeply nested items.
                for (; changeFromItem(parent)->depth >= 1; parent = parent->parent()) {}
                parent->appendRow(newRow);
                QString parentFilterString = parent->data(FilterRole).toString();
                parentFilterString += ' ';
                parentFilterString += newRow.first()->data(FilterRole).toString();
                parent->setData(QVariant(parentFilterString), FilterRole);
            } else {
                appendRow(newRow);
            }
        }
    }
}

void GerritModel::queryFinished()
{
    m_query->deleteLater();
    m_query = nullptr;
    setState(Idle);
    emit refreshStateChanged(false);
}

} // namespace Internal
} // namespace Gerrit

#include "gerritmodel.moc"
