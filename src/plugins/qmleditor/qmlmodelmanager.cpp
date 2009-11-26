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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QFile>
#include <QtConcurrentRun>
#include <qtconcurrent/runextensions.h>
#include <QTextStream>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <texteditor/itexteditor.h>

#include "qmleditorconstants.h"
#include "qmlmodelmanager.h"

#include <QtCore/QMetaType>

using namespace QmlEditor;
using namespace QmlEditor::Internal;

QmlModelManager::QmlModelManager(QObject *parent):
        QmlModelManagerInterface(parent),
        m_core(Core::ICore::instance())
{
    m_synchronizer.setCancelOnWait(true);

    qRegisterMetaType<QmlEditor::QmlDocument::Ptr>("QmlEditor::QmlDocument::Ptr");

    connect(this, SIGNAL(documentUpdated(QmlEditor::QmlDocument::Ptr)),
            this, SLOT(onDocumentUpdated(QmlEditor::QmlDocument::Ptr)));
}

Snapshot QmlModelManager::snapshot() const
{
    QMutexLocker locker(&m_mutex);

    return _snapshot;
}

void QmlModelManager::updateSourceFiles(const QStringList &files)
{
    refreshSourceFiles(files);
}

QFuture<void> QmlModelManager::refreshSourceFiles(const QStringList &sourceFiles)
{
    if (sourceFiles.isEmpty()) {
        return QFuture<void>();
    }

    const QMap<QString, QString> workingCopy = buildWorkingCopyList();

    QFuture<void> result = QtConcurrent::run(&QmlModelManager::parse,
                                              workingCopy, sourceFiles,
                                              this);

    if (m_synchronizer.futures().size() > 10) {
        QList<QFuture<void> > futures = m_synchronizer.futures();

        m_synchronizer.clearFutures();

        foreach (QFuture<void> future, futures) {
            if (! (future.isFinished() || future.isCanceled()))
                m_synchronizer.addFuture(future);
        }
    }

    m_synchronizer.addFuture(result);

    if (sourceFiles.count() > 1) {
        m_core->progressManager()->addTask(result, tr("Indexing"),
                        QmlEditor::Constants::TASK_INDEX);
    }

    return result;
}

QMap<QString, QString> QmlModelManager::buildWorkingCopyList()
{
    QMap<QString, QString> workingCopy;
    Core::EditorManager *editorManager = m_core->editorManager();

    foreach (Core::IEditor *editor, editorManager->openedEditors()) {
        const QString key = editor->file()->fileName();

        if (TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor)) {
            workingCopy[key] = textEditor->contents();
        }
    }

    return workingCopy;
}

void QmlModelManager::emitDocumentUpdated(QmlDocument::Ptr doc)
{ emit documentUpdated(doc); }

void QmlModelManager::onDocumentUpdated(QmlEditor::QmlDocument::Ptr doc)
{
    QMutexLocker locker(&m_mutex);

    _snapshot.insert(doc);
}

void QmlModelManager::parse(QFutureInterface<void> &future,
                            QMap<QString, QString> workingCopy,
                            QStringList files,
                            QmlModelManager *modelManager)
{
    future.setProgressRange(0, files.size());

    for (int i = 0; i < files.size(); ++i) {
        future.setProgressValue(i);

        const QString fileName = files.at(i);
        QString contents;

        if (workingCopy.contains(fileName)) {
            contents = workingCopy.value(fileName);
        } else {
            QFile inFile(fileName);

            if (inFile.open(QIODevice::ReadOnly)) {
                QTextStream ins(&inFile);
                contents = ins.readAll();
                inFile.close();
            }
        }

        QmlDocument::Ptr doc = QmlDocument::create(fileName);
        doc->setSource(contents);
        doc->parse();

        modelManager->emitDocumentUpdated(doc);
    }

    future.setProgressValue(files.size());
}
