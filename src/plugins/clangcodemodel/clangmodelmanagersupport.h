/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLANGCODEMODEL_INTERNAL_CLANGMODELMANAGERSUPPORT_H
#define CLANGCODEMODEL_INTERNAL_CLANGMODELMANAGERSUPPORT_H

#include "clangcompletionassistprovider.h"

#include <cpptools/cppmodelmanagersupport.h>

#include <QObject>
#include <QScopedPointer>

namespace Core { class IDocument; }

namespace ClangCodeModel {
namespace Internal {

class ModelManagerSupportClang:
        public QObject,
        public CppTools::ModelManagerSupport
{
    Q_OBJECT

public:
    ModelManagerSupportClang();
    ~ModelManagerSupportClang();

    CppTools::CppCompletionAssistProvider *completionAssistProvider() override;
    CppTools::BaseEditorDocumentProcessor *editorDocumentProcessor(
                TextEditor::TextDocument *baseTextDocument) override;

    IpcCommunicator &ipcCommunicator();

#ifdef QT_TESTLIB_LIB
    static ModelManagerSupportClang *instance_forTestsOnly();
#endif

private:
    void onEditorOpened(Core::IEditor *editor);
    void onCurrentEditorChanged(Core::IEditor *newCurrent);
    void onCppDocumentReloadFinished(bool success);
    void onCppDocumentContentsChanged();

    void onAbstractEditorSupportContentsUpdated(const QString &filePath, const QByteArray &content);
    void onAbstractEditorSupportRemoved(const QString &filePath);

    void onProjectPartsUpdated(ProjectExplorer::Project *project);
    void onProjectPartsRemoved(const QStringList &projectPartIds);

    IpcCommunicator m_ipcCommunicator;
    ClangCompletionAssistProvider m_completionAssistProvider;
    QPointer<Core::IEditor> m_previousCppEditor;
};

class ModelManagerSupportProviderClang : public CppTools::ModelManagerSupportProvider
{
public:
    QString id() const override;
    QString displayName() const override;

    CppTools::ModelManagerSupport::Ptr createModelManagerSupport() override;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_INTERNAL_CLANGMODELMANAGERSUPPORT_H
