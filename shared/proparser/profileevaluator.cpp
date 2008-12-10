/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "profileevaluator.h"
#include "proparserutils.h"
#include "proitems.h"

#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#else
#define QT_POPEN popen
#endif

QT_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////
//
// ProFileEvaluator::Private
//
///////////////////////////////////////////////////////////////////////

class ProFileEvaluator::Private : public AbstractProItemVisitor
{
public:
    Private(ProFileEvaluator *q_);

    bool read(ProFile *pro);

    void writeItem(const QList<ProItem *> &items, int index, QTextStream &out, QString indent);
    ProBlock *currentBlock();
    void updateItem();
    bool parseLine(const QString &line);
    void insertVariable(const QString &line, int *i);
    void insertOperator(const char op);
    void insertComment(const QString &comment);
    void enterScope(bool multiLine);
    void leaveScope();
    void finalizeBlock();
    void cleanup();

    // implementation of AbstractProItemVisitor
    bool visitBeginProBlock(ProBlock *block);
    bool visitEndProBlock(ProBlock *block);
    bool visitBeginProVariable(ProVariable *variable);
    bool visitEndProVariable(ProVariable *variable);
    bool visitBeginProFile(ProFile *value);
    bool visitEndProFile(ProFile *value);
    bool visitProValue(ProValue *value);
    bool visitProFunction(ProFunction *function);
    bool visitProOperator(ProOperator *oper);
    bool visitProCondition(ProCondition *condition);

    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

    bool isActiveConfig(const QString &config, bool regex = false);
    QStringList expandPattern(const QString &pattern);
    void expandPatternHelper(const QString &relName, const QString &absName,
        QStringList &sources_out);
    QStringList expandVariableReferences(const QString &value);
    QStringList evaluateExpandFunction(const QString &function, const QString &arguments);
    QString format(const char *format) const;

    QString currentFileName() const;
    QString getcwd() const;
    ProFile *currentProFile() const;

    bool evaluateConditionalFunction(const QString &function, const QString &arguments, bool *result);
    bool evaluateFile(const QString &fileName, bool *result);
    bool evaluateFeatureFile(const QString &fileName, bool *result);

    QStringList qmakeFeaturePaths();

    ProFileEvaluator *q;

    QStack<ProBlock *> m_blockstack;
    ProBlock *m_block;

    ProItem *m_commentItem;
    QString m_proitem;
    QString m_pendingComment;
    bool m_syntaxError;
    bool m_contNextLine;
    bool m_condition;
    bool m_invertNext;
    QString m_lastVarName;
    ProVariable::VariableOperator m_variableOperator;
    int m_lineNo;                                   // Error reporting
    QString m_oldPath;                              // To restore the current path to the path
    QStack<ProFile*> m_profileStack;                // To handle 'include(a.pri), so we can track back to 'a.pro' when finished with 'a.pri'

    QHash<QString, QStringList> m_valuemap;         // VariableName must be us-ascii, the content however can be non-us-ascii.
    QHash<const ProFile*, QHash<QString, QStringList> > m_filevaluemap; // Variables per include file
    QHash<QString, QString> m_properties;
    QString m_origfile;

    int m_prevLineNo;                               // Checking whether we're assigning the same TARGET
    ProFile *m_prevProFile;                         // See m_prevLineNo

    bool m_verbose;
};

ProFileEvaluator::Private::Private(ProFileEvaluator *q_)
  : q(q_)
{
    m_prevLineNo = 0;
    m_prevProFile = 0;
    m_verbose = true;
    m_block = 0;
    m_commentItem = 0;
    m_syntaxError = 0;
    m_lineNo = 0;
    m_contNextLine = false;
}

bool ProFileEvaluator::Private::read(ProFile *pro)
{
    QFile file(pro->fileName());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        q->errorMessage(format("%1 not readable.").arg(pro->fileName()));
        return false;
    }

    m_syntaxError = false;
    m_lineNo = 1;
    m_blockstack.push(pro);

    QTextStream ts(&file);
    while (!ts.atEnd()) {
        QString line = ts.readLine();
        if (!parseLine(line)) {
            q->errorMessage(format(".pro parse failure."));
            return false;
        }
        ++m_lineNo;
    }
    return true;
}

bool ProFileEvaluator::Private::parseLine(const QString &line0)
{
    if (m_blockstack.isEmpty())
        return false;

    ushort quote = 0;
    int parens = 0;
    bool contNextLine = false;
    QString line = line0.simplified();

    for (int i = 0; !m_syntaxError && i < line.length(); ++i) {
        ushort c = line.at(i).unicode();
        if (quote && c == quote)
            quote = 0;
        else if (c == '(')
            ++parens;
        else if (c == ')')
            --parens;
        else if (c == '"' && (i == 0 || line.at(i - 1).unicode() != '\\'))
            quote = c;
        else if (!parens && !quote) {
            if (c == '#') {
                insertComment(line.mid(i + 1));
                contNextLine = m_contNextLine;
                break;
            }
            if (c == '\\' && i >= line.count() - 1) {
                updateItem();
                contNextLine = true;
                continue;
            }
            if (m_block && (m_block->blockKind() & ProBlock::VariableKind)) {
                if (c == ' ')
                    updateItem();
                else
                    m_proitem += c;
                continue;
            }
            if (c == ':') {
                enterScope(false);
                continue;
            }
            if (c == '{') {
                enterScope(true);
                continue;
            }
            if (c == '}') {
                leaveScope();
                continue;
            }
            if (c == '=') {
                insertVariable(line, &i);
                continue;
            }
            if (c == '|' || c == '!') {
                insertOperator(c);
                continue;
            }
        }

        m_proitem += c;
    }
    m_contNextLine = contNextLine;

    if (!m_syntaxError) {
        updateItem();
        if (!m_contNextLine)
            finalizeBlock();
    }
    return !m_syntaxError;
}

void ProFileEvaluator::Private::finalizeBlock()
{
    if (m_blockstack.isEmpty()) {
        m_syntaxError = true;
    } else {
        if (m_blockstack.top()->blockKind() & ProBlock::SingleLine)
            leaveScope();
        m_block = 0;
        m_commentItem = 0;
    }
}

void ProFileEvaluator::Private::insertVariable(const QString &line, int *i)
{
    ProVariable::VariableOperator opkind;

    switch (m_proitem.at(m_proitem.length() - 1).unicode()) {
        case '+':
            m_proitem.chop(1);
            opkind = ProVariable::AddOperator;
            break;
        case '-':
            m_proitem.chop(1);
            opkind = ProVariable::RemoveOperator;
            break;
        case '*':
            m_proitem.chop(1);
            opkind = ProVariable::UniqueAddOperator;
            break;
        case '~':
            m_proitem.chop(1);
            opkind = ProVariable::ReplaceOperator;
            break;
        default:
            opkind = ProVariable::SetOperator;
    }

    ProBlock *block = m_blockstack.top();
    m_proitem = m_proitem.trimmed();
    ProVariable *variable = new ProVariable(m_proitem, block);
    variable->setLineNumber(m_lineNo);
    variable->setVariableOperator(opkind);
    block->appendItem(variable);
    m_block = variable;

    if (!m_pendingComment.isEmpty()) {
        m_block->setComment(m_pendingComment);
        m_pendingComment.clear();
    }
    m_commentItem = variable;

    m_proitem.clear();

    if (opkind == ProVariable::ReplaceOperator) {
        // skip util end of line or comment
        while (1) {
            ++(*i);

            // end of line?
            if (*i >= line.count())
                break;

            // comment?
            if (line.at(*i).unicode() == '#') {
                --(*i);
                break;
            }

            m_proitem += line.at(*i);
        }
        m_proitem = m_proitem.trimmed();
    }
}

void ProFileEvaluator::Private::insertOperator(const char op)
{
    updateItem();

    ProOperator::OperatorKind opkind;
    switch (op) {
        case '!':
            opkind = ProOperator::NotOperator;
            break;
        case '|':
            opkind = ProOperator::OrOperator;
            break;
        default:
            opkind = ProOperator::OrOperator;
    }

    ProBlock * const block = currentBlock();
    ProOperator * const proOp = new ProOperator(opkind);
    proOp->setLineNumber(m_lineNo);
    block->appendItem(proOp);
    m_commentItem = proOp;
}

void ProFileEvaluator::Private::insertComment(const QString &comment)
{
    updateItem();

    QString strComment;
    if (!m_commentItem)
        strComment = m_pendingComment;
    else
        strComment = m_commentItem->comment();

    if (strComment.isEmpty())
        strComment = comment;
    else {
        strComment += QLatin1Char('\n');
        strComment += comment.trimmed();
    }

    strComment = strComment.trimmed();

    if (!m_commentItem)
        m_pendingComment = strComment;
    else
        m_commentItem->setComment(strComment);
}

void ProFileEvaluator::Private::enterScope(bool multiLine)
{
    updateItem();

    ProBlock *parent = currentBlock();
    ProBlock *block = new ProBlock(parent);
    block->setLineNumber(m_lineNo);
    parent->setBlockKind(ProBlock::ScopeKind);

    parent->appendItem(block);

    if (multiLine)
        block->setBlockKind(ProBlock::ScopeContentsKind);
    else
        block->setBlockKind(ProBlock::ScopeContentsKind|ProBlock::SingleLine);

    m_blockstack.push(block);
    m_block = 0;
}

void ProFileEvaluator::Private::leaveScope()
{
    updateItem();
    m_blockstack.pop();
    finalizeBlock();
}

ProBlock *ProFileEvaluator::Private::currentBlock()
{
    if (m_block)
        return m_block;

    ProBlock *parent = m_blockstack.top();
    m_block = new ProBlock(parent);
    m_block->setLineNumber(m_lineNo);
    parent->appendItem(m_block);

    if (!m_pendingComment.isEmpty()) {
        m_block->setComment(m_pendingComment);
        m_pendingComment.clear();
    }

    m_commentItem = m_block;

    return m_block;
}

void ProFileEvaluator::Private::updateItem()
{
    m_proitem = m_proitem.trimmed();
    if (m_proitem.isEmpty())
        return;

    ProBlock *block = currentBlock();
    if (block->blockKind() & ProBlock::VariableKind) {
        m_commentItem = new ProValue(m_proitem, static_cast<ProVariable*>(block));
    } else if (m_proitem.endsWith(QLatin1Char(')'))) {
        m_commentItem = new ProFunction(m_proitem);
    } else {
        m_commentItem = new ProCondition(m_proitem);
    }
    m_commentItem->setLineNumber(m_lineNo);
    block->appendItem(m_commentItem);

    m_proitem.clear();
}


bool ProFileEvaluator::Private::visitBeginProBlock(ProBlock *block)
{
    if (block->blockKind() == ProBlock::ScopeKind) {
        m_invertNext = false;
        m_condition = false;
    }
    return true;
}

bool ProFileEvaluator::Private::visitEndProBlock(ProBlock *block)
{
    Q_UNUSED(block);
    return true;
}

bool ProFileEvaluator::Private::visitBeginProVariable(ProVariable *variable)
{
    m_lastVarName = variable->variable();
    m_variableOperator = variable->variableOperator();
    return true;
}

bool ProFileEvaluator::Private::visitEndProVariable(ProVariable *variable)
{
    Q_UNUSED(variable);
    m_lastVarName.clear();
    return true;
}

bool ProFileEvaluator::Private::visitProOperator(ProOperator *oper)
{
    m_invertNext = (oper->operatorKind() == ProOperator::NotOperator);
    return true;
}

bool ProFileEvaluator::Private::visitProCondition(ProCondition *cond)
{
    if (!m_condition) {
        if (m_invertNext)
            m_condition |= !isActiveConfig(cond->text(), true);
        else
            m_condition |= isActiveConfig(cond->text(), true);
    }
    return true;
}

bool ProFileEvaluator::Private::visitBeginProFile(ProFile * pro)
{
    PRE(pro);
    bool ok = true;
    m_lineNo = pro->lineNumber();

    if (m_origfile.isEmpty())
        m_origfile = pro->fileName();
    if (m_oldPath.isEmpty()) {
        // change the working directory for the initial profile we visit, since
        // that is *the* profile. All the other times we reach this function will be due to
        // include(file) or load(file)

        m_oldPath = QDir::currentPath();

        m_profileStack.push(pro);

        const QString mkspecDirectory = propertyValue("QMAKE_MKSPECS");
        if (!mkspecDirectory.isEmpty()) {
            // This is what qmake does, everything set in the mkspec is also set
            // But this also creates a lot of problems
            evaluateFile(mkspecDirectory + "/default/qmake.conf", &ok);
            evaluateFile(mkspecDirectory + "/features/default_pre.prf", &ok);
        }

        QString fn = pro->fileName();
        ok = QDir::setCurrent(QFileInfo(fn).absolutePath());
    }

    return ok;
}

bool ProFileEvaluator::Private::visitEndProFile(ProFile * pro)
{
    PRE(pro);
    bool ok = true;
    m_lineNo = pro->lineNumber();
    if (m_profileStack.count() == 1 && !m_oldPath.isEmpty()) {
        const QString mkspecDirectory = propertyValue("QMAKE_MKSPECS");
        if (!mkspecDirectory.isEmpty()) {
            evaluateFile(mkspecDirectory + "/features/default_post.prf", &ok);

            QStringList processed;
            while (1) {
                bool finished = true;
                QStringList configs = values("CONFIG");
                for (int i = configs.size()-1; i >= 0; --i) {
                    const QString config = configs[i].toLower();
                    if (!processed.contains(config)) {
                        processed.append(config);
                        evaluateFile(mkspecDirectory + "/features/" + config + ".prf", &ok);
                        if (ok) {
                            finished = false;
                            break;
                        }
                    }
                }
                if (finished)
                    break;
            }
        }

        m_profileStack.pop();
        ok = QDir::setCurrent(m_oldPath);
    }
    return ok;
}

bool ProFileEvaluator::Private::visitProValue(ProValue *value)
{
    PRE(value);
    m_lineNo = value->lineNumber();
    QString val = value->value();

    QString varName = m_lastVarName;

    QStringList v = expandVariableReferences(val);

    // Since qmake combines different values for the TARGET variable, we join
    // TARGET values that are on the same line. We can't do this later with all
    // values because this parser isn't scope-aware, so we'd risk joining
    // scope-specific targets together.
    if (varName == QLatin1String("TARGET")
            && m_lineNo == m_prevLineNo
            && currentProFile() == m_prevProFile) {
        QStringList targets = m_valuemap.value(QLatin1String("TARGET"));
        m_valuemap.remove(QLatin1String("TARGET"));
        QStringList lastTarget(targets.takeLast());
        lastTarget << v.join(QLatin1String(" "));
        targets.push_back(lastTarget.join(QLatin1String(" ")));
        v = targets;
    }
    m_prevLineNo = m_lineNo;
    m_prevProFile = currentProFile();

    // The following two blocks fix bug 180128 by making all "interesting"
    // file name absolute in each .pro file, not just the top most one
    if (varName == QLatin1String("SOURCES")
            || varName == QLatin1String("HEADERS")
            || varName == QLatin1String("INTERFACES")
            || varName == QLatin1String("FORMS")
            || varName == QLatin1String("FORMS3")
            || varName == QLatin1String("RESOURCES")) {
        // matches only existent files, expand certain(?) patterns
        QStringList vv;
        for (int i = v.count(); --i >= 0; )
            vv << expandPattern(v[i]);
        v = vv;
    }

    if (varName == QLatin1String("TRANSLATIONS")) {
        // also matches non-existent files, but does not expand pattern
        QString dir = QFileInfo(currentFileName()).absolutePath();
        dir += QLatin1Char('/');
        for (int i = v.count(); --i >= 0; )
            v[i] = QFileInfo(dir, v[i]).absoluteFilePath();
    }

    switch (m_variableOperator) {
        case ProVariable::UniqueAddOperator:    // *
            insertUnique(&m_valuemap, varName, v, true);
            insertUnique(&m_filevaluemap[currentProFile()], varName, v, true);
            break;
        case ProVariable::SetOperator:          // =
        case ProVariable::AddOperator:          // +
            insertUnique(&m_valuemap, varName, v, false);
            insertUnique(&m_filevaluemap[currentProFile()], varName, v, false);
            break;
        case ProVariable::RemoveOperator:       // -
            // fix me: interaction between AddOperator and RemoveOperator
            insertUnique(&m_valuemap, varName.prepend(QLatin1Char('-')), v, false);
            insertUnique(&m_filevaluemap[currentProFile()],
                         varName.prepend(QLatin1Char('-')), v, false);
            break;
        case ProVariable::ReplaceOperator:      // ~
            {
                // DEFINES ~= s/a/b/?[gqi]

/*              Create a superset by executing replacement + adding items that have changed
                to original list. We're not sure if this is really the right approach, so for
                the time being we will just do nothing ...

                QChar sep = val.at(1);
                QStringList func = val.split(sep);
                if (func.count() < 3 || func.count() > 4) {
                    q->logMessage(format("'~= operator '(function s///) expects 3 or 4 arguments."));
                    return false;
                }
                if (func[0] != QLatin1String("s")) {
                    q->logMessage(format("~= operator can only handle s/// function."));
                    return false;
                }
                bool global = false, quote = false, case_sense = false;

                if (func.count() == 4) {
                    global = func[3].indexOf(QLatin1Char('g')) != -1;
                    case_sense = func[3].indexOf(QLatin1Char('i')) == -1;
                    quote = func[3].indexOf(QLatin1Char('q')) != -1;
                }
                QString pattern = func[1];
                QString replace = func[2];
                if (quote)
                    pattern = QRegExp::escape(pattern);

                QRegExp regexp(pattern, case_sense ? Qt::CaseSensitive : Qt::CaseInsensitive);

                QStringList replaceList = replaceInList(m_valuemap.value(varName), regexp, replace,
                                                        global);
                // Add changed entries to list
                foreach (const QString &entry, replaceList)
                    if (!m_valuemap.value(varName).contains(entry))
                        insertUnique(&m_valuemap, varName, QStringList() << entry, false);

                replaceList = replaceInList(m_filevaluemap[currentProFile()].value(varName), regexp,
                                      replace, global);
                foreach (const QString &entry, replaceList)
                    if (!m_filevaluemap[currentProFile()].value(varName).contains(entry))
                        insertUnique(&m_filevaluemap[currentProFile()], varName,
                                     QStringList() << entry, false); */
            }
            break;

    }
    return true;
}

bool ProFileEvaluator::Private::visitProFunction(ProFunction *func)
{
    m_lineNo = func->lineNumber();
    bool result = true;
    bool ok = true;
    QString text = func->text();
    int lparen = text.indexOf(QLatin1Char('('));
    int rparen = text.lastIndexOf(QLatin1Char(')'));
    QTC_ASSERT(lparen < rparen, return false);

    QString arguments = text.mid(lparen + 1, rparen - lparen - 1);
    QString funcName = text.left(lparen);
    ok &= evaluateConditionalFunction(funcName.trimmed(), arguments, &result);
    return ok;
}


QStringList ProFileEvaluator::Private::qmakeFeaturePaths()
{
    QStringList concat;
    {
        const QString base_concat = QDir::separator() + QString(QLatin1String("features"));
        concat << base_concat + QDir::separator() + QLatin1String("mac");
        concat << base_concat + QDir::separator() + QLatin1String("macx");
        concat << base_concat + QDir::separator() + QLatin1String("unix");
        concat << base_concat + QDir::separator() + QLatin1String("win32");
        concat << base_concat + QDir::separator() + QLatin1String("mac9");
        concat << base_concat + QDir::separator() + QLatin1String("qnx6");
        concat << base_concat;
    }
    const QString mkspecs_concat = QDir::separator() + QString(QLatin1String("mkspecs"));
    QStringList feature_roots;
    QByteArray mkspec_path = qgetenv("QMAKEFEATURES");
    if (!mkspec_path.isNull())
        feature_roots += splitPathList(QString::fromLocal8Bit(mkspec_path));
    /*
    if (prop)
        feature_roots += splitPathList(prop->value("QMAKEFEATURES"));
    if (!Option::mkfile::cachefile.isEmpty()) {
        QString path;
        int last_slash = Option::mkfile::cachefile.lastIndexOf(Option::dir_sep);
        if (last_slash != -1)
            path = Option::fixPathToLocalOS(Option::mkfile::cachefile.left(last_slash));
        foreach (const QString &concat_it, concat)
            feature_roots << (path + concat_it);
    }
    */

    QByteArray qmakepath = qgetenv("QMAKEPATH");
    if (!qmakepath.isNull()) {
        const QStringList lst = splitPathList(QString::fromLocal8Bit(qmakepath));
        foreach (const QString &item, lst) {
            foreach (const QString &concat_it, concat)
                feature_roots << (item + mkspecs_concat + concat_it);
        }
    }
    //if (!Option::mkfile::qmakespec.isEmpty())
    //    feature_roots << Option::mkfile::qmakespec + QDir::separator() + "features";
    //if (!Option::mkfile::qmakespec.isEmpty()) {
    //    QFileInfo specfi(Option::mkfile::qmakespec);
    //    QDir specdir(specfi.absoluteFilePath());
    //    while (!specdir.isRoot()) {
    //        if (!specdir.cdUp() || specdir.isRoot())
    //            break;
    //        if (QFile::exists(specdir.path() + QDir::separator() + "features")) {
    //            foreach (const QString &concat_it, concat) 
    //                feature_roots << (specdir.path() + concat_it);
    //            break;
    //        }
    //    }
    //}
    foreach (const QString &concat_it, concat)
        feature_roots << (propertyValue(QLatin1String("QT_INSTALL_PREFIX")) +
                          mkspecs_concat + concat_it);
    foreach (const QString &concat_it, concat)
        feature_roots << (propertyValue(QLatin1String("QT_INSTALL_DATA")) +
                          mkspecs_concat + concat_it);
    return feature_roots;
}

QString ProFileEvaluator::Private::propertyValue(const QString &name) const
{
    if (m_properties.contains(name))
        return m_properties.value(name);
    if (name == QLatin1String("QT_INSTALL_PREFIX"))
        return QLibraryInfo::location(QLibraryInfo::PrefixPath);
    if (name == QLatin1String("QT_INSTALL_DATA"))
        return QLibraryInfo::location(QLibraryInfo::DataPath);
    if (name == QLatin1String("QT_INSTALL_DOCS"))
        return QLibraryInfo::location(QLibraryInfo::DocumentationPath);
    if (name == QLatin1String("QT_INSTALL_HEADERS"))
        return QLibraryInfo::location(QLibraryInfo::HeadersPath);
    if (name == QLatin1String("QT_INSTALL_LIBS"))
        return QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    if (name == QLatin1String("QT_INSTALL_BINS"))
        return QLibraryInfo::location(QLibraryInfo::BinariesPath);
    if (name == QLatin1String("QT_INSTALL_PLUGINS"))
        return QLibraryInfo::location(QLibraryInfo::PluginsPath);
    if (name == QLatin1String("QT_INSTALL_TRANSLATIONS"))
        return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    if (name == QLatin1String("QT_INSTALL_CONFIGURATION"))
        return QLibraryInfo::location(QLibraryInfo::SettingsPath);
    if (name == QLatin1String("QT_INSTALL_EXAMPLES"))
        return QLibraryInfo::location(QLibraryInfo::ExamplesPath);
    if (name == QLatin1String("QT_INSTALL_DEMOS"))
        return QLibraryInfo::location(QLibraryInfo::DemosPath);
    if (name == QLatin1String("QMAKE_MKSPECS"))
        return qmake_mkspec_paths().join(Option::dirlist_sep);
    if (name == QLatin1String("QMAKE_VERSION"))
        return QLatin1String("1.0");        //### FIXME
        //return qmake_version();
#ifdef QT_VERSION_STR
    if (name == QLatin1String("QT_VERSION"))
        return QLatin1String(QT_VERSION_STR);
#endif
    return QLatin1String("UNKNOWN");        //###
}

ProFile *ProFileEvaluator::Private::currentProFile() const
{
    if (m_profileStack.count() > 0)
        return m_profileStack.top();
    return 0;
}

QString ProFileEvaluator::Private::currentFileName() const
{
    ProFile *pro = currentProFile();
    if (pro)
        return pro->fileName();
    return QString();
}

QString ProFileEvaluator::Private::getcwd() const
{
    ProFile *cur = m_profileStack.top();
    QFileInfo fi(cur->fileName());
    return fi.absolutePath();
}

QStringList ProFileEvaluator::Private::expandVariableReferences(const QString &str)
{
    bool fOK;
    bool *ok = &fOK;
    QStringList ret;
    if (ok)
        *ok = true;
    if (str.isEmpty())
        return ret;

    const ushort LSQUARE = '[';
    const ushort RSQUARE = ']';
    const ushort LCURLY = '{';
    const ushort RCURLY = '}';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort DOLLAR = '$';
    const ushort BACKSLASH = '\\';
    const ushort UNDERSCORE = '_';
    const ushort DOT = '.';
    const ushort SPACE = ' ';
    const ushort TAB = '\t';

    const QChar *str_data = str.data();
    const int str_len = str.length();

    ushort term;
    QString var, args;

    int replaced = 0;
    QString current;
    for (int i = 0; i < str_len; ++i) {
        ushort c = str_data[i].unicode();
        const int start_var = i;
        if (c == BACKSLASH) {
            bool escape = false;
            const char *symbols = "[]{}()$\\";
            for (const char *s = symbols; *s; ++s) {
                if (str_data[i+1] == (ushort)*s) {
                    i++;
                    escape = true;
                    if (!(replaced++))
                        current = str.left(start_var);
                    current.append(str.at(i));
                    break;
                }
            }
            if (!escape && replaced)
                current.append(QChar(c));
            continue;
        }
        if (c == SPACE || c == TAB) {
            c = 0;
            if (!current.isEmpty()) {
                unquote(&current);
                ret.append(current);
                current.clear();
            }
        } else if (c == DOLLAR && str_len > i+2) {
            c = str_data[++i].unicode();
            if (c == DOLLAR) {
                term = 0;
                var.clear();
                args.clear();
                enum { VAR, ENVIRON, FUNCTION, PROPERTY } var_type = VAR;
                c = str_data[++i].unicode();
                if (c == LSQUARE) {
                    c = str_data[++i].unicode();
                    term = RSQUARE;
                    var_type = PROPERTY;
                } else if (c == LCURLY) {
                    c = str_data[++i].unicode();
                    var_type = VAR;
                    term = RCURLY;
                } else if (c == LPAREN) {
                    c = str_data[++i].unicode();
                    var_type = ENVIRON;
                    term = RPAREN;
                }
                while (1) {
                    if (!(c & (0xFF<<8)) &&
                       c != DOT && c != UNDERSCORE &&
                       (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9'))
                        break;
                    var.append(QChar(c));
                    if (++i == str_len)
                        break;
                    c = str_data[i].unicode();
                }
                if (var_type == VAR && c == LPAREN) {
                    var_type = FUNCTION;
                    int depth = 0;
                    while (1) {
                        if (++i == str_len)
                            break;
                        c = str_data[i].unicode();
                        if (c == LPAREN) {
                            depth++;
                        } else if (c == RPAREN) {
                            if (!depth)
                                break;
                            --depth;
                        }
                        args.append(QChar(c));
                    }
                    if (i < str_len-1)
                        c = str_data[++i].unicode();
                    else
                        c = 0;
                }
                if (term) {
                    if (c != term) {
                        q->logMessage(format("Missing %1 terminator [found %2]")
                            .arg(QChar(term)).arg(QChar(c)));
                        if (ok)
                            *ok = false;
                        return QStringList();
                    }
                    c = 0;
                } else if (i > str_len-1) {
                    c = 0;
                }

                QStringList replacement;
                if (var_type == ENVIRON) {
                    replacement << QString::fromLocal8Bit(qgetenv(var.toLocal8Bit().constData()));
                } else if (var_type == PROPERTY) {
                    replacement << propertyValue(var);
                    //if (prop)
                    //    replacement = QStringList(prop->value(var));
                } else if (var_type == FUNCTION) {
                    replacement << evaluateExpandFunction(var, args);
                } else if (var_type == VAR) {
                    replacement += values(var);
                }
                if (!(replaced++) && start_var)
                    current = str.left(start_var);
                if (!replacement.isEmpty()) {
                    /* If a list is beteen two strings make sure it expands in such a way
                     * that the string to the left is prepended to the first string and
                     * the string to the right is appended to the last string, example:
                     *  LIST = a b c
                     *  V3 = x/$$LIST/f.cpp
                     *  message($$member(V3,0))     # Outputs "x/a"
                     *  message($$member(V3,1))     # Outputs "b"
                     *  message($$member(V3,2))     # Outputs "c/f.cpp"
                     */
                    current.append(replacement.at(0));
                    for (int i = 1; i < replacement.count(); ++i) {
                        unquote(&current);
                        ret.append(current);
                        current = replacement.at(i);
                    }
                }
            } else {
                if (replaced)
                    current.append(QLatin1Char('$'));
            }
        }
        if (replaced && c != 0)
            current.append(QChar(c));
    }
    if (!replaced) {
        current = str;
        unquote(&current);
        ret.append(current);
    } else if (!current.isEmpty()) {
        unquote(&current);
        ret.append(current);
        q->logMessage(format("Project Parser [var replace]: %1 -> [%2]\n")
                    .arg(str).arg(ret.join(QLatin1String(","))));
    }
    return ret;
}

bool ProFileEvaluator::Private::isActiveConfig(const QString &config, bool regex)
{
    // magic types for easy flipping
    if (config == QLatin1String("true"))
        return true;
    if (config == QLatin1String("false"))
        return false;

    // mkspecs
    if ((Option::target_mode == Option::TARG_MACX_MODE
            || Option::target_mode == Option::TARG_QNX6_MODE
            || Option::target_mode == Option::TARG_UNIX_MODE)
          && config == QLatin1String("unix"))
        return true;
    if (Option::target_mode == Option::TARG_MACX_MODE && config == QLatin1String("macx"))
        return true;
    if (Option::target_mode == Option::TARG_QNX6_MODE && config == QLatin1String("qnx6"))
        return true;
    if (Option::target_mode == Option::TARG_MAC9_MODE && config == QLatin1String("mac9"))
        return true;
    if ((Option::target_mode == Option::TARG_MAC9_MODE
            || Option::target_mode == Option::TARG_MACX_MODE)
          && config == QLatin1String("mac"))
        return true;
    if (Option::target_mode == Option::TARG_WIN_MODE && config == QLatin1String("win32"))
        return true;

    QRegExp re(config, Qt::CaseSensitive, QRegExp::Wildcard);
    QString spec = Option::qmakespec;
    if ((regex && re.exactMatch(spec)) || (!regex && spec == config))
        return true;

    return false;
}

QStringList ProFileEvaluator::Private::evaluateExpandFunction(const QString &func, const QString &arguments)
{
    QStringList argumentsList = split_arg_list(arguments);
    QStringList args;
    for (int i = 0; i < argumentsList.count(); ++i)
        args += expandVariableReferences(argumentsList[i]);

    enum ExpandFunc { E_MEMBER=1, E_FIRST, E_LAST, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
                      E_SPRINTF, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
                      E_FIND, E_SYSTEM, E_UNIQUE, E_QUOTE, E_ESCAPE_EXPAND,
                      E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE,
                      E_REPLACE };

    static QHash<QString, int> *expands = 0;
    if (!expands) {
        expands = new QHash<QString, int>;
        expands->insert(QLatin1String("member"), E_MEMBER);                //v (implemented)
        expands->insert(QLatin1String("first"), E_FIRST);                  //v
        expands->insert(QLatin1String("last"), E_LAST);                    //v
        expands->insert(QLatin1String("cat"), E_CAT);
        expands->insert(QLatin1String("fromfile"), E_FROMFILE);
        expands->insert(QLatin1String("eval"), E_EVAL);
        expands->insert(QLatin1String("list"), E_LIST);
        expands->insert(QLatin1String("sprintf"), E_SPRINTF);
        expands->insert(QLatin1String("join"), E_JOIN);                    //v
        expands->insert(QLatin1String("split"), E_SPLIT);                  //v
        expands->insert(QLatin1String("basename"), E_BASENAME);            //v
        expands->insert(QLatin1String("dirname"), E_DIRNAME);              //v
        expands->insert(QLatin1String("section"), E_SECTION);
        expands->insert(QLatin1String("find"), E_FIND);
        expands->insert(QLatin1String("system"), E_SYSTEM);                //v
        expands->insert(QLatin1String("unique"), E_UNIQUE);
        expands->insert(QLatin1String("quote"), E_QUOTE);                  //v
        expands->insert(QLatin1String("escape_expand"), E_ESCAPE_EXPAND);
        expands->insert(QLatin1String("upper"), E_UPPER);
        expands->insert(QLatin1String("lower"), E_LOWER);
        expands->insert(QLatin1String("re_escape"), E_RE_ESCAPE);
        expands->insert(QLatin1String("files"), E_FILES);
        expands->insert(QLatin1String("prompt"), E_PROMPT);
        expands->insert(QLatin1String("replace"), E_REPLACE);
    }
    ExpandFunc func_t = ExpandFunc(expands->value(func.toLower()));

    QStringList ret;

    switch (func_t) {
        case E_BASENAME:
        case E_DIRNAME:
        case E_SECTION: {
            bool regexp = false;
            QString sep, var;
            int beg = 0;
            int end = -1;
            if (func_t == E_SECTION) {
                if (args.count() != 3 && args.count() != 4) {
                    q->logMessage(format("%1(var) section(var, sep, begin, end) "
                        "requires three arguments.").arg(func));
                } else {
                    var = args[0];
                    sep = args[1];
                    beg = args[2].toInt();
                    if (args.count() == 4)
                        end = args[3].toInt();
                }
            } else {
                if (args.count() != 1) {
                    q->logMessage(format("%1(var) requires one argument.").arg(func));
                } else {
                    var = args[0];
                    regexp = true;
                    sep = QLatin1String("[\\\\/]");
                    if (func_t == E_DIRNAME)
                        end = -2;
                    else
                        beg = -1;
                }
            }
            if (!var.isNull()) {
                foreach (const QString str, values(var)) {
                    if (regexp)
                        ret += str.section(QRegExp(sep), beg, end);
                    else
                        ret += str.section(sep, beg, end);
                }
            }
            break;
        }
        case E_JOIN: {
            if (args.count() < 1 || args.count() > 4) {
                q->logMessage(format("join(var, glue, before, after) requires four arguments."));
            } else {
                QString glue, before, after;
                if (args.count() >= 2)
                    glue = args[1];
                if (args.count() >= 3)
                    before = args[2];
                if (args.count() == 4)
                    after = args[3];
                const QStringList &var = values(args.first());
                if (!var.isEmpty())
                    ret.append(before + var.join(glue) + after);
            }
            break;
        }
        case E_SPLIT: {
            if (args.count() < 2 || args.count() > 2) {
                q->logMessage(format("split(var, sep) requires two arguments"));
            } else {
                QString sep = args.at(1);
                foreach (const QString &var, values(args.first()))
                    foreach (const QString &splt, var.split(sep))
                        ret.append(splt);
            }
            break;
        }
        case E_MEMBER: {
            if (args.count() < 1 || args.count() > 3) {
                q->logMessage(format("member(var, start, end) requires three arguments."));
            } else {
                bool ok = true;
                const QStringList var = values(args.first());
                int start = 0, end = 0;
                if (args.count() >= 2) {
                    QString start_str = args[1];
                    start = start_str.toInt(&ok);
                    if (!ok) {
                        if (args.count() == 2) {
                            int dotdot = start_str.indexOf(QLatin1String(".."));
                            if (dotdot != -1) {
                                start = start_str.left(dotdot).toInt(&ok);
                                if (ok)
                                    end = start_str.mid(dotdot+2).toInt(&ok);
                            }
                        }
                        if (!ok)
                            q->logMessage(format("member() argument 2 (start) '%2' invalid.")
                                .arg(start_str));
                    } else {
                        end = start;
                        if (args.count() == 3)
                            end = args[2].toInt(&ok);
                        if (!ok)
                            q->logMessage(format("member() argument 3 (end) '%2' invalid.\n")
                                .arg(args[2]));
                    }
                }
                if (ok) {
                    if (start < 0)
                        start += var.count();
                    if (end < 0)
                        end += var.count();
                    if (start < 0 || start >= var.count() || end < 0 || end >= var.count()) {
                        //nothing
                    } else if (start < end) {
                        for (int i = start; i <= end && var.count() >= i; i++)
                            ret.append(var[i]);
                    } else {
                        for (int i = start; i >= end && var.count() >= i && i >= 0; i--)
                            ret += var[i];
                    }
                }
            }
            break;
        }
        case E_FIRST:
        case E_LAST: {
            if (args.count() != 1) {
                q->logMessage(format("%1(var) requires one argument.").arg(func));
            } else {
                const QStringList var = values(args.first());
                if (!var.isEmpty()) {
                    if (func_t == E_FIRST)
                        ret.append(var[0]);
                    else
                        ret.append(var.last());
                }
            }
            break;
        }
        case E_SYSTEM: {
            if (m_condition) {
                if (args.count() < 1 || args.count() > 2) {
                    q->logMessage(format("system(execute) requires one or two arguments."));
                } else {
                    char buff[256];
                    FILE *proc = QT_POPEN(args[0].toLatin1(), "r");
                    bool singleLine = true;
                    if (args.count() > 1)
                        singleLine = (args[1].toLower() == QLatin1String("true"));
                    QString output;
                    while (proc && !feof(proc)) {
                        int read_in = int(fread(buff, 1, 255, proc));
                        if (!read_in)
                            break;
                        for (int i = 0; i < read_in; i++) {
                            if ((singleLine && buff[i] == '\n') || buff[i] == '\t')
                                buff[i] = ' ';
                        }
                        buff[read_in] = '\0';
                        output += QLatin1String(buff);
                    }
                    ret += split_value_list(output);
                }
            }
            break; }
        case E_QUOTE:
            for (int i = 0; i < args.count(); ++i)
                ret += QStringList(args.at(i));
            break;
        case 0:
            q->logMessage(format("'%1' is not a function").arg(func));
            break;
        default:
            q->logMessage(format("Function '%1' is not implemented").arg(func));
            break;
    }

    return ret;
}

bool ProFileEvaluator::Private::evaluateConditionalFunction(const QString &function,
    const QString &arguments, bool *result)
{
    QStringList argumentsList = split_arg_list(arguments);
    QString sep;
    sep.append(Option::field_sep);

    QStringList args;
    for (int i = 0; i < argumentsList.count(); ++i)
        args += expandVariableReferences(argumentsList[i]).join(sep);

    enum ConditionFunc { CF_CONFIG = 1, CF_CONTAINS, CF_COUNT, CF_EXISTS, CF_INCLUDE,
        CF_LOAD, CF_ISEMPTY, CF_SYSTEM, CF_MESSAGE};

    static QHash<QString, int> *functions = 0;
    if (!functions) {
        functions = new QHash<QString, int>;
        functions->insert(QLatin1String("load"), CF_LOAD);         //v
        functions->insert(QLatin1String("include"), CF_INCLUDE);   //v
        functions->insert(QLatin1String("message"), CF_MESSAGE);   //v
        functions->insert(QLatin1String("warning"), CF_MESSAGE);   //v
        functions->insert(QLatin1String("error"), CF_MESSAGE);     //v
    }

    bool cond = false;
    bool ok = true;

    ConditionFunc func_t = (ConditionFunc)functions->value(function);

    switch (func_t) {
        case CF_CONFIG: {
            if (args.count() < 1 || args.count() > 2) {
                q->logMessage(format("CONFIG(config) requires one or two arguments."));
                ok = false;
                break;
            }
            if (args.count() == 1) {
                //cond = isActiveConfig(args.first());
                break;
            }
            const QStringList mutuals = args[1].split(QLatin1Char('|'));
            const QStringList &configs = m_valuemap.value(QLatin1String("CONFIG"));
            for (int i = configs.size() - 1 && ok; i >= 0; i--) {
                for (int mut = 0; mut < mutuals.count(); mut++) {
                    if (configs[i] == mutuals[mut].trimmed()) {
                        cond = (configs[i] == args[0]);
                        break;
                    }
                }
            }
            break;
        }
        case CF_CONTAINS: {
            if (args.count() < 2 || args.count() > 3) {
                q->logMessage(format("contains(var, val) requires at least two arguments."));
                ok = false;
                break;
            }

            QRegExp regx(args[1]);
            const QStringList &l = values(args.first());
            if (args.count() == 2) {
                for (int i = 0; i < l.size(); ++i) {
                    const QString val = l[i];
                    if (regx.exactMatch(val) || val == args[1]) {
                        cond = true;
                        break;
                    }
                }
            } else {
                const QStringList mutuals = args[2].split(QLatin1Char('|'));
                for (int i = l.size() - 1; i >= 0; i--) {
                    const QString val = l[i];
                    for (int mut = 0; mut < mutuals.count(); mut++) {
                        if (val == mutuals[mut].trimmed()) {
                            cond = (regx.exactMatch(val) || val == args[1]);
                            break;
                        }
                    }
                }
            }

            break;
        }
        case CF_COUNT: {
            if (args.count() != 2 && args.count() != 3) {
                q->logMessage(format("count(var, count) requires at least two arguments."));
                ok = false;
                break;
            }
            if (args.count() == 3) {
                QString comp = args[2];
                if (comp == QLatin1String(">") || comp == QLatin1String("greaterThan")) {
                    cond = values(args.first()).count() > args[1].toInt();
                } else if (comp == QLatin1String(">=")) {
                    cond = values(args.first()).count() >= args[1].toInt();
                } else if (comp == QLatin1String("<") || comp == QLatin1String("lessThan")) {
                    cond = values(args.first()).count() < args[1].toInt();
                } else if (comp == QLatin1String("<=")) {
                    cond = values(args.first()).count() <= args[1].toInt();
                } else if (comp == QLatin1String("equals") || comp == QLatin1String("isEqual") || comp == QLatin1String("=") || comp == QLatin1String("==")) {
                    cond = values(args.first()).count() == args[1].toInt();
                } else {
                    ok = false;
                    q->logMessage(format("unexpected modifier to count(%2)").arg(comp));
                }
                break;
            }
            cond = values(args.first()).count() == args[1].toInt();
            break;
        }
        case CF_INCLUDE: {
            QString parseInto;
            if (args.count() == 2) {
                parseInto = args[1];
            } else if (args.count() != 1) {
                q->logMessage(format("include(file) requires one argument."));
                ok = false;
                break;
            }
            QString fileName = args.first();
            // ### this breaks if we have include(c:/reallystupid.pri) but IMHO that's really bad style.
            QDir currentProPath(getcwd());
            fileName = QDir::cleanPath(currentProPath.absoluteFilePath(fileName));
            ok = evaluateFile(fileName, &ok);
            break;
        }
        case CF_LOAD: {
            QString parseInto;
            bool ignore_error = false;
            if (args.count() == 2) {
                QString sarg = args[1];
                ignore_error = (sarg.toLower() == QLatin1String("true") || sarg.toInt());
            } else if (args.count() != 1) {
                q->logMessage(format("load(feature) requires one argument."));
                ok = false;
                break;
            }
            ok = evaluateFeatureFile( args.first(), &cond);
            break;
        }
        case CF_MESSAGE: {
            if (args.count() != 1) {
                q->logMessage(format("%1(message) requires one argument.").arg(function));
                ok = false;
                break;
            }
            QString msg = args.first();
            if (function == QLatin1String("error")) {
                QStringList parents;
                foreach (ProFile *proFile, m_profileStack)
                    parents.append(proFile->fileName());
                if (!parents.isEmpty())
                    parents.takeLast();
                if (parents.isEmpty()) 
                    q->fileMessage(format("Project ERROR: %1").arg(msg));
                else
                    q->fileMessage(format("Project ERROR: %1. File was included from: '%2'")
                        .arg(msg).arg(parents.join(QLatin1String("', '"))));
            } else {
                q->fileMessage(format("Project MESSAGE: %1").arg(msg));
            }
            break;
        }
        case CF_SYSTEM: {
            if (args.count() != 1) {
                q->logMessage(format("system(exec) requires one argument."));
                ok = false;
                break;
            }
            ok = system(args.first().toLatin1().constData()) == 0;
            break;
        }
        case CF_ISEMPTY: {
            if (args.count() != 1) {
                q->logMessage(format("isEmpty(var) requires one argument."));
                ok = false;
                break;
            }
            QStringList sl = values(args.first());
            if (sl.count() == 0) {
                cond = true;
            } else if (sl.count() > 0) {
                QString var = sl.first();
                cond = (var.isEmpty());
            }
            break;
        }
        case CF_EXISTS: {
            if (args.count() != 1) {
                q->logMessage(format("exists(file) requires one argument."));
                ok = false;
                break;
            }
            QString file = args.first();

            file = QDir::cleanPath(file);

            if (QFile::exists(file)) {
                cond = true;
                break;
            }
            //regular expression I guess
            QString dirstr = getcwd();
            int slsh = file.lastIndexOf(Option::dir_sep);
            if (slsh != -1) {
                dirstr = file.left(slsh+1);
                file = file.right(file.length() - slsh - 1);
            }
            cond = QDir(dirstr).entryList(QStringList(file)).count();

            break;
        }
    }

    if (result)
        *result = cond;

    return ok;
}

QStringList ProFileEvaluator::Private::values(const QString &variableName) const
{
    if (variableName == QLatin1String("TARGET")) {
        QStringList list = m_valuemap.value(variableName);
        if (!m_origfile.isEmpty())
            list.append(QFileInfo(m_origfile).baseName());
        return list;
    }
    if (variableName == QLatin1String("PWD")) {
        return QStringList(getcwd());
    }
    return m_valuemap.value(variableName);
}

QStringList ProFileEvaluator::Private::values(const QString &variableName, const ProFile *pro) const
{
    if (variableName == QLatin1String("TARGET")) {
        QStringList list = m_filevaluemap[pro].value(variableName);
        if (!m_origfile.isEmpty())
            list.append(QFileInfo(m_origfile).baseName());
        return list;
    }
    if (variableName == QLatin1String("PWD")) {
        return QStringList(QFileInfo(pro->fileName()).absoluteFilePath());
    }
    return m_filevaluemap[pro].value(variableName);
}

ProFile *ProFileEvaluator::parsedProFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (fi.exists()) {
        logMessage(d->format("Reading %1\n").arg(fileName));
        ProFile *pro = new ProFile(fi.absoluteFilePath());
        if (d->read(pro))
            return pro;
        delete pro;
    }
    return 0;
}

void ProFileEvaluator::releaseParsedProFile(ProFile *proFile)
{
    delete proFile;
}

bool ProFileEvaluator::Private::evaluateFile(const QString &fileName, bool *result)
{
    bool ok = true;
    ProFile *pro = q->parsedProFile(fileName);
    if (pro) {
        m_profileStack.push(pro);
        ok = (currentProFile() ? pro->Accept(this) : false);
        m_profileStack.pop();
        q->releaseParsedProFile(pro);

        if (result)
            *result = true;
    } else {
        if (result)
            *result = false;
    }
/*    if (ok && readFeatures) {
        QStringList configs = values("CONFIG");
        QSet<QString> processed;
        foreach (const QString &fn, configs) {
            if (!processed.contains(fn)) {
                processed.insert(fn);
                evaluateFeatureFile(fn, 0);
            }
        }
    } */

    return ok;
}

bool ProFileEvaluator::Private::evaluateFeatureFile(const QString &fileName, bool *result)
{
    QString fn;
    foreach (const QString &path, qmakeFeaturePaths()) {
        QString fname = path + QLatin1Char('/') + fileName;
        if (QFileInfo(fname).exists()) {
            fn = fname;
            break;
        }
        fname += QLatin1String(".prf");
        if (QFileInfo(fname).exists()) {
            fn = fname;
            break;
        }
    }
    return fn.isEmpty() ? false : evaluateFile(fn, result);
}

void ProFileEvaluator::Private::expandPatternHelper(const QString &relName, const QString &absName,
        QStringList &sources_out)
{
    const QStringList vpaths = values(QLatin1String("VPATH"))
        + values(QLatin1String("QMAKE_ABSOLUTE_SOURCE_PATH"))
        + values(QLatin1String("DEPENDPATH"))
        + values(QLatin1String("VPATH_SOURCES"));

    QFileInfo fi(absName);
    bool found = fi.exists();
    // Search in all vpaths
    if (!found) {
        foreach (const QString &vpath, vpaths) {
            fi.setFile(vpath + QDir::separator() + relName);
            if (fi.exists()) {
                found = true;
                break;
            }
        }
    }

    if (found) {
        sources_out += fi.absoluteFilePath(); // Not resolving symlinks
    } else {
        QString val = relName;
        QString dir;
        QString wildcard = val;
        QString real_dir;
        if (wildcard.lastIndexOf(QLatin1Char('/')) != -1) {
            dir = wildcard.left(wildcard.lastIndexOf(QLatin1Char('/')) + 1);
            real_dir = dir;
            wildcard = wildcard.right(wildcard.length() - dir.length());
        }

        if (real_dir.isEmpty() || QFileInfo(real_dir).exists()) {
            QStringList files = QDir(real_dir).entryList(QStringList(wildcard));
            if (files.isEmpty()) {
                q->logMessage(format("Failure to find %1").arg(val));
            } else {
                QString a;
                for (int i = files.count() - 1; i >= 0; --i) {
                    if (files[i] == QLatin1String(".") || files[i] == QLatin1String(".."))
                        continue;
                    a = dir + files[i];
                    sources_out += a;
                }
            }
        } else {
            q->logMessage(format("Cannot match %1/%2, as %3 does not exist.")
                .arg(real_dir).arg(wildcard).arg(real_dir));
        }
    }
}


/*
 * Lookup of files are done in this order:
 *  1. look in pwd
 *  2. look in vpaths
 *  3. expand wild card files relative from the profiles folder
 **/

// FIXME: This code supports something that I'd consider a flaw in .pro file syntax
// which is not even documented. So arguably this can be ditched completely...
QStringList ProFileEvaluator::Private::expandPattern(const QString& pattern)
{
    if (!currentProFile())
        return QStringList();

    QStringList sources_out;
    const QString absName = QDir::cleanPath(QDir::current().absoluteFilePath(pattern));

    expandPatternHelper(pattern, absName, sources_out);
    return sources_out;
}

QString ProFileEvaluator::Private::format(const char *fmt) const
{
    ProFile *pro = currentProFile();
    QString fileName = pro ? pro->fileName() : QLatin1String("Not a file");
    int lineNumber = pro ? m_lineNo : 0;
    return QString::fromLatin1("%1(%2):").arg(fileName).arg(lineNumber) + QString::fromAscii(fmt);
}


///////////////////////////////////////////////////////////////////////
//
// ProFileEvaluator
//
///////////////////////////////////////////////////////////////////////

ProFileEvaluator::ProFileEvaluator()
  : d(new Private(this))
{
    Option::init();
}

ProFileEvaluator::~ProFileEvaluator()
{
    delete d;
}

bool ProFileEvaluator::contains(const QString &variableName) const
{
    return d->m_valuemap.contains(variableName);
}

QStringList ProFileEvaluator::values(const QString &variableName) const
{
    return d->values(variableName);
}

QStringList ProFileEvaluator::values(const QString &variableName, const ProFile *pro) const
{
    return d->values(variableName, pro);
}

ProFileEvaluator::TemplateType ProFileEvaluator::templateType()
{
    QStringList templ = d->m_valuemap.value(QLatin1String("TEMPLATE"));
    if (templ.count() >= 1) {
        QString t = templ.last().toLower();
        if (t == QLatin1String("app"))
            return TT_Application;
        if (t == QLatin1String("lib"))
            return TT_Library;
        if (t == QLatin1String("script"))
            return TT_Script;
        if (t == QLatin1String("subdirs"))
            return TT_Subdirs;
    }
    return TT_Unknown;
}

bool ProFileEvaluator::queryProFile(ProFile *pro)
{
    return d->read(pro);
}

bool ProFileEvaluator::accept(ProFile *pro)
{
    return pro->Accept(d);
}

QString ProFileEvaluator::propertyValue(const QString &name) const
{
    return d->propertyValue(name);
}

namespace {
    template<class K, class T> void insert(QHash<K,T> *out, const QHash<K,T> &in)
    {
        typename QHash<K,T>::const_iterator i = in.begin();
        while (i != in.end()) {
            out->insert(i.key(), i.value());
            ++i;
        }
    }
} // anon namespace

void ProFileEvaluator::addVariables(const QHash<QString, QStringList> &variables)
{
    insert(&(d->m_valuemap), variables);
}

void ProFileEvaluator::addProperties(const QHash<QString, QString> &properties)
{
    insert(&(d->m_properties), properties);
}

void ProFileEvaluator::logMessage(const QString &message)
{
    if (d->m_verbose)
        qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::fileMessage(const QString &message)
{
    qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::errorMessage(const QString &message)
{
    qWarning("%s", qPrintable(message));
}

// This function is unneeded and still retained. See log message for reason.
QStringList ProFileEvaluator::absFileNames(const QString &variableName)
{
    QStringList sources_out;
    QFileInfo fi(d->m_origfile);
    QDir dir(fi.absoluteDir());
    foreach (const QString &fn, values(variableName)) {
        const QString absName = QDir::cleanPath(dir.absoluteFilePath(fn));
        d->expandPatternHelper(fn, absName, sources_out);
    }

    return sources_out;
}

void ProFileEvaluator::setVerbose(bool on)
{
    d->m_verbose = on;
}

bool evaluateProFile(const QString &fileName, bool verbose, QHash<QByteArray, QStringList> *varMap)
{
    QStringList sourceFiles;
    QString codecForTr;
    QString codecForSource;
    QStringList tsFileNames;

    QFileInfo fi(fileName);
    QDir rootPath;
    if (!fi.exists())
        return false;

    rootPath.setPath(fi.absolutePath());
    ProFile pro(fi.absoluteFilePath());

    ProFileEvaluator visitor;
    visitor.setVerbose(verbose);

    if (!visitor.queryProFile(&pro))
        return false;

    if (!visitor.accept(&pro))
        return false;

    // app/lib template
    sourceFiles += visitor.values(QLatin1String("SOURCES"));
    sourceFiles += visitor.values(QLatin1String("HEADERS"));
    tsFileNames = visitor.values(QLatin1String("TRANSLATIONS"));
    QStringList trcodec = visitor.values(QLatin1String("CODEC"))
        + visitor.values(QLatin1String("DEFAULTCODEC"))
        + visitor.values(QLatin1String("CODECFORTR"));

    if (!trcodec.isEmpty())
        codecForTr = trcodec.last();

    QStringList srccodec = visitor.values(QLatin1String("CODECFORSRC"));
    if (!srccodec.isEmpty())
        codecForSource = srccodec.last();

    QStringList forms = visitor.values(QLatin1String("INTERFACES"))
        + visitor.values(QLatin1String("FORMS"))
        + visitor.values(QLatin1String("FORMS3"));
    sourceFiles << forms;

#if QT_VERSION >= 0x040500
    sourceFiles.sort();
    sourceFiles.removeDuplicates();
    tsFileNames.sort();
    tsFileNames.removeDuplicates();
#else
    sourceFiles = sourceFiles.toSet().toList();
    sourceFiles.sort();
    tsFileNames = tsFileNames.toSet().toList();
    tsFileNames.sort();
#endif

    varMap->insert("SOURCES", sourceFiles);
    varMap->insert("CODECFORTR", QStringList() << codecForTr);
    varMap->insert("CODECFORSRC", QStringList() << codecForSource);
    varMap->insert("TRANSLATIONS", tsFileNames);
    return true;
}

QT_END_NAMESPACE
