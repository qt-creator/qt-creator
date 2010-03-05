/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "documentcloser.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

#include <QtGui/QMessageBox>

#include "designdocumentcontroller.h"

namespace QmlDesigner {

DocumentCloser::DocumentCloser():
        m_quitWhenDone(false)
{
}

void DocumentCloser::close(DesignDocumentController* designDocument)
{
    DocumentCloser* closer = new DocumentCloser;
    closer->m_designDocuments.append(designDocument);
    closer->runClose(designDocument);
}

void DocumentCloser::runClose(DesignDocumentController* designDocument)
{
    bool isDirty = designDocument->isDirty();

//#ifndef QT_NO_DEBUG
//    isDirty=false;
//#endif

    if (isDirty) {
        QMessageBox* msgBox = new QMessageBox(designDocument->documentWidget());
        QString txt = tr("Do you want to save the changes you made in the document \"%1\"?");
        QString shortFileName = QFileInfo(designDocument->fileName()).baseName();
        msgBox->setText(txt.arg(shortFileName));
        msgBox->setInformativeText(tr("Your changes will be lost if you don't save them."));
        msgBox->setIcon(QMessageBox::Question);
        msgBox->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox->setDefaultButton(QMessageBox::Save);
        msgBox->setEscapeButton(QMessageBox::Cancel);
        msgBox->resize(451, 131);
        msgBox->open(this, SLOT(choiceMade()));
    } else {
        designDocument->close();
        closeDone();
    }
}

void DocumentCloser::choiceMade()
{
    QMessageBox* msgBox = dynamic_cast<QMessageBox*>(sender());
    if (!msgBox)
        return;

    switch (msgBox->result()) {
        case QMessageBox::Save:
            m_designDocuments.first()->save();

        case QMessageBox::Discard:
            m_designDocuments.first()->close();
            closeDone();
            break;

        case QMessageBox::Cancel:
        default:
            closeCancelled();
    }
}

void DocumentCloser::closeCancelled()
{
    delete this;
}

void DocumentCloser::closeDone()
{
    if (!m_designDocuments.isEmpty()) // should always be the case, but let's be safe.
        m_designDocuments.removeFirst();

    bool quit = m_quitWhenDone;

    if (m_designDocuments.isEmpty()) {
        delete this;

        if (quit) {
            qApp->quit();
        }
    } else {
        runClose(m_designDocuments.first().data());
    }
}

void DocumentCloser::close(QList<QWeakPointer<DesignDocumentController> > designDocuments, bool quitWhenAllEditorsClosed)
{
    if (designDocuments.isEmpty()) {
        if (quitWhenAllEditorsClosed) {
            qApp->quit();
        }
    } else {
        DocumentCloser* closer = new DocumentCloser;
        closer->m_designDocuments = designDocuments;
        closer->m_quitWhenDone = quitWhenAllEditorsClosed;
        closer->runClose(closer->m_designDocuments.first().data());
    }
}

} // namespace QmlDesigner

