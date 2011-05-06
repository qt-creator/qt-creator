/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef SEMANTICHIGHLIGHTER_H
#define SEMANTICHIGHLIGHTER_H

#include "qmljseditor.h"

#include <QtCore/QWaitCondition>
#include <QtCore/QModelIndex>
#include <QtCore/QMutex>
#include <QtCore/QThread>

namespace QmlJSEditor {
namespace Internal {

struct SemanticHighlighterSource
{
    QmlJS::Snapshot snapshot;
    QString fileName;
    QString code;
    int line;
    int column;
    int revision;
    bool force;

    SemanticHighlighterSource()
        : line(0), column(0), revision(0), force(false)
    { }

    SemanticHighlighterSource(const QmlJS::Snapshot &snapshot,
           const QString &fileName,
           const QString &code,
           int line, int column,
           int revision)
        : snapshot(snapshot), fileName(fileName),
          code(code), line(line), column(column),
          revision(revision), force(false)
    { }

    void clear()
    {
        snapshot = QmlJS::Snapshot();
        fileName.clear();
        code.clear();
        line = 0;
        column = 0;
        revision = 0;
        force = false;
    }
};

class SemanticHighlighter: public QThread
{
    Q_OBJECT

public:
    SemanticHighlighter(QObject *parent = 0);
    virtual ~SemanticHighlighter();

    void abort();
    void rehighlight(const SemanticHighlighterSource &source);
    void setModelManager(QmlJS::ModelManagerInterface *modelManager);

Q_SIGNALS:
    void changed(const QmlJSEditor::SemanticInfo &semanticInfo);

protected:
    virtual void run();

private:
    bool isOutdated();
    SemanticInfo semanticInfo(const SemanticHighlighterSource &source);

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_done;
    SemanticHighlighterSource m_source;
    SemanticInfo m_lastSemanticInfo;
    QmlJS::ModelManagerInterface *m_modelManager;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // SEMANTICHIGHLIGHTER_H
