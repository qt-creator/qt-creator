/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "profileevaluator.h"
#include "proparserutils.h"
#include "proitems.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
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

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/utsname.h>
#else
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_PCLOSE pclose
#endif

QT_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////
//
// Option
//
///////////////////////////////////////////////////////////////////////

QString
Option::fixString(QString string, uchar flags)
{
    // XXX Ripped out caching, so this will be slow. Should not matter for current uses.

    //fix the environment variables
    if (flags & Option::FixEnvVars) {
        int rep;
        QRegExp reg_variableName(QLatin1String("\\$\\(.*\\)"));
        reg_variableName.setMinimal(true);
        while ((rep = reg_variableName.indexIn(string)) != -1)
            string.replace(rep, reg_variableName.matchedLength(),
                           QString::fromLocal8Bit(qgetenv(string.mid(rep + 2, reg_variableName.matchedLength() - 3).toLatin1().constData()).constData()));
    }

    //canonicalize it (and treat as a path)
    if (flags & Option::FixPathCanonicalize) {
#if 0
        string = QFileInfo(string).canonicalFilePath();
#endif
        string = QDir::cleanPath(string);
    }

    if (string.length() > 2 && string[0].isLetter() && string[1] == QLatin1Char(':'))
        string[0] = string[0].toLower();

    //fix separators
    Q_ASSERT(!((flags & Option::FixPathToLocalSeparators) && (flags & Option::FixPathToTargetSeparators)));
    if (flags & Option::FixPathToLocalSeparators) {
#if defined(Q_OS_WIN32)
        string = string.replace(QLatin1Char('/'), QLatin1Char('\\'));
#else
        string = string.replace(QLatin1Char('\\'), QLatin1Char('/'));
#endif
    } else if (flags & Option::FixPathToTargetSeparators) {
        string = string.replace(QLatin1Char('/'), Option::dir_sep)
                       .replace(QLatin1Char('\\'), Option::dir_sep);
    }

    if ((string.startsWith(QLatin1Char('"')) && string.endsWith(QLatin1Char('"'))) ||
        (string.startsWith(QLatin1Char('\'')) && string.endsWith(QLatin1Char('\''))))
        string = string.mid(1, string.length() - 2);

    return string;
}

///////////////////////////////////////////////////////////////////////
//
// ProFileEvaluator::Private
//
///////////////////////////////////////////////////////////////////////

class ProFileEvaluator::Private : public AbstractProItemVisitor
{
public:
    Private(ProFileEvaluator *q_);

    ProFileEvaluator *q;
    int m_lineNo;                                   // Error reporting
    bool m_verbose;

    /////////////// Reading pro file

    bool read(ProFile *pro);

    ProBlock *currentBlock();
    void updateItem();
    bool parseLine(const QString &line);
    void insertVariable(const QString &line, int *i);
    void insertOperator(const char op);
    void insertComment(const QString &comment);
    void enterScope(bool multiLine);
    void leaveScope();
    void finalizeBlock();

    QStack<ProBlock *> m_blockstack;
    ProBlock *m_block;

    ProItem *m_commentItem;
    QString m_proitem;
    QString m_pendingComment;
    bool m_syntaxError;
    bool m_contNextLine;
    bool m_inQuote;
    int m_parens;

    /////////////// Evaluating pro file contents

    // implementation of AbstractProItemVisitor
    ProItem::ProItemReturn visitBeginProBlock(ProBlock *block);
    void visitEndProBlock(ProBlock *block);
    ProItem::ProItemReturn visitProLoopIteration();
    void visitProLoopCleanup();
    void visitBeginProVariable(ProVariable *variable);
    void visitEndProVariable(ProVariable *variable);
    ProItem::ProItemReturn visitBeginProFile(ProFile *value);
    ProItem::ProItemReturn visitEndProFile(ProFile *value);
    void visitProValue(ProValue *value);
    ProItem::ProItemReturn visitProFunction(ProFunction *function);
    void visitProOperator(ProOperator *oper);
    void visitProCondition(ProCondition *condition);

    QStringList valuesDirect(const QString &variableName) const { return m_valuemap[variableName]; }
    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QStringList values(const QString &variableName, const QHash<QString, QStringList> &place,
                       const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

    bool isActiveConfig(const QString &config, bool regex = false);
    QStringList expandPattern(const QString &pattern);
    void expandPatternHelper(const QString &relName, const QString &absName,
        QStringList &sources_out);
    QStringList expandVariableReferences(const QString &value);
    void doVariableReplace(QString *str);
    QStringList evaluateExpandFunction(const QString &function, const QString &arguments);
    QString format(const char *format) const;

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;

    ProItem::ProItemReturn evaluateConditionalFunction(const QString &function, const QString &arguments);
    bool evaluateFile(const QString &fileName);
    bool evaluateFeatureFile(const QString &fileName);

    static inline ProItem::ProItemReturn returnBool(bool b)
        { return b ? ProItem::ReturnTrue : ProItem::ReturnFalse; }

    QStringList evaluateFunction(ProBlock *funcPtr, const QStringList &argumentsList, bool *ok);

    QStringList qmakeFeaturePaths();

    struct State {
        bool condition;
        bool prevCondition;
    } m_sts;
    bool m_invertNext; // Short-lived, so not in State
    int m_skipLevel;
    bool m_cumulative;
    bool m_isFirstVariableValue;
    QString m_lastVarName;
    ProVariable::VariableOperator m_variableOperator;
    QString m_origfile;
    QString m_oldPath;                              // To restore the current path to the path
    QStack<ProFile*> m_profileStack;                // To handle 'include(a.pri), so we can track back to 'a.pro' when finished with 'a.pri'
    struct ProLoop {
        QString variable;
        QStringList oldVarVal;
        QStringList list;
        int index;
        bool infinite;
    };
    QStack<ProLoop> m_loopStack;

    // we need the following two variables for handling
    // CONFIG = foo bar $$CONFIG
    QHash<QString, QStringList> m_tempValuemap;         // used while evaluating (variable operator value1 value2 ...)
    QHash<const ProFile*, QHash<QString, QStringList> > m_tempFilevaluemap; // used while evaluating (variable operator value1 value2 ...)

    QHash<QString, QStringList> m_valuemap;         // VariableName must be us-ascii, the content however can be non-us-ascii.
    QHash<const ProFile*, QHash<QString, QStringList> > m_filevaluemap; // Variables per include file
    QHash<QString, QString> m_properties;
    QString m_outputDir;

    bool m_definingTest;
    QString m_definingFunc;
    QHash<QString, ProBlock *> m_testFunctions;
    QHash<QString, ProBlock *> m_replaceFunctions;
    QStringList m_returnValue;
    QStack<QHash<QString, QStringList> > m_valuemapStack;
    QStack<QHash<const ProFile*, QHash<QString, QStringList> > > m_filevaluemapStack;

    int m_prevLineNo;                               // Checking whether we're assigning the same TARGET
    ProFile *m_prevProFile;                         // See m_prevLineNo
    QStringList m_addUserConfigCmdArgs;
    QStringList m_removeUserConfigCmdArgs;
    bool m_parsePreAndPostFiles;
};

#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::State, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::ProLoop, Q_MOVABLE_TYPE);
#endif

ProFileEvaluator::Private::Private(ProFileEvaluator *q_)
  : q(q_)
{
    // Global parser state
    m_prevLineNo = 0;
    m_prevProFile = 0;

    // Configuration, more or less
    m_verbose = true;
    m_cumulative = true;
    m_parsePreAndPostFiles = true;

    // Evaluator state
    m_sts.condition = false;
    m_sts.prevCondition = false;
    m_invertNext = false;
    m_skipLevel = 0;
    m_isFirstVariableValue = true;
    m_definingFunc.clear();
}

bool ProFileEvaluator::Private::read(ProFile *pro)
{
    QFile file(pro->fileName());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        q->errorMessage(format("%1 not readable.").arg(pro->fileName()));
        return false;
    }

    // Parser state
    m_block = 0;
    m_commentItem = 0;
    m_inQuote = false;
    m_parens = 0;
    m_contNextLine = false;
    m_syntaxError = false;
    m_lineNo = 1;
    m_blockstack.clear();
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

    int parens = m_parens;
    bool inQuote = m_inQuote;
    bool escaped = false;
    QString line = line0.simplified();

    for (int i = 0; !m_syntaxError && i < line.length(); ++i) {
        ushort c = line.at(i).unicode();
        if (c == '#') { // Yep - no escaping possible
            insertComment(line.mid(i + 1));
            escaped = m_contNextLine;
            break;
        }
        if (!escaped) {
            if (c == '\\') {
                escaped = true;
                m_proitem += c;
                continue;
            } else if (c == '"') {
                inQuote = !inQuote;
                m_proitem += c;
                continue;
            }
        } else {
            escaped = false;
        }
        if (!inQuote) {
            if (c == '(') {
                ++parens;
            } else if (c == ')') {
                --parens;
            } else if (!parens) {
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
        }

        m_proitem += c;
    }
    m_inQuote = inQuote;
    m_parens = parens;
    m_contNextLine = escaped;
    if (escaped) {
        m_proitem.chop(1);
        updateItem();
        return true;
    } else {
        if (!m_syntaxError) {
            updateItem();
            finalizeBlock();
            return true;
        }
        return false;
    }
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

    if (m_proitem.isEmpty()) // Line starting with '=', like a conflict marker
        return;

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


ProItem::ProItemReturn ProFileEvaluator::Private::visitBeginProBlock(ProBlock *block)
{
    if (block->blockKind() & ProBlock::ScopeContentsKind) {
        if (!m_definingFunc.isEmpty()) {
            if (!m_skipLevel || m_cumulative) {
                QHash<QString, ProBlock *> *hash =
                        (m_definingTest ? &m_testFunctions : &m_replaceFunctions);
                if (ProBlock *def = hash->value(m_definingFunc))
                    def->deref();
                hash->insert(m_definingFunc, block);
                block->ref();
                block->setBlockKind(block->blockKind() | ProBlock::FunctionBodyKind);
            }
            m_definingFunc.clear();
            return ProItem::ReturnSkip;
        } else if (!(block->blockKind() & ProBlock::FunctionBodyKind)) {
            if (!m_sts.condition)
                ++m_skipLevel;
            else
                Q_ASSERT(!m_skipLevel);
        }
    } else {
        if (!m_skipLevel) {
            if (m_sts.condition) {
                m_sts.prevCondition = true;
                m_sts.condition = false;
            }
        } else {
            Q_ASSERT(!m_sts.condition);
        }
    }
    return ProItem::ReturnTrue;
}

void ProFileEvaluator::Private::visitEndProBlock(ProBlock *block)
{
    if ((block->blockKind() & ProBlock::ScopeContentsKind)
        && !(block->blockKind() & ProBlock::FunctionBodyKind)) {
        if (m_skipLevel) {
            Q_ASSERT(!m_sts.condition);
            --m_skipLevel;
        } else if (!(block->blockKind() & ProBlock::SingleLine)) {
            // Conditionals contained inside this block may have changed the state.
            // So we reset it here to make an else following us do the right thing.
            m_sts.condition = true;
        }
    }
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitProLoopIteration()
{
    ProLoop &loop = m_loopStack.top();

    if (loop.infinite) {
        if (!loop.variable.isEmpty())
            m_valuemap[loop.variable] = QStringList(QString::number(loop.index++));
        if (loop.index > 1000) {
            q->errorMessage(format("ran into infinite loop (> 1000 iterations)."));
            return ProItem::ReturnFalse;
        }
    } else {
        QString val;
        do {
            if (loop.index >= loop.list.count())
                return ProItem::ReturnFalse;
            val = loop.list.at(loop.index++);
        } while (val.isEmpty()); // stupid, but qmake is like that
        m_valuemap[loop.variable] = QStringList(val);
    }
    return ProItem::ReturnTrue;
}

void ProFileEvaluator::Private::visitProLoopCleanup()
{
    ProLoop &loop = m_loopStack.top();
    m_valuemap[loop.variable] = loop.oldVarVal;
    m_loopStack.pop_back();
}

void ProFileEvaluator::Private::visitBeginProVariable(ProVariable *variable)
{
    m_lastVarName = variable->variable();
    m_variableOperator = variable->variableOperator();
    m_isFirstVariableValue = true;
    m_tempValuemap = m_valuemap;
    m_tempFilevaluemap = m_filevaluemap;
}

void ProFileEvaluator::Private::visitEndProVariable(ProVariable *variable)
{
    Q_UNUSED(variable);
    m_valuemap = m_tempValuemap;
    m_filevaluemap = m_tempFilevaluemap;
    m_lastVarName.clear();
}

void ProFileEvaluator::Private::visitProOperator(ProOperator *oper)
{
    m_invertNext = (oper->operatorKind() == ProOperator::NotOperator);
}

void ProFileEvaluator::Private::visitProCondition(ProCondition *cond)
{
    if (!m_skipLevel) {
        if (!cond->text().compare(QLatin1String("else"), Qt::CaseInsensitive)) {
            m_sts.condition = !m_sts.prevCondition;
        } else {
            m_sts.prevCondition = false;
            if (!m_sts.condition && isActiveConfig(cond->text(), true) ^ m_invertNext)
                m_sts.condition = true;
        }
    }
    m_invertNext = false;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitBeginProFile(ProFile * pro)
{
    PRE(pro);
    m_lineNo = pro->lineNumber();
    if (m_origfile.isEmpty())
        m_origfile = pro->fileName();
    if (m_oldPath.isEmpty()) {
        // change the working directory for the initial profile we visit, since
        // that is *the* profile. All the other times we reach this function will be due to
        // include(file) or load(file)

        m_oldPath = QDir::currentPath();

        m_profileStack.push(pro);

        const QString mkspecDirectory = propertyValue(QLatin1String("QMAKE_MKSPECS"));
        if (!mkspecDirectory.isEmpty() && m_parsePreAndPostFiles) {
            bool cumulative = m_cumulative;
            m_cumulative = false;
            evaluateFile(mkspecDirectory + QLatin1String("/default/qmake.conf"));
            evaluateFile(mkspecDirectory + QLatin1String("/features/default_pre.prf"));

            QStringList tmp = m_valuemap.value(QLatin1String("CONFIG"));
            tmp.append(m_addUserConfigCmdArgs);
            foreach(const QString &remove, m_removeUserConfigCmdArgs)
                tmp.removeAll(remove);
            m_valuemap.insert(QLatin1String("CONFIG"), tmp);
            m_cumulative = cumulative;
        }

        return returnBool(QDir::setCurrent(pro->directoryName()));
    }

    return ProItem::ReturnTrue;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitEndProFile(ProFile * pro)
{
    PRE(pro);
    m_lineNo = pro->lineNumber();
    if (m_profileStack.count() == 1 && !m_oldPath.isEmpty()) {
        const QString &mkspecDirectory = propertyValue(QLatin1String("QMAKE_MKSPECS"));
        if (!mkspecDirectory.isEmpty()) {
            if (m_parsePreAndPostFiles) {
                bool cumulative = m_cumulative;
                m_cumulative = false;

                evaluateFile(mkspecDirectory + QLatin1String("/features/default_post.prf"));

                QSet<QString> processed;
                forever {
                    bool finished = true;
                    QStringList configs = valuesDirect(QLatin1String("CONFIG"));
                    for (int i = configs.size() - 1; i >= 0; --i) {
                        const QString config = configs[i].toLower();
                        if (!processed.contains(config)) {
                            processed.insert(config);
                            if (evaluateFile(mkspecDirectory + QLatin1String("/features/")
                                + config + QLatin1String(".prf"))) {
                                finished = false;
                                break;
                            }
                        }
                    }
                    if (finished)
                        break;
                }
                m_cumulative = cumulative;
            }

            foreach (ProBlock *itm, m_replaceFunctions)
                itm->deref();
            m_replaceFunctions.clear();
            foreach (ProBlock *itm, m_testFunctions)
                itm->deref();
            m_testFunctions.clear();
        }

        m_profileStack.pop();
        return returnBool(QDir::setCurrent(m_oldPath));
    }

    return ProItem::ReturnTrue;
}

static void replaceInList(QStringList *varlist,
        const QRegExp &regexp, const QString &replace, bool global)
{
    for (QStringList::Iterator varit = varlist->begin(); varit != varlist->end(); ) {
        if ((*varit).contains(regexp)) {
            (*varit).replace(regexp, replace);
            if ((*varit).isEmpty())
                varit = varlist->erase(varit);
            else
                ++varit;
            if(!global)
                break;
        } else {
            ++varit;
        }
    }
}

void ProFileEvaluator::Private::visitProValue(ProValue *value)
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
        QStringList targets = m_tempValuemap.value(QLatin1String("TARGET"));
        m_tempValuemap.remove(QLatin1String("TARGET"));
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
            || varName == QLatin1String("OBJECTIVE_SOURCES")
            || varName == QLatin1String("HEADERS")
            || varName == QLatin1String("INTERFACES")
            || varName == QLatin1String("FORMS")
            || varName == QLatin1String("FORMS3")
            || varName == QLatin1String("RESOURCES")
            || varName == QLatin1String("LEXSOURCES")
            || varName == QLatin1String("YACCSOURCES")){
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
        case ProVariable::SetOperator:          // =
            if (!m_cumulative) {
                if (!m_skipLevel) {
                    if (m_isFirstVariableValue) {
                        m_tempValuemap[varName] = v;
                        m_tempFilevaluemap[currentProFile()][varName] = v;
                    } else { // handle lines "CONFIG = foo bar"
                        m_tempValuemap[varName] += v;
                        m_tempFilevaluemap[currentProFile()][varName] += v;
                    }
                }
            } else {
                // We are greedy for values.
                m_tempValuemap[varName] += v;
                m_tempFilevaluemap[currentProFile()][varName] += v;
            }
            break;
        case ProVariable::UniqueAddOperator:    // *=
            if (!m_skipLevel || m_cumulative) {
                insertUnique(&m_tempValuemap, varName, v);
                insertUnique(&m_tempFilevaluemap[currentProFile()], varName, v);
            }
            break;
        case ProVariable::AddOperator:          // +=
            if (!m_skipLevel || m_cumulative) {
                m_tempValuemap[varName] += v;
                m_tempFilevaluemap[currentProFile()][varName] += v;
            }
            break;
        case ProVariable::RemoveOperator:       // -=
            if (!m_cumulative) {
                if (!m_skipLevel) {
                    removeEach(&m_tempValuemap, varName, v);
                    removeEach(&m_tempFilevaluemap[currentProFile()], varName, v);
                }
            } else {
                // We are stingy with our values, too.
            }
            break;
        case ProVariable::ReplaceOperator:      // ~=
            {
                // DEFINES ~= s/a/b/?[gqi]

                doVariableReplace(&val);
                if (val.length() < 4 || val[0] != QLatin1Char('s')) {
                    q->logMessage(format("the ~= operator can handle only the s/// function."));
                    break;
                }
                QChar sep = val.at(1);
                QStringList func = val.split(sep);
                if (func.count() < 3 || func.count() > 4) {
                    q->logMessage(format("the s/// function expects 3 or 4 arguments."));
                    break;
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

                if (!m_skipLevel || m_cumulative) {
                    // We could make a union of modified and unmodified values,
                    // but this will break just as much as it fixes, so leave it as is.
                    replaceInList(&m_tempValuemap[varName], regexp, replace, global);
                    replaceInList(&m_tempFilevaluemap[currentProFile()][varName], regexp, replace, global);

                }
            }
            break;

    }
    m_isFirstVariableValue = false;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitProFunction(ProFunction *func)
{
    // Make sure that called subblocks don't inherit & destroy the state
    bool invertThis = m_invertNext;
    m_invertNext = false;
    if (!m_skipLevel)
        m_sts.prevCondition = false;
    if (m_cumulative || !m_sts.condition) {
        QString text = func->text();
        int lparen = text.indexOf(QLatin1Char('('));
        int rparen = text.lastIndexOf(QLatin1Char(')'));
        Q_ASSERT(lparen < rparen);
        QString arguments = text.mid(lparen + 1, rparen - lparen - 1);
        QString funcName = text.left(lparen);
        m_lineNo = func->lineNumber();
        ProItem::ProItemReturn result = evaluateConditionalFunction(funcName.trimmed(), arguments);
        if (result != ProItem::ReturnFalse && result != ProItem::ReturnTrue)
            return result;
        if (!m_skipLevel && ((result == ProItem::ReturnTrue) ^ invertThis))
            m_sts.condition = true;
    }
    return ProItem::ReturnTrue;
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

QString ProFileEvaluator::Private::currentDirectory() const
{
    ProFile *cur = m_profileStack.top();
    return cur->directoryName();
}

void ProFileEvaluator::Private::doVariableReplace(QString *str)
{
    *str = expandVariableReferences(*str).join(QString(Option::field_sep));
}

QStringList ProFileEvaluator::Private::expandVariableReferences(const QString &str)
{
    QStringList ret;
//    if (ok)
//        *ok = true;
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
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';

    ushort unicode, quote = 0;
    const QChar *str_data = str.data();
    const int str_len = str.length();

    ushort term;
    QString var, args;

    int replaced = 0;
    QString current;
    for (int i = 0; i < str_len; ++i) {
        unicode = str_data[i].unicode();
        const int start_var = i;
        if (unicode == DOLLAR && str_len > i+2) {
            unicode = str_data[++i].unicode();
            if (unicode == DOLLAR) {
                term = 0;
                var.clear();
                args.clear();
                enum { VAR, ENVIRON, FUNCTION, PROPERTY } var_type = VAR;
                unicode = str_data[++i].unicode();
                if (unicode == LSQUARE) {
                    unicode = str_data[++i].unicode();
                    term = RSQUARE;
                    var_type = PROPERTY;
                } else if (unicode == LCURLY) {
                    unicode = str_data[++i].unicode();
                    var_type = VAR;
                    term = RCURLY;
                } else if (unicode == LPAREN) {
                    unicode = str_data[++i].unicode();
                    var_type = ENVIRON;
                    term = RPAREN;
                }
                forever {
                    if (!(unicode & (0xFF<<8)) &&
                       unicode != DOT && unicode != UNDERSCORE &&
                       //unicode != SINGLEQUOTE && unicode != DOUBLEQUOTE &&
                       (unicode < 'a' || unicode > 'z') && (unicode < 'A' || unicode > 'Z') &&
                       (unicode < '0' || unicode > '9'))
                        break;
                    var.append(QChar(unicode));
                    if (++i == str_len)
                        break;
                    unicode = str_data[i].unicode();
                    // at this point, i points to either the 'term' or 'next' character (which is in unicode)
                }
                if (var_type == VAR && unicode == LPAREN) {
                    var_type = FUNCTION;
                    int depth = 0;
                    forever {
                        if (++i == str_len)
                            break;
                        unicode = str_data[i].unicode();
                        if (unicode == LPAREN) {
                            depth++;
                        } else if (unicode == RPAREN) {
                            if (!depth)
                                break;
                            --depth;
                        }
                        args.append(QChar(unicode));
                    }
                    if (++i < str_len)
                        unicode = str_data[i].unicode();
                    else
                        unicode = 0;
                    // at this point i is pointing to the 'next' character (which is in unicode)
                    // this might actually be a term character since you can do $${func()}
                }
                if (term) {
                    if (unicode != term) {
                        q->logMessage(format("Missing %1 terminator [found %2]")
                            .arg(QChar(term))
                            .arg(unicode ? QString(unicode) : QString::fromLatin1(("end-of-line"))));
//                        if (ok)
//                            *ok = false;
                        return QStringList();
                    }
                } else {
                    // move the 'cursor' back to the last char of the thing we were looking at
                    --i;
                }
                // since i never points to the 'next' character, there is no reason for this to be set
                unicode = 0;

                QStringList replacement;
                if (var_type == ENVIRON) {
                    replacement = split_value_list(QString::fromLocal8Bit(qgetenv(var.toLatin1().constData())));
                } else if (var_type == PROPERTY) {
                    replacement << propertyValue(var);
                } else if (var_type == FUNCTION) {
                    replacement << evaluateExpandFunction(var, args);
                } else if (var_type == VAR) {
                    replacement = values(var);
                }
                if (!(replaced++) && start_var)
                    current = str.left(start_var);
                if (!replacement.isEmpty()) {
                    if (quote) {
                        current += replacement.join(QString(Option::field_sep));
                    } else {
                        current += replacement.takeFirst();
                        if (!replacement.isEmpty()) {
                            if (!current.isEmpty())
                                ret.append(current);
                            current = replacement.takeLast();
                            if (!replacement.isEmpty())
                                ret += replacement;
                        }
                    }
                }
            } else {
                if (replaced)
                    current.append(QLatin1Char('$'));
            }
        }
        if (quote && unicode == quote) {
            unicode = 0;
            quote = 0;
        } else if (unicode == BACKSLASH) {
            bool escape = false;
            const char *symbols = "[]{}()$\\'\"";
            for (const char *s = symbols; *s; ++s) {
                if (str_data[i+1].unicode() == (ushort)*s) {
                    i++;
                    escape = true;
                    if (!(replaced++))
                        current = str.left(start_var);
                    current.append(str.at(i));
                    break;
                }
            }
            if (escape || !replaced)
                unicode =0;
        } else if (!quote && (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE)) {
            quote = unicode;
            unicode = 0;
            if (!(replaced++) && i)
                current = str.left(i);
        } else if (!quote && (unicode == SPACE || unicode == TAB)) {
            unicode = 0;
            if (!current.isEmpty()) {
                ret.append(current);
                current.clear();
            }
        }
        if (replaced && unicode)
            current.append(QChar(unicode));
    }
    if (!replaced)
        ret = QStringList(str);
    else if (!current.isEmpty())
        ret.append(current);
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

QStringList ProFileEvaluator::Private::evaluateFunction(
        ProBlock *funcPtr, const QStringList &argumentsList, bool *ok)
{
    bool oki;
    QStringList ret;

    if (m_valuemapStack.count() >= 100) {
        q->errorMessage(format("ran into infinite recursion (depth > 100)."));
        oki = false;
    } else {
        State sts = m_sts;
        m_valuemapStack.push(m_valuemap);
        m_filevaluemapStack.push(m_filevaluemap);

        QStringList args;
        for (int i = 0; i < argumentsList.count(); ++i) {
            QStringList theArgs = expandVariableReferences(argumentsList[i]);
            args += theArgs;
            m_valuemap[QString::number(i+1)] = theArgs;
        }
        m_valuemap[QLatin1String("ARGS")] = args;
        oki = (funcPtr->Accept(this) != ProItem::ReturnFalse); // True || Return
        ret = m_returnValue;
        m_returnValue.clear();

        m_valuemap = m_valuemapStack.pop();
        m_filevaluemap = m_filevaluemapStack.pop();
        m_sts = sts;
    }
    if (ok)
        *ok = oki;
    if (oki)
        return ret;
    return QStringList();
}

QStringList ProFileEvaluator::Private::evaluateExpandFunction(const QString &func, const QString &arguments)
{
    QStringList argumentsList = split_arg_list(arguments);

    if (ProBlock *funcPtr = m_replaceFunctions.value(func, 0))
        return evaluateFunction(funcPtr, argumentsList, 0);

    QStringList args;
    for (int i = 0; i < argumentsList.count(); ++i)
        args += expandVariableReferences(argumentsList[i]).join(Option::field_sep);

    enum ExpandFunc { E_MEMBER=1, E_FIRST, E_LAST, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
                      E_SPRINTF, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
                      E_FIND, E_SYSTEM, E_UNIQUE, E_QUOTE, E_ESCAPE_EXPAND,
                      E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE,
                      E_REPLACE };

    static QHash<QString, int> expands;
    if (expands.isEmpty()) {
        expands.insert(QLatin1String("member"), E_MEMBER);
        expands.insert(QLatin1String("first"), E_FIRST);
        expands.insert(QLatin1String("last"), E_LAST);
        expands.insert(QLatin1String("cat"), E_CAT);
        expands.insert(QLatin1String("fromfile"), E_FROMFILE); // implementation disabled (see comment below)
        expands.insert(QLatin1String("eval"), E_EVAL);
        expands.insert(QLatin1String("list"), E_LIST);
        expands.insert(QLatin1String("sprintf"), E_SPRINTF);
        expands.insert(QLatin1String("join"), E_JOIN);
        expands.insert(QLatin1String("split"), E_SPLIT);
        expands.insert(QLatin1String("basename"), E_BASENAME);
        expands.insert(QLatin1String("dirname"), E_DIRNAME);
        expands.insert(QLatin1String("section"), E_SECTION);
        expands.insert(QLatin1String("find"), E_FIND);
        expands.insert(QLatin1String("system"), E_SYSTEM);
        expands.insert(QLatin1String("unique"), E_UNIQUE);
        expands.insert(QLatin1String("quote"), E_QUOTE);
        expands.insert(QLatin1String("escape_expand"), E_ESCAPE_EXPAND);
        expands.insert(QLatin1String("upper"), E_UPPER);
        expands.insert(QLatin1String("lower"), E_LOWER);
        expands.insert(QLatin1String("re_escape"), E_RE_ESCAPE);
        expands.insert(QLatin1String("files"), E_FILES);
        expands.insert(QLatin1String("prompt"), E_PROMPT); // interactive, so cannot be implemented
        expands.insert(QLatin1String("replace"), E_REPLACE);
    }
    ExpandFunc func_t = ExpandFunc(expands.value(func.toLower()));

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
                        "requires three or four arguments.").arg(func));
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
        case E_SPRINTF:
            if(args.count() < 1) {
                q->logMessage(format("sprintf(format, ...) requires at least one argument"));
            } else {
                QString tmp = args.at(0);
                for (int i = 1; i < args.count(); ++i)
                    tmp = tmp.arg(args.at(i));
                ret = split_value_list(tmp);
            }
            break;
        case E_JOIN: {
            if (args.count() < 1 || args.count() > 4) {
                q->logMessage(format("join(var, glue, before, after) requires one to four arguments."));
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
            if (args.count() != 2) {
                q->logMessage(format("split(var, sep) requires one or two arguments"));
            } else {
                const QString &sep = (args.count() == 2) ? args[1] : QString(Option::field_sep);
                foreach (const QString &var, values(args.first()))
                    foreach (const QString &splt, var.split(sep))
                        ret.append(splt);
            }
            break;
        }
        case E_MEMBER: {
            if (args.count() < 1 || args.count() > 3) {
                q->logMessage(format("member(var, start, end) requires one to three arguments."));
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
        case E_CAT:
            if (args.count() < 1 || args.count() > 2) {
                q->logMessage(format("cat(file, singleline=true) requires one or two arguments."));
            } else {
                QString file = args[0];
                file = Option::fixPathToLocalOS(file);

                bool singleLine = true;
                if (args.count() > 1)
                    singleLine = (!args[1].compare(QLatin1String("true"), Qt::CaseInsensitive));

                QFile qfile(file);
                if (qfile.open(QIODevice::ReadOnly)) {
                    QTextStream stream(&qfile);
                    while (!stream.atEnd()) {
                        ret += split_value_list(stream.readLine().trimmed());
                        if (!singleLine)
                            ret += QLatin1String("\n");
                    }
                    qfile.close();
                }
            }
            break;
#if 0 // Used only by Qt's configure for caching
        case E_FROMFILE:
            if (args.count() != 2) {
                q->logMessage(format("fromfile(file, variable) requires two arguments."));
            } else {
                QString file = args[0], seek_variableName = args[1];

                ProFile pro(Option::fixPathToLocalOS(file));

                ProFileEvaluator visitor;
                visitor.setVerbose(m_verbose);
                visitor.setCumulative(m_cumulative);

                if (!visitor.queryProFile(&pro))
                    break;

                if (!visitor.accept(&pro))
                    break;

                ret = visitor.values(seek_variableName);
            }
            break;
#endif
        case E_EVAL: {
            if (args.count() != 1) {
                q->logMessage(format("eval(variable) requires one argument"));

            } else {
                ret += values(args.at(0));
            }
            break; }
        case E_LIST: {
            static int x = 0;
            QString tmp;
            tmp.sprintf(".QMAKE_INTERNAL_TMP_variableName_%d", x++);
            ret = QStringList(tmp);
            QStringList lst;
            foreach (const QString &arg, args)
                lst += split_value_list(arg);
            m_valuemap[tmp] = lst;
            break; }
        case E_FIND:
            if (args.count() != 2) {
                q->logMessage(format("find(var, str) requires two arguments."));
            } else {
                QRegExp regx(args[1]);
                foreach (const QString &val, values(args.first()))
                    if (regx.indexIn(val) != -1)
                        ret += val;
            }
            break;
        case E_SYSTEM:
            if (!m_skipLevel) {
                if (args.count() < 1 || args.count() > 2) {
                    q->logMessage(format("system(execute) requires one or two arguments."));
                } else {
                    char buff[256];
                    FILE *proc = QT_POPEN(args[0].toLatin1(), "r");
                    bool singleLine = true;
                    if (args.count() > 1)
                        singleLine = (!args[1].compare(QLatin1String("true"), Qt::CaseInsensitive));
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
                    if (proc)
                        QT_PCLOSE(proc);
                }
            }
            break;
        case E_UNIQUE:
            if(args.count() != 1) {
                q->logMessage(format("unique(var) requires one argument."));
            } else {
                foreach (const QString &var, values(args.first()))
                    if (!ret.contains(var))
                        ret.append(var);
            }
            break;
        case E_QUOTE:
            for (int i = 0; i < args.count(); ++i)
                ret += QStringList(args.at(i));
            break;
        case E_ESCAPE_EXPAND:
            for (int i = 0; i < args.size(); ++i) {
                QChar *i_data = args[i].data();
                int i_len = args[i].length();
                for (int x = 0; x < i_len; ++x) {
                    if (*(i_data+x) == QLatin1Char('\\') && x < i_len-1) {
                        if (*(i_data+x+1) == QLatin1Char('\\')) {
                            ++x;
                        } else {
                            struct {
                                char in, out;
                            } mapped_quotes[] = {
                                { 'n', '\n' },
                                { 't', '\t' },
                                { 'r', '\r' },
                                { 0, 0 }
                            };
                            for (int i = 0; mapped_quotes[i].in; ++i) {
                                if (*(i_data+x+1) == QLatin1Char(mapped_quotes[i].in)) {
                                    *(i_data+x) = QLatin1Char(mapped_quotes[i].out);
                                    if (x < i_len-2)
                                        memmove(i_data+x+1, i_data+x+2, (i_len-x-2)*sizeof(QChar));
                                    --i_len;
                                    break;
                                }
                            }
                        }
                    }
                }
                ret.append(QString(i_data, i_len));
            }
            break;
        case E_RE_ESCAPE:
            for (int i = 0; i < args.size(); ++i)
                ret += QRegExp::escape(args[i]);
            break;
        case E_UPPER:
        case E_LOWER:
            for (int i = 0; i < args.count(); ++i)
                if (func_t == E_UPPER)
                    ret += args[i].toUpper();
                else
                    ret += args[i].toLower();
            break;
        case E_FILES:
            if (args.count() != 1 && args.count() != 2) {
                q->logMessage(format("files(pattern, recursive=false) requires one or two arguments"));
            } else {
                bool recursive = false;
                if (args.count() == 2)
                    recursive = (!args[1].compare(QLatin1String("true"), Qt::CaseInsensitive) || args[1].toInt());
                QStringList dirs;
                QString r = Option::fixPathToLocalOS(args[0]);
                int slash = r.lastIndexOf(QDir::separator());
                if (slash != -1) {
                    dirs.append(r.left(slash));
                    r = r.mid(slash+1);
                } else {
                    dirs.append(QString());
                }

                const QRegExp regex(r, Qt::CaseSensitive, QRegExp::Wildcard);
                for (int d = 0; d < dirs.count(); d++) {
                    QString dir = dirs[d];
                    if (!dir.isEmpty() && !dir.endsWith(Option::dir_sep))
                        dir += QLatin1Char('/');

                    QDir qdir(dir);
                    for (int i = 0; i < (int)qdir.count(); ++i) {
                        if (qdir[i] == QLatin1String(".") || qdir[i] == QLatin1String(".."))
                            continue;
                        QString fname = dir + qdir[i];
                        if (QFileInfo(fname).isDir()) {
                            if (recursive)
                                dirs.append(fname);
                        }
                        if (regex.exactMatch(qdir[i]))
                            ret += fname;
                    }
                }
            }
            break;
        case E_REPLACE:
            if(args.count() != 3 ) {
                q->logMessage(format("replace(var, before, after) requires three arguments"));
            } else {
                const QRegExp before(args[1]);
                const QString after(args[2]);
                foreach (QString val, values(args.first()))
                    ret += val.replace(before, after);
            }
            break;
        case 0:
            q->logMessage(format("'%1' is not a recognized replace function").arg(func));
            break;
        default:
            q->logMessage(format("Function '%1' is not implemented").arg(func));
            break;
    }

    return ret;
}

ProItem::ProItemReturn ProFileEvaluator::Private::evaluateConditionalFunction(
        const QString &function, const QString &arguments)
{
    QStringList argumentsList = split_arg_list(arguments);

    if (ProBlock *funcPtr = m_testFunctions.value(function, 0)) {
        bool ok;
        QStringList ret = evaluateFunction(funcPtr, argumentsList, &ok);
        if (ok) {
            if (ret.isEmpty()) {
                return ProItem::ReturnTrue;
            } else {
                if (ret.first() != QLatin1String("false")) {
                    if (ret.first() == QLatin1String("true")) {
                        return ProItem::ReturnTrue;
                    } else {
                        bool ok;
                        int val = ret.first().toInt(&ok);
                        if (ok) {
                            if (val)
                                return ProItem::ReturnTrue;
                        } else {
                            q->logMessage(format("Unexpected return value from test '%1': %2")
                                          .arg(function).arg(ret.join(QLatin1String(" :: "))));
                        }
                    }
                }
            }
        }
        return ProItem::ReturnFalse;
    }

    QString sep;
    sep.append(Option::field_sep);
    QStringList args;
    for (int i = 0; i < argumentsList.count(); ++i)
        args += expandVariableReferences(argumentsList[i]).join(sep);

    enum TestFunc { T_REQUIRES=1, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
                    T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
                    T_RETURN, T_BREAK, T_NEXT, T_DEFINED, T_CONTAINS, T_INFILE,
                    T_COUNT, T_ISEMPTY, T_INCLUDE, T_LOAD, T_DEBUG, T_MESSAGE, T_IF,
                    T_FOR, T_DEFINE_TEST, T_DEFINE_REPLACE };

    static QHash<QString, int> functions;
    if (functions.isEmpty()) {
        functions.insert(QLatin1String("requires"), T_REQUIRES);
        functions.insert(QLatin1String("greaterThan"), T_GREATERTHAN);
        functions.insert(QLatin1String("lessThan"), T_LESSTHAN);
        functions.insert(QLatin1String("equals"), T_EQUALS);
        functions.insert(QLatin1String("isEqual"), T_EQUALS);
        functions.insert(QLatin1String("exists"), T_EXISTS);
        functions.insert(QLatin1String("export"), T_EXPORT);
        functions.insert(QLatin1String("clear"), T_CLEAR);
        functions.insert(QLatin1String("unset"), T_UNSET);
        functions.insert(QLatin1String("eval"), T_EVAL);
        functions.insert(QLatin1String("CONFIG"), T_CONFIG);
        functions.insert(QLatin1String("if"), T_IF);
        functions.insert(QLatin1String("isActiveConfig"), T_CONFIG);
        functions.insert(QLatin1String("system"), T_SYSTEM);
        functions.insert(QLatin1String("return"), T_RETURN);
        functions.insert(QLatin1String("break"), T_BREAK);
        functions.insert(QLatin1String("next"), T_NEXT);
        functions.insert(QLatin1String("defined"), T_DEFINED);
        functions.insert(QLatin1String("contains"), T_CONTAINS);
        functions.insert(QLatin1String("infile"), T_INFILE);
        functions.insert(QLatin1String("count"), T_COUNT);
        functions.insert(QLatin1String("isEmpty"), T_ISEMPTY);
        functions.insert(QLatin1String("load"), T_LOAD);         //v
        functions.insert(QLatin1String("include"), T_INCLUDE);   //v
        functions.insert(QLatin1String("debug"), T_DEBUG);
        functions.insert(QLatin1String("message"), T_MESSAGE);   //v
        functions.insert(QLatin1String("warning"), T_MESSAGE);   //v
        functions.insert(QLatin1String("error"), T_MESSAGE);     //v
        functions.insert(QLatin1String("for"), T_FOR);     //v
        functions.insert(QLatin1String("defineTest"), T_DEFINE_TEST);        //v
        functions.insert(QLatin1String("defineReplace"), T_DEFINE_REPLACE);  //v
    }

    TestFunc func_t = (TestFunc)functions.value(function);

    switch (func_t) {
        case T_DEFINE_TEST:
            m_definingTest = true;
            goto defineFunc;
        case T_DEFINE_REPLACE:
            m_definingTest = false;
          defineFunc:
            if (args.count() != 1) {
                q->logMessage(format("%s(function) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            m_definingFunc = args.first();
            return ProItem::ReturnTrue;
        case T_DEFINED:
            if (args.count() < 1 || args.count() > 2) {
                q->logMessage(format("defined(function, [\"test\"|\"replace\"])"
                                     " requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            if (args.count() > 1) {
                if (args[1] == QLatin1String("test"))
                    return returnBool(m_testFunctions.contains(args[0]));
                else if (args[1] == QLatin1String("replace"))
                    return returnBool(m_replaceFunctions.contains(args[0]));
                q->logMessage(format("defined(function, type):"
                                     " unexpected type [%1].\n").arg(args[1]));
                return ProItem::ReturnFalse;
            }
            return returnBool(m_replaceFunctions.contains(args[0])
                              || m_testFunctions.contains(args[0]));
        case T_RETURN:
            m_returnValue = args;
            // It is "safe" to ignore returns - due to qmake brokeness
            // they cannot be used to terminate loops anyway.
            if (m_skipLevel || m_cumulative)
                return ProItem::ReturnTrue;
            if (m_valuemapStack.isEmpty()) {
                q->logMessage(format("unexpected return()."));
                return ProItem::ReturnFalse;
            }
            return ProItem::ReturnReturn;
        case T_EXPORT:
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnTrue;
            if (args.count() != 1) {
                q->logMessage(format("export(variable) requires one argument."));
                return ProItem::ReturnFalse;
            }
            for (int i = 0; i < m_valuemapStack.size(); ++i) {
                m_valuemapStack[i][args[0]] = m_valuemap[args[0]];
                m_filevaluemapStack[i][currentProFile()][args[0]] =
                        m_filevaluemap[currentProFile()][args[0]];
            }
            return ProItem::ReturnTrue;
#if 0
        case T_INFILE:
        case T_REQUIRES:
        case T_EVAL:
#endif
        case T_FOR: {
            if (m_cumulative) // This is a no-win situation, so just pretend it's no loop
                return ProItem::ReturnTrue;
            if (m_skipLevel)
                return ProItem::ReturnFalse;
            if (args.count() > 2 || args.count() < 1) {
                q->logMessage(format("for({var, list|var, forever|ever})"
                                     " requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            ProLoop loop;
            loop.infinite = false;
            loop.index = 0;
            QString it_list;
            if (args.count() == 1) {
                doVariableReplace(&args[0]);
                it_list = args[0];
                if (args[0] != QLatin1String("ever")) {
                    q->logMessage(format("for({var, list|var, forever|ever})"
                                         " requires one or two arguments."));
                    return ProItem::ReturnFalse;
                }
                it_list = QLatin1String("forever");
            } else {
                loop.variable = args[0];
                loop.oldVarVal = m_valuemap.value(loop.variable);
                doVariableReplace(&args[1]);
                it_list = args[1];
            }
            loop.list = m_valuemap[it_list];
            if (loop.list.isEmpty()) {
                if (it_list == QLatin1String("forever")) {
                    loop.infinite = true;
                } else {
                    int dotdot = it_list.indexOf(QLatin1String(".."));
                    if (dotdot != -1) {
                        bool ok;
                        int start = it_list.left(dotdot).toInt(&ok);
                        if (ok) {
                            int end = it_list.mid(dotdot+2).toInt(&ok);
                            if (ok) {
                                if (start < end) {
                                    for (int i = start; i <= end; i++)
                                        loop.list << QString::number(i);
                                } else {
                                    for (int i = start; i >= end; i--)
                                        loop.list << QString::number(i);
                                }
                            }
                        }
                    }
                }
            }
            m_loopStack.push(loop);
            m_sts.condition = true;
            return ProItem::ReturnLoop;
        }
        case T_BREAK:
            if (m_skipLevel)
                return ProItem::ReturnFalse;
            if (!m_loopStack.isEmpty())
                return ProItem::ReturnBreak;
            // ### missing: breaking out of multiline blocks
            q->logMessage(format("unexpected break()."));
            return ProItem::ReturnFalse;
        case T_NEXT:
            if (m_skipLevel)
                return ProItem::ReturnFalse;
            if (!m_loopStack.isEmpty())
                return ProItem::ReturnNext;
            q->logMessage(format("unexpected next()."));
            return ProItem::ReturnFalse;
        case T_IF: {
            if (args.count() != 1) {
                q->logMessage(format("if(condition) requires one argument."));
                return ProItem::ReturnFalse;
            }
            QString cond = args.first();
            bool escaped = false; // This is more than qmake does
            bool quoted = false;
            bool ret = true;
            bool orOp = false;
            bool invert = false;
            bool isFunc = false;
            int parens = 0;
            QString test;
            test.reserve(20);
            QString args;
            args.reserve(50);
            const QChar *d = cond.unicode();
            const QChar *ed = d + cond.length();
            while (d < ed) {
                ushort c = (d++)->unicode();
                if (!escaped) {
                    if (c == '\\') {
                        escaped = true;
                        args += c; // Assume no-one quotes the test name
                        continue;
                    } else if (c == '"') {
                        quoted = !quoted;
                        args += c; // Ditto
                        continue;
                    }
                } else {
                    escaped = false;
                }
                if (quoted) {
                    args += c; // Ditto
                } else {
                    bool isOp = false;
                    if (c == '(') {
                        isFunc = true;
                        if (parens)
                            args += c;
                        ++parens;
                    } else if (c == ')') {
                        --parens;
                        if (parens)
                            args += c;
                    } else if (!parens) {
                        if (c == ':' || c == '|')
                            isOp = true;
                        else if (c == '!')
                            invert = true;
                        else
                            test += c;
                    } else {
                        args += c;
                    }
                    if (!parens && (isOp || d == ed)) {
                        // Yes, qmake doesn't shortcut evaluations here. We can't, either,
                        // as some test functions have side effects.
                        bool success;
                        if (isFunc) {
                            success = evaluateConditionalFunction(test, args);
                        } else {
                            success = isActiveConfig(test, true);
                        }
                        success ^= invert;
                        if (orOp)
                            ret |= success;
                        else
                            ret &= success;
                        orOp = (c == '|');
                        invert = false;
                        isFunc = false;
                        test.clear();
                        args.clear();
                    }
                }
            }
            return returnBool(ret);
        }
        case T_CONFIG: {
            if (args.count() < 1 || args.count() > 2) {
                q->logMessage(format("CONFIG(config) requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            if (args.count() == 1) {
                //cond = isActiveConfig(args.first()); XXX
                return ProItem::ReturnFalse;
            }
            const QStringList mutuals = args[1].split(QLatin1Char('|'));
            const QStringList &configs = valuesDirect(QLatin1String("CONFIG"));

            for (int i = configs.size() - 1; i >= 0; i--) {
                for (int mut = 0; mut < mutuals.count(); mut++) {
                    if (configs[i] == mutuals[mut].trimmed()) {
                        return returnBool(configs[i] == args[0]);
                    }
                }
            }
            return ProItem::ReturnFalse;
        }
        case T_CONTAINS: {
            if (args.count() < 2 || args.count() > 3) {
                q->logMessage(format("contains(var, val) requires two or three arguments."));
                return ProItem::ReturnFalse;
            }

            QRegExp regx(args[1]);
            const QStringList &l = values(args.first());
            if (args.count() == 2) {
                for (int i = 0; i < l.size(); ++i) {
                    const QString val = l[i];
                    if (regx.exactMatch(val) || val == args[1]) {
                        return ProItem::ReturnTrue;
                    }
                }
            } else {
                const QStringList mutuals = args[2].split(QLatin1Char('|'));
                for (int i = l.size() - 1; i >= 0; i--) {
                    const QString val = l[i];
                    for (int mut = 0; mut < mutuals.count(); mut++) {
                        if (val == mutuals[mut].trimmed()) {
                            return returnBool(regx.exactMatch(val) || val == args[1]);
                        }
                    }
                }
            }
            return ProItem::ReturnFalse;
        }
        case T_COUNT: {
            if (args.count() != 2 && args.count() != 3) {
                q->logMessage(format("count(var, count, op=\"equals\") requires two or three arguments."));
                return ProItem::ReturnFalse;
            }
            if (args.count() == 3) {
                QString comp = args[2];
                if (comp == QLatin1String(">") || comp == QLatin1String("greaterThan")) {
                    return returnBool(values(args.first()).count() > args[1].toInt());
                } else if (comp == QLatin1String(">=")) {
                    return returnBool(values(args.first()).count() >= args[1].toInt());
                } else if (comp == QLatin1String("<") || comp == QLatin1String("lessThan")) {
                    return returnBool(values(args.first()).count() < args[1].toInt());
                } else if (comp == QLatin1String("<=")) {
                    return returnBool(values(args.first()).count() <= args[1].toInt());
                } else if (comp == QLatin1String("equals") || comp == QLatin1String("isEqual")
                           || comp == QLatin1String("=") || comp == QLatin1String("==")) {
                    return returnBool(values(args.first()).count() == args[1].toInt());
                } else {
                    q->logMessage(format("unexpected modifier to count(%2)").arg(comp));
                    return ProItem::ReturnFalse;
                }
            }
            return returnBool(values(args.first()).count() == args[1].toInt());
        }
        case T_GREATERTHAN:
        case T_LESSTHAN: {
            if (args.count() != 2) {
                q->logMessage(format("%1(variable, value) requires two arguments.").arg(function));
                return ProItem::ReturnFalse;
            }
            QString rhs(args[1]), lhs(values(args[0]).join(QString(Option::field_sep)));
            bool ok;
            int rhs_int = rhs.toInt(&ok);
            if (ok) { // do integer compare
                int lhs_int = lhs.toInt(&ok);
                if (ok) {
                    if (func_t == T_GREATERTHAN)
                        return returnBool(lhs_int > rhs_int);
                    return returnBool(lhs_int < rhs_int);
                }
            }
            if (func_t == T_GREATERTHAN)
                return returnBool(lhs > rhs);
            return returnBool(lhs < rhs);
        }
        case T_EQUALS:
            if (args.count() != 2) {
                q->logMessage(format("%1(variable, value) requires two arguments.").arg(function));
                return ProItem::ReturnFalse;
            }
            return returnBool(values(args[0]).join(QString(Option::field_sep)) == args[1]);
        case T_CLEAR: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            if (args.count() != 1) {
                q->logMessage(format("%1(variable) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            QHash<QString, QStringList>::Iterator it = m_valuemap.find(args[0]);
            if (it == m_valuemap.end())
                return ProItem::ReturnFalse;
            it->clear();
            return ProItem::ReturnTrue;
        }
        case T_UNSET: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            if (args.count() != 1) {
                q->logMessage(format("%1(variable) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            QHash<QString, QStringList>::Iterator it = m_valuemap.find(args[0]);
            if (it == m_valuemap.end())
                return ProItem::ReturnFalse;
            m_valuemap.erase(it);
            return ProItem::ReturnTrue;
        }
        case T_INCLUDE: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            QString parseInto;
            if (args.count() == 2) {
                parseInto = args[1];
            } else if (args.count() != 1) {
                q->logMessage(format("include(file) requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            QString fileName = args.first();
            // ### this breaks if we have include(c:/reallystupid.pri) but IMHO that's really bad style.
            QDir currentProPath(currentDirectory());
            fileName = QDir::cleanPath(currentProPath.absoluteFilePath(fileName));
            State sts = m_sts;
            bool ok = evaluateFile(fileName);
            m_sts = sts;
            return returnBool(ok);
        }
        case T_LOAD: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            QString parseInto;
            bool ignore_error = false;
            if (args.count() == 2) {
                QString sarg = args[1];
                ignore_error = (!sarg.compare(QLatin1String("true"), Qt::CaseInsensitive) || sarg.toInt());
            } else if (args.count() != 1) {
                q->logMessage(format("load(feature) requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            // XXX ignore_error unused
            return returnBool(evaluateFeatureFile(args.first()));
        }
        case T_DEBUG:
            // Yup - do nothing. Nothing is going to enable debug output anyway.
            return ProItem::ReturnFalse;
        case T_MESSAGE: {
            if (args.count() != 1) {
                q->logMessage(format("%1(message) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            QString msg = fixEnvVariables(args.first());
            q->fileMessage(QString::fromLatin1("Project %1: %2").arg(function.toUpper(), msg));
            // ### Consider real termination in non-cumulative mode
            return returnBool(function != QLatin1String("error"));
        }
#if 0 // Way too dangerous to enable.
        case T_SYSTEM: {
            if (args.count() != 1) {
                q->logMessage(format("system(exec) requires one argument."));
                ProItem::ReturnFalse;
            }
            return returnBool(system(args.first().toLatin1().constData()) == 0);
        }
#endif
        case T_ISEMPTY: {
            if (args.count() != 1) {
                q->logMessage(format("isEmpty(var) requires one argument."));
                return ProItem::ReturnFalse;
            }
            QStringList sl = values(args.first());
            if (sl.count() == 0) {
                return ProItem::ReturnTrue;
            } else if (sl.count() > 0) {
                QString var = sl.first();
                if (var.isEmpty())
                    return ProItem::ReturnTrue;
            }
            return ProItem::ReturnFalse;
        }
        case T_EXISTS: {
            if (args.count() != 1) {
                q->logMessage(format("exists(file) requires one argument."));
                return ProItem::ReturnFalse;
            }
            QString file = args.first();
            file = Option::fixPathToLocalOS(file);

            if (QFile::exists(file)) {
                return ProItem::ReturnTrue;
            }
            //regular expression I guess
            QString dirstr = currentDirectory();
            int slsh = file.lastIndexOf(Option::dir_sep);
            if (slsh != -1) {
                dirstr = file.left(slsh+1);
                file = file.right(file.length() - slsh - 1);
            }
            if (file.contains(QLatin1Char('*')) || file.contains(QLatin1Char('?')))
                if (!QDir(dirstr).entryList(QStringList(file)).isEmpty())
                    return ProItem::ReturnTrue;

            return ProItem::ReturnFalse;
        }
        case 0:
            q->logMessage(format("'%1' is not a recognized test function").arg(function));
            return ProItem::ReturnFalse;
        default:
            q->logMessage(format("Function '%1' is not implemented").arg(function));
            return ProItem::ReturnFalse;
    }
}

QStringList ProFileEvaluator::Private::values(const QString &variableName,
                                              const QHash<QString, QStringList> &place,
                                              const ProFile *pro) const
{
    if (variableName == QLatin1String("LITERAL_WHITESPACE")) //a real space in a token
        return QStringList(QLatin1String("\t"));
    if (variableName == QLatin1String("LITERAL_DOLLAR")) //a real $
        return QStringList(QLatin1String("$"));
    if (variableName == QLatin1String("LITERAL_HASH")) //a real #
        return QStringList(QLatin1String("#"));
    if (variableName == QLatin1String("OUT_PWD")) //the out going dir
        return QStringList(m_outputDir);
    if (variableName == QLatin1String("PWD") ||  //current working dir (of _FILE_)
        variableName == QLatin1String("IN_PWD"))
        return QStringList(currentDirectory());
    if (variableName == QLatin1String("DIR_SEPARATOR"))
        return QStringList(Option::dir_sep);
    if (variableName == QLatin1String("DIRLIST_SEPARATOR"))
        return QStringList(Option::dirlist_sep);
    if (variableName == QLatin1String("_LINE_")) //parser line number
        return QStringList(QString::number(m_lineNo));
    if (variableName == QLatin1String("_FILE_")) //parser file; qmake is a bit weird here
        return QStringList(m_profileStack.size() == 1 ? pro->fileName() : QFileInfo(pro->fileName()).fileName());
    if (variableName == QLatin1String("_DATE_")) //current date/time
        return QStringList(QDateTime::currentDateTime().toString());
    if (variableName == QLatin1String("_PRO_FILE_"))
        return QStringList(m_origfile);
    if (variableName == QLatin1String("_PRO_FILE_PWD_"))
        return  QStringList(QFileInfo(m_origfile).absolutePath());
    if (variableName == QLatin1String("_QMAKE_CACHE_"))
        return QStringList(); // FIXME?
    if (variableName.startsWith(QLatin1String("QMAKE_HOST."))) {
        QString ret, type = variableName.mid(11);
#if defined(Q_OS_WIN32)
        if (type == QLatin1String("os")) {
            ret = QLatin1String("Windows");
        } else if (type == QLatin1String("name")) {
            DWORD name_length = 1024;
            TCHAR name[1024];
            if (GetComputerName(name, &name_length))
                ret = QString::fromUtf16((ushort*)name, name_length);
        } else if (type == QLatin1String("version") || type == QLatin1String("version_string")) {
            QSysInfo::WinVersion ver = QSysInfo::WindowsVersion;
            if (type == QLatin1String("version"))
                ret = QString::number(ver);
            else if (ver == QSysInfo::WV_Me)
                ret = QLatin1String("WinMe");
            else if (ver == QSysInfo::WV_95)
                ret = QLatin1String("Win95");
            else if (ver == QSysInfo::WV_98)
                ret = QLatin1String("Win98");
            else if (ver == QSysInfo::WV_NT)
                ret = QLatin1String("WinNT");
            else if (ver == QSysInfo::WV_2000)
                ret = QLatin1String("Win2000");
            else if (ver == QSysInfo::WV_2000)
                ret = QLatin1String("Win2003");
            else if (ver == QSysInfo::WV_XP)
                ret = QLatin1String("WinXP");
            else if (ver == QSysInfo::WV_VISTA)
                ret = QLatin1String("WinVista");
            else
                ret = QLatin1String("Unknown");
        } else if (type == QLatin1String("arch")) {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            switch(info.wProcessorArchitecture) {
#ifdef PROCESSOR_ARCHITECTURE_AMD64
            case PROCESSOR_ARCHITECTURE_AMD64:
                ret = QLatin1String("x86_64");
                break;
#endif
            case PROCESSOR_ARCHITECTURE_INTEL:
                ret = QLatin1String("x86");
                break;
            case PROCESSOR_ARCHITECTURE_IA64:
#ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
            case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
#endif
                ret = QLatin1String("IA64");
                break;
            default:
                ret = QLatin1String("Unknown");
                break;
            }
        }
#elif defined(Q_OS_UNIX)
        struct utsname name;
        if (!uname(&name)) {
            if (type == QLatin1String("os"))
                ret = QString::fromLatin1(name.sysname);
            else if (type == QLatin1String("name"))
                ret = QString::fromLatin1(name.nodename);
            else if (type == QLatin1String("version"))
                ret = QString::fromLatin1(name.release);
            else if (type == QLatin1String("version_string"))
                ret = QString::fromLatin1(name.version);
            else if (type == QLatin1String("arch"))
                ret = QString::fromLatin1(name.machine);
        }
#endif
        return QStringList(ret);
    }

    QStringList result = place[variableName];
    if (result.isEmpty()) {
        if (variableName == QLatin1String("TARGET")) {
            result.append(QFileInfo(m_origfile).baseName());
        } else if (variableName == QLatin1String("TEMPLATE")) {
            result.append(QLatin1String("app"));
        } else if (variableName == QLatin1String("QMAKE_DIR_SEP")) {
            result.append(Option::dirlist_sep);
        }
    }
    return result;
}

QStringList ProFileEvaluator::Private::values(const QString &variableName) const
{
    return values(variableName, m_valuemap, currentProFile());
}

QStringList ProFileEvaluator::Private::values(const QString &variableName, const ProFile *pro) const
{
    return values(variableName, m_filevaluemap[pro], pro);
}

ProFile *ProFileEvaluator::parsedProFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (fi.exists()) {
        QString fn = QDir::cleanPath(fi.absoluteFilePath());
        foreach (const ProFile *pf, d->m_profileStack)
            if (pf->fileName() == fn) {
                errorMessage(d->format("circular inclusion of %1").arg(fn));
                return 0;
            }
        ProFile *pro = new ProFile(fn);
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

bool ProFileEvaluator::Private::evaluateFile(const QString &fileName)
{
    ProFile *pro = q->parsedProFile(fileName);
    if (pro) {
        m_profileStack.push(pro);
        bool ok = (pro->Accept(this) == ProItem::ReturnTrue);
        m_profileStack.pop();
        q->releaseParsedProFile(pro);
        return ok;
    } else {
        return false;
    }
}

bool ProFileEvaluator::Private::evaluateFeatureFile(const QString &fileName)
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
    if (fn.isEmpty())
        return false;
    bool cumulative = m_cumulative;
    m_cumulative = false;
    bool ok = evaluateFile(fn);
    m_cumulative = cumulative;
    return ok;
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

inline QStringList fixEnvVariables(const QStringList &x)
{
    QStringList ret;
    foreach (const QString &str, x)
        ret << Option::fixString(str, Option::FixEnvVars);
    return ret;
}


QStringList ProFileEvaluator::values(const QString &variableName) const
{
    return fixEnvVariables(d->values(variableName));
}

QStringList ProFileEvaluator::values(const QString &variableName, const ProFile *pro) const
{
    return fixEnvVariables(d->values(variableName, pro));
}

ProFileEvaluator::TemplateType ProFileEvaluator::templateType()
{
    QStringList templ = values(QLatin1String("TEMPLATE"));
    if (templ.count() >= 1) {
        const QString &t = templ.last();
        if (!t.compare(QLatin1String("app"), Qt::CaseInsensitive))
            return TT_Application;
        if (!t.compare(QLatin1String("lib"), Qt::CaseInsensitive))
            return TT_Library;
        if (!t.compare(QLatin1String("script"), Qt::CaseInsensitive))
            return TT_Script;
        if (!t.compare(QLatin1String("subdirs"), Qt::CaseInsensitive))
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
    if (d->m_verbose && !d->m_skipLevel)
        qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::fileMessage(const QString &message)
{
    if (!d->m_skipLevel)
        qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::errorMessage(const QString &message)
{
    if (!d->m_skipLevel)
        qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::setVerbose(bool on)
{
    d->m_verbose = on;
}

void ProFileEvaluator::setCumulative(bool on)
{
    d->m_cumulative = on;
}

void ProFileEvaluator::setOutputDir(const QString &dir)
{
    d->m_outputDir = dir;
}

void ProFileEvaluator::setUserConfigCmdArgs(const QStringList &addUserConfigCmdArgs, const QStringList &removeUserConfigCmdArgs)
{
    d->m_addUserConfigCmdArgs = addUserConfigCmdArgs;
    d->m_removeUserConfigCmdArgs = removeUserConfigCmdArgs;
}

void ProFileEvaluator::setParsePreAndPostFiles(bool on)
{
    d->m_parsePreAndPostFiles = on;
}

void evaluateProFile(const ProFileEvaluator &visitor, QHash<QByteArray, QStringList> *varMap)
{
    QStringList sourceFiles;
    QString codecForTr;
    QString codecForSource;
    QStringList tsFileNames;

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

    sourceFiles.sort();
    sourceFiles.removeDuplicates();
    tsFileNames.sort();
    tsFileNames.removeDuplicates();

    varMap->insert("SOURCES", sourceFiles);
    varMap->insert("CODECFORTR", QStringList() << codecForTr);
    varMap->insert("CODECFORSRC", QStringList() << codecForSource);
    varMap->insert("TRANSLATIONS", tsFileNames);
}

bool evaluateProFile(const QString &fileName, bool verbose, QHash<QByteArray, QStringList> *varMap)
{
    QFileInfo fi(fileName);
    if (!fi.exists())
        return false;

    ProFile pro(fi.absoluteFilePath());

    ProFileEvaluator visitor;
    visitor.setVerbose(verbose);

    if (!visitor.queryProFile(&pro))
        return false;

    if (!visitor.accept(&pro))
        return false;

    evaluateProFile(visitor, varMap);

    return true;
}

QT_END_NAMESPACE
