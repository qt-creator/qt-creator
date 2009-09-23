/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef CPPFINDREFERENCES_H
#define CPPFINDREFERENCES_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <utils/filesearch.h>

namespace Find {
class SearchResultWindow;
} // end of namespace Find

namespace CppTools {
namespace Internal {

class CppModelManager;

class CppFindReferences: public QObject
{
    Q_OBJECT

public:
    CppFindReferences(CppModelManager *modelManager);
    virtual ~CppFindReferences();

Q_SIGNALS:
    void changed();

public:
    void findAll(const QString &fileName, const QString &text);

private Q_SLOTS:
    void displayResult(int);
    void searchFinished();
    void openEditor(const QString&, int, int);

private:
    QPointer<CppModelManager> _modelManager;
    Find::SearchResultWindow *_resultWindow;
    QFutureWatcher<Core::Utils::FileSearchResult> m_watcher;
};

} // end of namespace Internal
} // end of namespace CppTools

#endif // CPPFINDREFERENCES_H
