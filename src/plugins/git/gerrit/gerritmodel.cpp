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
#include "gerritparameters.h"
#include "../gitplugin.h"
#include "../gitclient.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <vcsbase/vcsoutputwindow.h>

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

enum { debug = 0 };

using namespace VcsBase;

namespace Gerrit {
namespace Internal {

QDebug operator<<(QDebug d, const GerritApproval &a)
{
    d.nospace() << a.reviewer << " :" << a.approval << " ("
                << a.type << ", " << a.description << ')';
    return d;
}

// Sort approvals by type and reviewer
bool gerritApprovalLessThan(const GerritApproval &a1, const GerritApproval &a2)
{
    return a1.type.compare(a2.type) < 0 || a1.reviewer.compare(a2.reviewer) < 0;
}

QDebug operator<<(QDebug d, const GerritPatchSet &p)
{
    d.nospace() << " Patch set: " << p.ref << ' ' << p.patchSetNumber
                << ' ' << p.approvals;
    return d;
}

QDebug operator<<(QDebug d, const GerritChange &c)
{
    d.nospace() << c.fullTitle() << " by " << c.email
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
        str << a.reviewer;
        if (!a.email.isEmpty())
            str << " <a href=\"mailto:" << a.email << "\">" << a.email << "</a>";
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

bool GerritPatchSet::hasApproval(const QString &userName) const
{
    for (const GerritApproval &a : approvals)
        if (a.reviewer == userName)
            return true;
    return false;
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
            + owner + blank + project + blank
            + branch + blank + status;
    for (const GerritApproval &a : currentPatchSet.approvals) {
        result += blank;
        result += a.reviewer;
    }
    return result;
}

QStringList GerritChange::gitFetchArguments(const GerritServer &server) const
{
    return { "fetch", server.url() + '/' + project, currentPatchSet.ref };
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

    int currentQuery() const { return m_currentQuery; }

public slots:
    void start();

signals:
    void resultRetrieved(const QByteArray &);
    void finished();

private:
    void processError(QProcess::ProcessError);
    void processFinished(int exitCode, QProcess::ExitStatus);
    void readyReadStandardError();
    void readyReadStandardOutput();
    void timeout();

    void startQuery(const QString &query);
    void errorTermination(const QString &msg);
    void terminate();

    const QString m_query;
    QProcess m_process;
    QTimer m_timer;
    QString m_binary;
    QByteArray m_output;
    int m_currentQuery;
    QFutureInterface<void> m_progress;
    QFutureWatcher<void> m_watcher;
    QStringList m_baseArguments;
};

enum { timeOutMS = 30000 };

QueryContext::QueryContext(const QString &query,
                           const QSharedPointer<GerritParameters> &p,
                           const GerritServer &server,
                           QObject *parent)
    : QObject(parent)
    , m_query(query)
    , m_currentQuery(0)
{
    m_baseArguments << p->ssh;
    if (server.port)
        m_baseArguments << p->portFlag << QString::number(server.port);
    m_baseArguments << server.sshHostArgument() << "gerrit";
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &QueryContext::readyReadStandardError);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &QueryContext::readyReadStandardOutput);
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &QueryContext::processFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &QueryContext::processError);
    connect(&m_watcher, &QFutureWatcherBase::canceled, this, &QueryContext::terminate);
    m_watcher.setFuture(m_progress.future());
    m_process.setProcessEnvironment(Git::Internal::GitPlugin::client()->processEnvironment());
    m_progress.setProgressRange(0, 1);

    // Determine binary and common command line arguments.
    m_baseArguments << "query" << "--dependencies"
                    << "--current-patch-set"
                    << "--format=JSON";
    m_binary = m_baseArguments.front();
    m_baseArguments.pop_front();

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
    startQuery(m_query); // Order: synchronous call to error handling if something goes wrong.
}

void QueryContext::startQuery(const QString &query)
{
    QStringList arguments = m_baseArguments;
    arguments.push_back(query);
    VcsOutputWindow::appendCommand(
                m_process.workingDirectory(), Utils::FileName::fromString(m_binary), arguments);
    m_timer.start();
    m_process.start(m_binary, arguments);
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

void QueryContext::readyReadStandardError()
{
    VcsOutputWindow::appendError(QString::fromLocal8Bit(m_process.readAllStandardError()));
}

void QueryContext::readyReadStandardOutput()
{
    m_output.append(m_process.readAllStandardOutput());
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
        << "<tr><td>" << ownerHeader << "</td><td>" << c->owner << ' '
        << "<a href=\"mailto:" << c->email << "\">" << c->email << "</a></td></tr>"
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
        const QString user = m_server->user;
        if (!user.isEmpty())
            realQuery += QString(" (owner:%1 OR reviewer:%1)").arg(user);
    }

    m_query = new QueryContext(realQuery, m_parameters, *m_server, this);
    connect(m_query, &QueryContext::resultRetrieved, this, &GerritModel::resultRetrieved);
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

static bool parseOutput(const QSharedPointer<GerritParameters> &parameters,
                        const GerritServer &server,
                        const QByteArray &output,
                        QList<GerritChangePtr> &result)
{
    // The output consists of separate lines containing a document each
    const QString typeKey = "type";
    const QString dependsOnKey = "dependsOn";
    const QString neededByKey = "neededBy";
    const QString branchKey = "branch";
    const QString numberKey = "number";
    const QString ownerKey = "owner";
    const QString ownerNameKey = "name";
    const QString ownerUserKey = "username";
    const QString ownerEmailKey = "email";
    const QString statusKey = "status";
    const QString projectKey = "project";
    const QString titleKey = "subject";
    const QString urlKey = "url";
    const QString patchSetKey = "currentPatchSet";
    const QString refKey = "ref";
    const QString approvalsKey = "approvals";
    const QString approvalsValueKey = "value";
    const QString approvalsByKey = "by";
    const QString lastUpdatedKey = "lastUpdated";
    const QList<QByteArray> lines = output.split('\n');
    const QString approvalsTypeKey = "type";
    const QString approvalsDescriptionKey = "description";

    bool res = true;
    result.clear();
    result.reserve(lines.size());

    for (const QByteArray &line : lines) {
        if (line.isEmpty())
            continue;
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (doc.isNull()) {
            QString errorMessage = GerritModel::tr("Parse error: \"%1\" -> %2")
                    .arg(QString::fromLocal8Bit(line))
                    .arg(error.errorString());
            qWarning() << errorMessage;
            VcsOutputWindow::appendError(errorMessage);
            res = false;
            continue;
        }
        GerritChangePtr change(new GerritChange);
        const QJsonObject object = doc.object();
        // Skip stats line: {"type":"stats","rowCount":9,"runTimeMilliseconds":13}
        if (!object.value(typeKey).toString().isEmpty())
            continue;
        // Read current patch set.
        const QJsonObject patchSet = object.value(patchSetKey).toObject();
        change->currentPatchSet.patchSetNumber = qMax(1, patchSet.value(numberKey).toString().toInt());
        change->currentPatchSet.ref = patchSet.value(refKey).toString();
        const QJsonArray approvalsJ = patchSet.value(approvalsKey).toArray();
        const int ac = approvalsJ.size();
        for (int a = 0; a < ac; ++a) {
            const QJsonObject ao = approvalsJ.at(a).toObject();
            GerritApproval approval;
            const QJsonObject approverO = ao.value(approvalsByKey).toObject();
            approval.reviewer = approverO.value(ownerNameKey).toString();
            approval.email = approverO.value(ownerEmailKey).toString();
            approval.approval = ao.value(approvalsValueKey).toString().toInt();
            approval.type = ao.value(approvalsTypeKey).toString();
            approval.description = ao.value(approvalsDescriptionKey).toString();
            change->currentPatchSet.approvals.push_back(approval);
        }
        std::stable_sort(change->currentPatchSet.approvals.begin(),
                         change->currentPatchSet.approvals.end(),
                         gerritApprovalLessThan);
        // Remaining
        change->number = object.value(numberKey).toString().toInt();
        change->url = object.value(urlKey).toString();
        if (change->url.isEmpty()) //  No "canonicalWebUrl" is in gerrit.config.
            change->url = defaultUrl(parameters, server, change->number);
        change->title = object.value(titleKey).toString();
        const QJsonObject ownerJ = object.value(ownerKey).toObject();
        change->owner = ownerJ.value(ownerNameKey).toString();
        change->user = ownerJ.value(ownerUserKey).toString();
        change->email = ownerJ.value(ownerEmailKey).toString();
        change->project = object.value(projectKey).toString();
        change->branch = object.value(branchKey).toString();
        change->status =  object.value(statusKey).toString();
        if (const int timeT = qRound(object.value(lastUpdatedKey).toDouble()))
            change->lastUpdated = QDateTime::fromTime_t(timeT);
        if (change->isValid()) {
            result.push_back(change);
        } else {
            qWarning("%s: Parse error: '%s'.", Q_FUNC_INFO, line.constData());
            VcsOutputWindow::appendError(GerritModel::tr("Parse error: \"%1\"")
                                  .arg(QString::fromLocal8Bit(line)));
            res = false;
        }
        // Read out dependencies
        const QJsonValue dependsOnValue = object.value(dependsOnKey);
        if (dependsOnValue.isArray()) {
            const QJsonArray dependsOnArray = dependsOnValue.toArray();
            if (!dependsOnArray.isEmpty()) {
                const QJsonValue first = dependsOnArray.at(0);
                if (first.isObject())
                    change->dependsOnNumber = first.toObject()[numberKey].toString().toInt();
            }
        }
        // Read out needed by
        const QJsonValue neededByValue = object.value(neededByKey);
        if (neededByValue.isArray()) {
            const QJsonArray neededByArray = neededByValue.toArray();
            if (!neededByArray.isEmpty()) {
                const QJsonValue first = neededByArray.at(0);
                if (first.isObject())
                    change->neededByNumber = first.toObject()[numberKey].toString().toInt();
            }
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
    row[OwnerColumn]->setText(c->owner);
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
    if (c->user == m_server->user) { // Owned changes: Review != 0,1. Submit or amend.
        const int level = c->currentPatchSet.approvalLevel();
        bold = level != 0 && level != 1;
    } else if (m_query->currentQuery() == 1) { // Changes pending for review: No review yet.
        bold = !m_server->user.isEmpty() && !c->currentPatchSet.hasApproval(m_server->user);
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
    return c1->lastUpdated < c2->lastUpdated;
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
