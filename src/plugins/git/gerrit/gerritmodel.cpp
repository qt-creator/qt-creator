/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
#include <QDebug>
#include <QScopedPointer>
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

// Format default Url for a change
static inline QString defaultUrl(const QSharedPointer<GerritParameters> &p, int gerritNumber)
{
    QString result = p->https ? QLatin1String("https://") : QLatin1String("http://");
    result += p->host;
    result += QLatin1Char('/');
    result += QString::number(gerritNumber);
    return result;
}

// Format approvals as "John Doe: -1, ...".
QString GerritPatchSet::approvalsToolTip() const
{
    QString result;
    if (const int approvalsCount = approvals.size()) {
        const QString sepComma = QLatin1String(", ");
        const QString sepColon = QLatin1String(": ");
        for (int i = 0; i < approvalsCount; ++ i) {
            if (i)
                result += sepComma;
            result += approvals.at(i).first;
            result += sepColon;
            result += QString::number(approvals.at(i).second);
        }
    }
    return result;
}

QString GerritChange::toolTip() const
{
    static const QString format = GerritModel::tr(
       "Subject: %1\nNumber: %2 Id: %3\nOwner: %4 <%5>\n"
       "Project: %6 Branch: %7\nStatus: %8, %9\n"
       "URL: %10\nApprovals: %11");
    return format.arg(title).arg(number).arg(id, owner, email, branch)
           .arg(currentPatchSet.patchSetNumber)
           .arg(status, lastUpdated.toString(Qt::DefaultLocaleShortDate),
                url, currentPatchSet.approvalsToolTip());
}

QString GerritChange::filterString() const
{
    const QChar blank = QLatin1Char(' ');
    QString result = QString::number(number) + blank + title + blank
            + owner + blank + project + blank
            + branch + blank + status;
    foreach (const GerritPatchSet::Approval &a, currentPatchSet.approvals) {
        result += blank;
        result += a.first;
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
    typedef QList<QStringList> StringListList;

    QueryContext(const StringListList &queries,
                 const QSharedPointer<GerritParameters> &p,
                 QObject *parent = 0);

    ~QueryContext();

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

private:
    void startQuery(const QStringList &queryArguments);
    void errorTermination(const QString &msg);

    const QSharedPointer<GerritParameters> m_parameters;
    const StringListList m_queries;
    QProcess m_process;
    QString m_binary;
    QByteArray m_output;
    int m_currentQuery;
    QFutureInterface<void> m_progress;
    QStringList m_baseArguments;
};

QueryContext::QueryContext(const StringListList &queries,
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
}

QueryContext::~QueryContext()
{
    if (m_progress.isRunning())
        m_progress.reportFinished();
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

void QueryContext::startQuery(const QStringList &queryArguments)
{
    const QStringList arguments = m_baseArguments + queryArguments;
    VcsBase::VcsBaseOutputWindow::instance()
        ->appendCommand(m_process.workingDirectory(), m_binary, arguments);
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
    if (e == QProcess::FailedToStart) {
        errorTermination(msg);
    } else {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(msg);
    }
}

void QueryContext::processFinished(int exitCode, QProcess::ExitStatus es)
{
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

GerritModel::GerritModel(const QSharedPointer<GerritParameters> &p, QObject *parent)
    : QStandardItemModel(0, ColumnCount, parent)
    , m_parameters(p)
    , m_query(0)
{
    QStringList headers;
    headers << QLatin1String("#") << tr("Title") << tr("Owner")
            << tr("Date") << tr("Project")
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

void GerritModel::refresh()
{
    typedef QueryContext::StringListList StringListList;

    if (m_query) {
        qWarning("%s: Another query is still running", Q_FUNC_INFO);
        return;
    }
    clearData();

    // Assemble list of queries
    const QString statusOpenQuery = QLatin1String("status:open");

    StringListList queries;
    QStringList queryArguments;
    if (m_parameters->user.isEmpty()) {
        queryArguments << statusOpenQuery;
        queries.push_back(queryArguments);
        queryArguments.clear();
    } else {
        // Owned by:
        queryArguments << (QLatin1String("owner:") + m_parameters->user) << statusOpenQuery;
        queries.push_back(queryArguments);
        queryArguments.clear();
        // For Review by:
        queryArguments << (QLatin1String("reviewer:") + m_parameters->user) << statusOpenQuery;
        queries.push_back(queryArguments);
        queryArguments.clear();
    }
    // Any custom queries?
    if (!m_parameters->additionalQueries.isEmpty()) {
        foreach (const QString &customQuery, m_parameters->additionalQueries.split(QString::SkipEmptyParts)) {
            queryArguments = customQuery.split(QLatin1Char(' '), QString::SkipEmptyParts);
            if (!queryArguments.isEmpty()) {
                queries.push_back(queryArguments);
                queryArguments.clear();
            }
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
static QList<GerritChangePtr> parseOutput(const QSharedPointer<GerritParameters> &parameters,
                                          const QByteArray &output)
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
    QList<GerritChangePtr> result;
    result.reserve(lines.size());

    foreach (const QByteArray &line, lines) {
        if (line.isEmpty())
            continue;
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (doc.isNull()) {
            qWarning("Parse error: '%s' -> %s", line.constData(),
                     qPrintable(error.errorString()));
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
            const int value = ao.value(approvalsValueKey).toString().toInt();
            const QString by = ao.value(approvalsByKey).toObject().value(ownerNameKey).toString();
            change->currentPatchSet.approvals.push_back(GerritChange::Approval(by, value));
        }
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
            qWarning("%s: Parse error in line '%s'.", Q_FUNC_INFO, line.constData());
        }
    }
    return result;
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

static QList<GerritChangePtr> parseOutput(const QSharedPointer<GerritParameters> &parameters,
                                          const QByteArray &output)
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
    const QString lastUpdatedKey = QLatin1String("lastUpdated");
    QList<GerritChangePtr> result;
    result.reserve(lines.size());

    foreach (const QByteArray &line, lines) {
        if (line.isEmpty())
            continue;
        QScopedPointer<Utils::JsonValue> objectValue(Utils::JsonValue::create(QString::fromAscii(line)));
        if (objectValue.isNull()) {
            qWarning("Parse error: '%s'", line.constData());
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
                    if (Utils::JsonValue *byV = oc->member(approvalsByKey)) {
                        Utils::JsonObjectValue *byO = byV->toObject();
                        QTC_ASSERT(byO, break );
                        by = jsonStringMember(byO, ownerNameKey);
                    }
                    change->currentPatchSet.approvals.push_back(GerritChange::Approval(by, value));
                }
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
            qWarning("%s: Parse error in line '%s'.", Q_FUNC_INFO, line.constData());
        }
    }
    return result;
}
#endif // QT_VERSION < 0x050000

bool changeDateLessThan(const GerritChangePtr &c1, const GerritChangePtr &c2)
{
    return c1->lastUpdated < c2->lastUpdated;
}

void GerritModel::queryFinished(const QByteArray &output)
{
    QList<GerritChangePtr> changes = parseOutput(m_parameters, output);
    qStableSort(changes.begin(), changes.end(), changeDateLessThan);
    foreach (const GerritChangePtr &c, changes) {
        // Avoid duplicate entries for example in the (unlikely)
        // case people do self-reviews.
        if (indexOf(c->number) == -1) {
            const QVariant filterV = QVariant(c->filterString());
            const QString toolTip = c->toolTip();
            const QVariant changeV = qVariantFromValue(c);
            QList<QStandardItem *> row;
            for (int i = 0; i < GerritModel::ColumnCount; ++i) {
                QStandardItem *item = new QStandardItem;
                item->setData(changeV, GerritModel::GerritChangeRole);
                item->setData(filterV, GerritModel::FilterRole);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                item->setToolTip(toolTip);
                row.append(item);
            }
            row[NumberColumn]->setText(QString::number(c->number));
            row[TitleColumn]->setText(c->title);
            row[OwnerColumn]->setText(c->owner);
            row[DateColumn]->setText(c->lastUpdated.toString(Qt::SystemLocaleShortDate));
            QString project = c->project;
            if (c->branch != QLatin1String("master"))
                project += QLatin1String(" (") + c->branch  + QLatin1Char(')');
            row[ProjectColumn]->setText(project);
            row[StatusColumn]->setText(c->status);
            QString approvals;
            foreach (const GerritChange::Approval &a, c->currentPatchSet.approvals) {
                if (!approvals.isEmpty())
                    approvals.append(QLatin1String(", "));
                approvals.append(QString::number(a.second));
            }
            row[ApprovalsColumn]->setText(approvals);
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
