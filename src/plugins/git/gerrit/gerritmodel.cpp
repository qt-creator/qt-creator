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

#include "gerritmodel.h"
#include "gerritparameters.h"

#include <coreplugin/vcsmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <utils/synchronousprocess.h>

#include <QStringList>
#include <QProcess>
#include <QVariant>
#include <QUrl>
#include <QTextStream>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QDebug>
#include <QScopedPointer>
#include <QTimer>
#include <QApplication>
#include <QMessageBox>
#if QT_VERSION >= 0x050000
#  include <QJsonDocument>
#  include <QJsonValue>
#  include <QJsonArray>
#  include <QJsonObject>
#else
#  include <utils/json.h>
#  include <utils/qtcassert.h>
#endif

enum { debug = 0 };

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
    d.nospace() << c.title << " by " << c.email
                << ' ' << c.lastUpdated << ' ' <<  c.currentPatchSet;
    return d;
}

// Format default Url for a change
static inline QString defaultUrl(const QSharedPointer<GerritParameters> &p, int gerritNumber)
{
    QString result = p->https ? QLatin1String("https://") : QLatin1String("http://");
    result += p->host;
    result += QLatin1Char('/');
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
    foreach (const GerritApproval &a, approvals) {
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
    foreach (const GerritApproval &a, approvals) {
        if (a.type != QLatin1String("STGN")) { // Qt-Project specific: Ignore "STGN" (Staged)
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
    foreach (const GerritApproval &a, approvals)
        if (a.reviewer == userName)
            return true;
    return false;
}

int GerritPatchSet::approvalLevel() const
{
    int value = 0;
    foreach (const GerritApproval &a, approvals)
        applyApproval(a.approval, &value);
    return value;
}

QString GerritChange::toHtml() const
{
    // Keep in sync with list model headers.
    static const QString format = GerritModel::tr(
       "<html><head/><body><table>"
       "<tr><td>Subject</td><td>%1</td></tr>"
       "<tr><td>Number</td><td><a href=\"%11\">%2</a></td></tr>"
       "<tr><td>Owner</td><td>%3 <a href=\"mailto:%4\">%4</a></td></tr>"
       "<tr><td>Project</td><td>%5 (%6)</td></tr>"
       "<tr><td>Status</td><td>%7, %8</td></tr>"
       "<tr><td>Patch set</td><td>%9</td></tr>"
       "%10"
       "<tr><td>URL</td><td><a href=\"%11\">%11</a></td></tr>"
       "</table></body></html>");
    return format.arg(title).arg(number).arg(owner, email, project, branch)
           .arg(status, lastUpdated.toString(Qt::DefaultLocaleShortDate))
           .arg(currentPatchSet.patchSetNumber)
           .arg(currentPatchSet.approvalsToHtml(), url);
}

QString GerritChange::filterString() const
{
    const QChar blank = QLatin1Char(' ');
    QString result = QString::number(number) + blank + title + blank
            + owner + blank + project + blank
            + branch + blank + status;
    foreach (const GerritApproval &a, currentPatchSet.approvals) {
        result += blank;
        result += a.reviewer;
    }
    return result;
}

QStringList GerritChange::gitFetchArguments(const QSharedPointer<GerritParameters> &p) const
{
    QStringList arguments;
    const QString url = QLatin1String("ssh://") + p->sshHostArgument()
            + QLatin1Char(':') + QString::number(p->port) + QLatin1Char('/')
            + project;
    arguments << QLatin1String("fetch") << url << currentPatchSet.ref;
    return arguments;
}

// Helper class that runs ssh gerrit queries from a list of query argument
// string lists,
// see http://gerrit.googlecode.com/svn/documentation/2.1.5/cmd-query.html
// In theory, querying uses a continuation/limit protocol, but we assume
// we will never reach a limit with those queries.

class QueryContext : public QObject {
    Q_OBJECT
public:
    QueryContext(const QStringList &queries,
                 const QSharedPointer<GerritParameters> &p,
                 QObject *parent = 0);

    ~QueryContext();

    int currentQuery() const { return m_currentQuery; }

public slots:
    void start();

signals:
    void queryFinished(const QByteArray &);
    void finished();

private slots:
    void processError(QProcess::ProcessError);
    void processFinished(int exitCode, QProcess::ExitStatus);
    void readyReadStandardError();
    void readyReadStandardOutput();
    void timeout();

private:
    void startQuery(const QString &query);
    void errorTermination(const QString &msg);

    const QSharedPointer<GerritParameters> m_parameters;
    const QStringList m_queries;
    QProcess m_process;
    QTimer m_timer;
    QString m_binary;
    QByteArray m_output;
    int m_currentQuery;
    QFutureInterface<void> m_progress;
    QStringList m_baseArguments;
};

enum { timeOutMS = 30000 };

QueryContext::QueryContext(const QStringList &queries,
                           const QSharedPointer<GerritParameters> &p,
                           QObject *parent)
    : QObject(parent)
    , m_parameters(p)
    , m_queries(queries)
    , m_currentQuery(0)
    , m_baseArguments(p->baseCommandArguments())
{
    connect(&m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(readyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readyReadStandardOutput()));
    connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError(QProcess::ProcessError)));
    m_progress.setProgressRange(0, m_queries.size());

    // Determine binary and common command line arguments.
    m_baseArguments << QLatin1String("query") << QLatin1String("--current-patch-set")
                    << QLatin1String("--format=JSON");
    m_binary = m_baseArguments.front();
    m_baseArguments.pop_front();

    m_timer.setInterval(timeOutMS);
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

QueryContext::~QueryContext()
{
    if (m_progress.isRunning())
        m_progress.reportFinished();
    if (m_timer.isActive())
        m_timer.stop();
    m_process.disconnect(this);
    Utils::SynchronousProcess::stopProcess(m_process);
}

void QueryContext::start()
{
    Core::ProgressManager *pm = Core::ICore::instance()->progressManager();
    Core::FutureProgress *fp = pm->addTask(m_progress.future(), tr("Gerrit"),
                                           QLatin1String("gerrit-query"));
    fp->setKeepOnFinish(Core::FutureProgress::HideOnFinish);
    m_progress.reportStarted();
    startQuery(m_queries.front()); // Order: synchronous call to  error handling if something goes wrong.
}

void QueryContext::startQuery(const QString &query)
{
    QStringList arguments = m_baseArguments;
    arguments.push_back(query);
    VcsBase::VcsBaseOutputWindow::instance()
        ->appendCommand(m_process.workingDirectory(), m_binary, arguments);
    m_timer.start();
    m_process.start(m_binary, arguments);
    m_process.closeWriteChannel();
}

void QueryContext::errorTermination(const QString &msg)
{
    m_progress.reportCanceled();
    m_progress.reportFinished();
    VcsBase::VcsBaseOutputWindow::instance()->appendError(msg);
    emit finished();
}

void QueryContext::processError(QProcess::ProcessError e)
{
    const QString msg = tr("Error running %1: %2").arg(m_binary, m_process.errorString());
    if (e == QProcess::FailedToStart)
        errorTermination(msg);
    else
        VcsBase::VcsBaseOutputWindow::instance()->appendError(msg);
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
    emit queryFinished(m_output);
    m_output.clear();

    if (++m_currentQuery >= m_queries.size()) {
        m_progress.reportFinished();
        emit finished();
    } else {
        m_progress.setProgressValue(m_currentQuery);
        startQuery(m_queries.at(m_currentQuery));
    }
}

void QueryContext::readyReadStandardError()
{
    VcsBase::VcsBaseOutputWindow::instance()->appendError(QString::fromLocal8Bit(m_process.readAllStandardError()));
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
                    tr("The gerrit process has not responded within %1s.\n"
                       "Most likely this is caused by problems with SSH-authentication.\n"
                       "Would you like to terminate it?").
                    arg(timeOutMS / 1000), QMessageBox::NoButton, parent);
    QPushButton *terminateButton = box.addButton(tr("Terminate"), QMessageBox::YesRole);
    box.addButton(tr("Keep running"), QMessageBox::NoRole);
    connect(&m_process, SIGNAL(finished(int)), &box, SLOT(reject()));
    box.exec();
    if (m_process.state() != QProcess::Running)
        return;
    if (box.clickedButton() == terminateButton)
        Utils::SynchronousProcess::stopProcess(m_process);
    else
        m_timer.start();
}

GerritModel::GerritModel(const QSharedPointer<GerritParameters> &p, QObject *parent)
    : QStandardItemModel(0, ColumnCount, parent)
    , m_parameters(p)
    , m_query(0)
{
    QStringList headers; // Keep in sync with GerritChange::toHtml()
    headers << QLatin1String("#") << tr("Subject") << tr("Owner")
            << tr("Updated") << tr("Project")
            << tr("Approvals") << tr("Status");
    setHorizontalHeaderLabels(headers);
}

GerritModel::~GerritModel()
{
}

GerritChangePtr GerritModel::change(int row) const
{
    if (row >= 0 && row < rowCount())
        return qvariant_cast<GerritChangePtr>(item(row, 0)->data(GerritChangeRole));
    return GerritChangePtr(new GerritChange);
}

int GerritModel::indexOf(int gerritNumber) const
{
    const int numRows = rowCount();
    for (int r = 0; r < numRows; ++r)
        if (change(r)->number == gerritNumber)
            return r;
    return -1;
}

void GerritModel::refresh(const QString &query)
{
    if (m_query) {
        qWarning("%s: Another query is still running", Q_FUNC_INFO);
        return;
    }
    clearData();

    // Assemble list of queries

    QStringList queries;
    if (!query.trimmed().isEmpty())
        queries.push_back(query);
    else
    {
        const QString statusOpenQuery = QLatin1String("status:open");
        if (m_parameters->user.isEmpty()) {
            queries.push_back(statusOpenQuery);
        } else {
            // Owned by:
            queries.push_back(statusOpenQuery + QLatin1String(" owner:") + m_parameters->user);
            // For Review by:
            queries.push_back(statusOpenQuery + QLatin1String(" reviewer:") + m_parameters->user);
        }
    }

    m_query = new QueryContext(queries, m_parameters, this);
    connect(m_query, SIGNAL(queryFinished(QByteArray)),
            this, SLOT(queryFinished(QByteArray)));
    connect(m_query, SIGNAL(finished()),
            this, SLOT(queriesFinished()));
    emit refreshStateChanged(true);
    m_query->start();
}

void GerritModel::clearData()
{
    if (const int rows = rowCount())
        removeRows(0, rows);
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

#if QT_VERSION >= 0x050000
// Qt 5 using the QJsonDocument classes.
static bool parseOutput(const QSharedPointer<GerritParameters> &parameters,
                        const QByteArray &output,
                        QList<GerritChangePtr> &result)
{
    // The output consists of separate lines containing a document each
    const QString typeKey = QLatin1String("type");
    const QString idKey = QLatin1String("id");
    const QString branchKey = QLatin1String("branch");
    const QString numberKey = QLatin1String("number");
    const QString ownerKey = QLatin1String("owner");
    const QString ownerNameKey = QLatin1String("name");
    const QString ownerEmailKey = QLatin1String("email");
    const QString statusKey = QLatin1String("status");
    const QString projectKey = QLatin1String("project");
    const QString titleKey = QLatin1String("subject");
    const QString urlKey = QLatin1String("url");
    const QString patchSetKey = QLatin1String("currentPatchSet");
    const QString refKey = QLatin1String("ref");
    const QString approvalsKey = QLatin1String("approvals");
    const QString approvalsValueKey = QLatin1String("value");
    const QString approvalsByKey = QLatin1String("by");
    const QString lastUpdatedKey = QLatin1String("lastUpdated");
    const QList<QByteArray> lines = output.split('\n');
    const QString approvalsTypeKey = QLatin1String("type");
    const QString approvalsDescriptionKey = QLatin1String("description");

    bool res = true;
    result.clear();
    result.reserve(lines.size());

    foreach (const QByteArray &line, lines) {
        if (line.isEmpty())
            continue;
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (doc.isNull()) {
            QString errorMessage = GerritModel::tr("Parse error: '%1' -> %2")
                    .arg(QString::fromLocal8Bit(line))
                    .arg(error.errorString());
            qWarning() << errorMessage;
            VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
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
        qStableSort(change->currentPatchSet.approvals.begin(),
                    change->currentPatchSet.approvals.end(),
                    gerritApprovalLessThan);
        // Remaining
        change->number = object.value(numberKey).toString().toInt();
        change->url = object.value(urlKey).toString();
        if (change->url.isEmpty()) //  No "canonicalWebUrl" is in gerrit.config.
            change->url = defaultUrl(parameters, change->number);
        change->id = object.value(idKey).toString();
        change->title = object.value(titleKey).toString();
        const QJsonObject ownerJ = object.value(ownerKey).toObject();
        change->owner = ownerJ.value(ownerNameKey).toString();
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
            VcsBase::VcsBaseOutputWindow::instance()
                    ->appendError(GerritModel::tr("Parse error: '%1'")
                                  .arg(QString::fromLocal8Bit(line)));
            res = false;
        }
    }
    return res;
}
#else // QT_VERSION >= 0x050000
// Qt 4.8 using the Json classes from utils.

static inline int jsonIntMember(Utils::JsonObjectValue *object,
                                const QString &key,
                                int defaultValue = 0)
{
    if (Utils::JsonValue *v = object->member(key)) {
        if (Utils::JsonIntValue *iv = v->toInt())
            return iv->value();
        if (Utils::JsonStringValue *sv = v->toString()) {
            bool ok;
            const int result = sv->value().toInt(&ok);
            if (ok)
                return result;
        }
    }
    return defaultValue;
}

static inline QString jsonStringMember(Utils::JsonObjectValue *object,
                                       const QString &key)
{
    if (Utils::JsonValue *v = object->member(key))
        if (Utils::JsonStringValue *sv = v->toString())
            return sv->value();
    return QString();
}

static bool parseOutput(const QSharedPointer<GerritParameters> &parameters,
                                          const QByteArray &output,
                                          QList<GerritChangePtr> &result)
{
   // The output consists of separate lines containing a document each
    const QList<QByteArray> lines = output.split('\n');
    const QString typeKey = QLatin1String("type");
    const QString idKey = QLatin1String("id");
    const QString branchKey = QLatin1String("branch");
    const QString numberKey = QLatin1String("number");
    const QString ownerKey = QLatin1String("owner");
    const QString ownerNameKey = QLatin1String("name");
    const QString ownerEmailKey = QLatin1String("email");
    const QString statusKey = QLatin1String("status");
    const QString projectKey = QLatin1String("project");
    const QString titleKey = QLatin1String("subject");
    const QString urlKey = QLatin1String("url");
    const QString patchSetKey = QLatin1String("currentPatchSet");
    const QString refKey = QLatin1String("ref");
    const QString approvalsKey = QLatin1String("approvals");
    const QString approvalsValueKey = QLatin1String("value");
    const QString approvalsByKey = QLatin1String("by");
    const QString approvalsTypeKey = QLatin1String("type");
    const QString approvalsDescriptionKey = QLatin1String("description");
    const QString lastUpdatedKey = QLatin1String("lastUpdated");
    bool res = true;
    result.clear();
    result.reserve(lines.size());

    foreach (const QByteArray &line, lines) {
        if (line.isEmpty())
            continue;
        QScopedPointer<Utils::JsonValue> objectValue(Utils::JsonValue::create(QString::fromUtf8(line)));
        if (objectValue.isNull()) {
            QString errorMessage = GerritModel::tr("Parse error: '%1'")
                    .arg(QString::fromLocal8Bit(line));
            qWarning() << errorMessage;
            VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
            res = false;
            continue;
        }
        Utils::JsonObjectValue *object = objectValue->toObject();
        QTC_ASSERT(object, continue );
        GerritChangePtr change(new GerritChange);
        // Skip stats line: {"type":"stats","rowCount":9,"runTimeMilliseconds":13}
        if (jsonStringMember(object, typeKey) == QLatin1String("stats"))
            continue;
        // Read current patch set.
        if (Utils::JsonValue *patchSetValue = object->member(patchSetKey)) {
            Utils::JsonObjectValue *patchSet = patchSetValue->toObject();
            QTC_ASSERT(patchSet, continue );
            const int n = jsonIntMember(patchSet, numberKey);
            change->currentPatchSet.patchSetNumber = qMax(1, n);
            change->currentPatchSet.ref = jsonStringMember(patchSet, refKey);
            if (Utils::JsonValue *approvalsV = patchSet->member(approvalsKey)) {
                Utils::JsonArrayValue *approvals = approvalsV->toArray();
                QTC_ASSERT(approvals, continue );
                foreach (Utils::JsonValue *c, approvals->elements()) {
                    Utils::JsonObjectValue *oc = c->toObject();
                    QTC_ASSERT(oc, break );
                    const int value = jsonIntMember(oc, approvalsValueKey);
                    QString by;
                    QString byMail;
                    if (Utils::JsonValue *byV = oc->member(approvalsByKey)) {
                        Utils::JsonObjectValue *byO = byV->toObject();
                        QTC_ASSERT(byO, break );
                        by = jsonStringMember(byO, ownerNameKey);
                        byMail = jsonStringMember(byO, ownerEmailKey);
                    }
                    GerritApproval a;
                    a.reviewer = by;
                    a.email = byMail;
                    a.approval = value;
                    a.type = jsonStringMember(oc, approvalsTypeKey);
                    a.description = jsonStringMember(oc, approvalsDescriptionKey);
                    change->currentPatchSet.approvals.push_back(a);
                }
                qStableSort(change->currentPatchSet.approvals.begin(),
                            change->currentPatchSet.approvals.end(),
                            gerritApprovalLessThan);
            }
        } // patch set
        // Remaining

        change->number = jsonIntMember(object, numberKey);
        change->url = jsonStringMember(object, urlKey);
        if (change->url.isEmpty()) //  No "canonicalWebUrl" is in gerrit.config.
            change->url = defaultUrl(parameters, change->number);
        change->id = jsonStringMember(object, idKey);
        change->title = jsonStringMember(object, titleKey);
        if (Utils::JsonValue *ownerV = object->member(ownerKey)) {
            Utils::JsonObjectValue *ownerO = ownerV->toObject();
            QTC_ASSERT(ownerO, continue );
            change->owner = jsonStringMember(ownerO, ownerNameKey);
            change->email = jsonStringMember(ownerO, ownerEmailKey);
        }
        change->project = jsonStringMember(object, projectKey);
        change->branch = jsonStringMember(object, branchKey);
        change->status = jsonStringMember(object, statusKey);

        if (const int timeT = jsonIntMember(object, lastUpdatedKey))
            change->lastUpdated = QDateTime::fromTime_t(timeT);
        if (change->isValid()) {
            result.push_back(change);
        } else {
            QString errorMessage = GerritModel::tr("Parse error in line '%1'")
                    .arg(QString::fromLocal8Bit(line));
            VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
            qWarning("%s: Parse error in line '%s'.", Q_FUNC_INFO, line.constData());
            res = false;
        }
    }
    if (debug) {
        qDebug() << __FUNCTION__;
        foreach (const GerritChangePtr &p, result)
            qDebug() << *p;
    }
    return res;
}
#endif // QT_VERSION < 0x050000

bool changeDateLessThan(const GerritChangePtr &c1, const GerritChangePtr &c2)
{
    return c1->lastUpdated < c2->lastUpdated;
}

void GerritModel::queryFinished(const QByteArray &output)
{
    const QDate today = QDate::currentDate();
    QList<GerritChangePtr> changes;
    if (!parseOutput(m_parameters, output, changes))
        emit queryError();
    qStableSort(changes.begin(), changes.end(), changeDateLessThan);
    foreach (const GerritChangePtr &c, changes) {
        // Avoid duplicate entries for example in the (unlikely)
        // case people do self-reviews.
        if (indexOf(c->number) == -1) {
            // Determine the verbose user name from the owner of the first query.
            // It used for marking the changes pending for review in bold.
            if (m_userName.isEmpty() && !m_query->currentQuery())
                m_userName = c->owner;
            const QVariant filterV = QVariant(c->filterString());
            const QVariant changeV = qVariantFromValue(c);
            QList<QStandardItem *> row;
            for (int i = 0; i < GerritModel::ColumnCount; ++i) {
                QStandardItem *item = new QStandardItem;
                item->setData(changeV, GerritModel::GerritChangeRole);
                item->setData(filterV, GerritModel::FilterRole);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                row.append(item);
            }
            row[NumberColumn]->setText(QString::number(c->number));
            row[TitleColumn]->setText(c->title);
            row[OwnerColumn]->setText(c->owner);
            // Shorten columns: Display time if it is today, else date
            const QString dateString = c->lastUpdated.date() == today ?
                c->lastUpdated.time().toString(Qt::SystemLocaleShortDate) :
                c->lastUpdated.date().toString(Qt::SystemLocaleShortDate);
            row[DateColumn]->setText(dateString);

            QString project = c->project;
            if (c->branch != QLatin1String("master"))
                project += QLatin1String(" (") + c->branch  + QLatin1Char(')');
            row[ProjectColumn]->setText(project);
            row[StatusColumn]->setText(c->status);
            row[ApprovalsColumn]->setText(c->currentPatchSet.approvalsColumn());
            // Mark changes awaiting action using a bold font.
            bool bold = false;
            if (c->owner == m_userName) { // Owned changes: Review != 0,1. Submit or amend.
                const int level = c->currentPatchSet.approvalLevel();
                bold = level != 0 && level != 1;
            } else if (m_query->currentQuery() == 1) { // Changes pending for review: No review yet.
                bold = !m_userName.isEmpty() && !c->currentPatchSet.hasApproval(m_userName);
            }
            if (bold) {
                QFont font = row.first()->font();
                font.setBold(true);
                for (int i = 0; i < GerritModel::ColumnCount; ++i)
                    row[i]->setFont(font);
            }
            appendRow(row);
        }
    }
}

void GerritModel::queriesFinished()
{
    m_query->deleteLater();
    m_query = 0;
    emit refreshStateChanged(false);
}

} // namespace Internal
} // namespace Gerrit

#include "gerritmodel.moc"
