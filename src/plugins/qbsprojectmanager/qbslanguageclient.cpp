// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbslanguageclient.h"

#include "qbsproject.h"
#include "qbssettings.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientsettings.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/filepath.h>
#include <utils/mimeconstants.h>

#include <QPointer>

using namespace Core;
using namespace LanguageClient;
using namespace TextEditor;
using namespace Utils;

namespace QbsProjectManager::Internal {

class QbsLanguageClientInterface : public LocalSocketClientInterface
{
public:
    QbsLanguageClientInterface(const QString &serverPath)
        : LocalSocketClientInterface(serverPath),
          m_qbsExecutable(QbsSettings::qbsExecutableFilePath()) {}

private:
    Utils::FilePath serverDeviceTemplate() const override{ return m_qbsExecutable; };

    const FilePath m_qbsExecutable;
};

class QbsLanguageClient::Private
{
public:
    Private(QbsLanguageClient * q) : q(q) {}

    void checkDocument(IDocument *document);

    QbsLanguageClient * const q;
    QPointer<QbsBuildSystem> buildSystem;
};


QbsLanguageClient::QbsLanguageClient(const QString &serverPath, QbsBuildSystem *buildSystem)
    : Client(new QbsLanguageClientInterface(serverPath)), d(new Private(this))
{
    d->buildSystem = buildSystem;
    setName(QString::fromLatin1("qbs@%1").arg(serverPath));
    setCurrentProject(buildSystem->project());
    LanguageFilter langFilter;
    langFilter.mimeTypes << Utils::Constants::QBS_MIMETYPE;
    setSupportedLanguage(langFilter);
    connect(EditorManager::instance(), &EditorManager::documentOpened,
            this, [this](IDocument *document) { d->checkDocument(document); });
    const QList<IDocument *> &allDocuments = DocumentModel::openedDocuments();
    for (IDocument * const document : allDocuments)
        d->checkDocument(document);
    start();
}

QbsLanguageClient::~QbsLanguageClient() { delete d; }

bool QbsLanguageClient::isActive() const
{
    if (!d->buildSystem)
        return false;
    if (!d->buildSystem->target()->activeBuildConfiguration())
        return false;
    if (d->buildSystem->target()->activeBuildConfiguration()->buildSystem() != d->buildSystem)
        return false;
    if (d->buildSystem->project()->activeTarget() != d->buildSystem->target())
        return false;
    return true;
}

void QbsLanguageClient::Private::checkDocument(IDocument *document)
{
    if (const auto doc = qobject_cast<TextDocument *>(document))
        q->openDocument(doc);
}

} // namespace QbsProjectManager::Internal
