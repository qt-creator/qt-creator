/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qmljseditordocument.h"

#include "qmljseditordocument_p.h"
#include "qmljshighlighter.h"

#include <qmljstools/qmljsqtstylecodeformatter.h>
#include <qmljstools/qmljsmodelmanager.h>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

namespace {

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100
};

}

QmlJSEditorDocumentPrivate::QmlJSEditorDocumentPrivate(QmlJSEditorDocument *parent)
    : m_q(parent)
{
    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_q->document(), SIGNAL(contentsChanged()), m_updateDocumentTimer, SLOT(start()));
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(reparseDocument()));
}

void QmlJSEditorDocumentPrivate::invalidateFormatterCache()
{
    QmlJSTools::CreatorCodeFormatter formatter(m_q->tabSettings());
    formatter.invalidateCache(m_q->document());
}

void QmlJSEditorDocumentPrivate::reparseDocument()
{
    QmlJS::ModelManagerInterface::instance()->updateSourceFiles(QStringList() << m_q->filePath(),
                                                                false);
}

QmlJSEditorDocument::QmlJSEditorDocument()
    : m_d(new QmlJSEditorDocumentPrivate(this))
{
    connect(this, SIGNAL(tabSettingsChanged()),
            m_d, SLOT(invalidateFormatterCache()));
    setSyntaxHighlighter(new Highlighter(document()));
}

QmlJSEditorDocument::~QmlJSEditorDocument()
{
    delete m_d;
}
