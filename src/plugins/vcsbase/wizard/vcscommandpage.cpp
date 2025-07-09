// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcscommandpage.h"

#include "../vcsbaseplugin.h"
#include "../vcsbasetr.h"

#include <coreplugin/vcsmanager.h>

#include <projectexplorer/jsonwizard/jsonwizard.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace VcsBase {
namespace Internal {

static char VCSCOMMAND_VCSID[] = "vcsId";
static char VCSCOMMAND_RUN_MESSAGE[] = "trRunMessage";
static char VCSCOMMAND_REPO[] = "repository";
static char VCSCOMMAND_DIR[] = "baseDirectory";
static char VCSCOMMAND_EXTRA_ARGS[] = "extraArguments";
static char VCSCOMMAND_CHECKOUTNAME[] = "checkoutName";

// ----------------------------------------------------------------------
// VcsCommandPageFactory:
// ----------------------------------------------------------------------

VcsCommandPageFactory::VcsCommandPageFactory()
{
    setTypeIdsSuffix(QLatin1String("VcsCommand"));
}

WizardPage *VcsCommandPageFactory::create(JsonWizard *wizard, Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    QVariantMap tmp = data.toMap();

    auto page = new VcsCommandPage;
    page->setVersionControlId(tmp.value(QLatin1String(VCSCOMMAND_VCSID)).toString());
    page->setRunMessage(tmp.value(QLatin1String(VCSCOMMAND_RUN_MESSAGE)).toString());

    QStringList args;
    const QVariant argsVar = tmp.value(QLatin1String(VCSCOMMAND_EXTRA_ARGS));
    if (!argsVar.isNull()) {
        if (argsVar.typeId() == QMetaType::QString) {
            args << argsVar.toString();
        } else if (argsVar.typeId() == QMetaType::QVariantList) {
            args = Utils::transform(argsVar.toList(), &QVariant::toString);
        } else {
            return nullptr;
        }
    }

    page->setCheckoutData(tmp.value(QLatin1String(VCSCOMMAND_REPO)).toString(),
                          tmp.value(QLatin1String(VCSCOMMAND_DIR)).toString(),
                          tmp.value(QLatin1String(VCSCOMMAND_CHECKOUTNAME)).toString(),
                          args);
    return page;
}

Result<> VcsCommandPageFactory::validateData(Id typeId, const QVariant &data)
{
    QTC_ASSERT(canCreate(typeId), return ResultError(ResultAssert));

    QString em;
    if (data.typeId() != QMetaType::QVariantMap)
        em = Tr::tr("\"data\" is no JSON object in \"VcsCommand\" page.");

    if (em.isEmpty()) {
        const QVariantMap tmp = data.toMap();

        QString str = tmp.value(QLatin1String(VCSCOMMAND_VCSID)).toString();
        if (str.isEmpty()) {
            em = Tr::tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_VCSID));
        }
        str = tmp.value(QLatin1String(VCSCOMMAND_REPO)).toString();
        if (str.isEmpty()) {
            em = Tr::tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_REPO));
        }
        str = tmp.value(QLatin1String(VCSCOMMAND_DIR)).toString();
        if (str.isEmpty()) {
            em = Tr::tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_DIR));;
        }
        str = tmp.value(QLatin1String(VCSCOMMAND_CHECKOUTNAME)).toString();
        if (str.isEmpty()) {
            em = Tr::tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_CHECKOUTNAME));
        }

        str = tmp.value(QLatin1String(VCSCOMMAND_RUN_MESSAGE)).toString();
        if (str.isEmpty()) {
            em = Tr::tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_RUN_MESSAGE));
        }

        const QVariant extra = tmp.value(QLatin1String(VCSCOMMAND_EXTRA_ARGS));
        if (!extra.isNull() && extra.typeId() != QMetaType::QString && extra.typeId() != QMetaType::QVariantList) {
            em = Tr::tr("\"%1\" in \"data\" section of \"VcsCommand\" page has unexpected type (unset, String or List).")
                    .arg(QLatin1String(VCSCOMMAND_EXTRA_ARGS));
        }
    }

    if (!em.isEmpty())
        return ResultError(em);

    return ResultOk;
}

// ----------------------------------------------------------------------
// VcsCommandPage:
// ----------------------------------------------------------------------

/*!
    \class VcsBase::VcsCommandPage

    \brief The VcsCommandPage implements a page showing the
    progress of a \c VcsCommand.

    Turns complete when the command succeeds.
*/

VcsCommandPage::VcsCommandPage()
    : m_startedStatus(Tr::tr("Command started..."))
{
    auto verticalLayout = new QVBoxLayout(this);
    m_logPlainTextEdit = new QPlainTextEdit;
    m_formatter = new OutputFormatter;
    m_logPlainTextEdit->setReadOnly(true);
    m_formatter->setPlainTextEdit(m_logPlainTextEdit);

    verticalLayout->addWidget(m_logPlainTextEdit);

    m_statusLabel = new QLabel;
    verticalLayout->addWidget(m_statusLabel);
    setTitle(Tr::tr("Checkout"));
}

VcsCommandPage::~VcsCommandPage()
{
    m_taskTreeRunner.cancel();
    delete m_formatter;
}


void VcsCommandPage::initializePage()
{
    // Delay real initialization till after QWizard is done with its setup.
    // Otherwise QWizard will reset our disabled back button again.
    QTimer::singleShot(0, this, &VcsCommandPage::delayedInitialize);
}

bool VcsCommandPage::isComplete() const
{
    return m_isComplete;
}

bool VcsCommandPage::handleReject()
{
    if (!m_taskTreeRunner.isRunning())
        return false;

    m_taskTreeRunner.cancel();
    return true;
}

void VcsCommandPage::delayedInitialize()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    const QString vcsId = wiz->expander()->expand(m_vcsId);
    VersionControlBase *vc = static_cast<VersionControlBase *>(
                VcsManager::versionControl(Id::fromString(vcsId)));
    if (!vc) {
        qWarning() << Tr::tr("\"%1\" (%2) not found.")
                          .arg(QLatin1String(VCSCOMMAND_VCSID), vcsId);
        return;
    }
    if (!vc->isConfigured()) {
        qWarning() << Tr::tr("Version control \"%1\" is not configured.")
                          .arg(vcsId);
        return;
    }
    if (!vc->supportsOperation(IVersionControl::InitialCheckoutOperation)) {
        qWarning() << Tr::tr("Version control \"%1\" does not support initial checkouts.")
                          .arg(vcsId);
        return;
    }

    const QString repo = wiz->expander()->expand(m_repository);
    if (repo.isEmpty()) {
        qWarning() << Tr::tr("\"%1\" is empty when trying to run checkout.")
                      .arg(QLatin1String(VCSCOMMAND_REPO));
        return;
    }

    const QString base = wiz->expander()->expand(m_directory);
    if (!QDir(base).exists()) {
        qWarning() << Tr::tr("\"%1\" (%2) does not exist.")
                      .arg(QLatin1String(VCSCOMMAND_DIR), base);
        return;
    }

    const QString name = wiz->expander()->expand(m_name);
    if (name.isEmpty()) {
        qWarning() << Tr::tr("\"%1\" is empty when trying to run checkout.")
                      .arg(QLatin1String(VCSCOMMAND_CHECKOUTNAME));
        return;
    }

    const QString runMessage = wiz->expander()->expand(m_runMessage);
    if (!runMessage.isEmpty())
        m_startedStatus = runMessage;

    QStringList extraArgs;
    for (const QString &in : std::as_const(m_arguments)) {
        const QString tmp = wiz->expander()->expand(in);
        if (tmp.isEmpty())
           continue;
        if (tmp == QStringLiteral("\"\""))
            extraArgs << QString();
        extraArgs << tmp;
    }

    const auto outCallback = [formatter = QPointer<OutputFormatter>(m_formatter)](const QString &text) {
        if (formatter)
            formatter->appendMessage(text, StdOutFormat);
    };
    const auto errCallback = [formatter = QPointer<OutputFormatter>(m_formatter)](const QString &text) {
        if (formatter)
            formatter->appendMessage(text, StdErrFormat);
    };

    const auto onSetup = [this] {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        m_logPlainTextEdit->clear();
        m_overwriteOutput = false;
        m_statusLabel->setText(m_startedStatus);
        m_statusLabel->setPalette(QPalette());

        wizard()->button(QWizard::BackButton)->setEnabled(false);
    };

    const auto onDone = [this](DoneWith result) {
        QApplication::restoreOverrideCursor();

        m_isComplete = result == DoneWith::Success;
        QPalette palette;
        const Theme::Color role = m_isComplete ? Theme::TextColorNormal : Theme::TextColorError;
        palette.setColor(QPalette::WindowText, creatorColor(role).name());
        m_statusLabel->setPalette(palette);
        m_statusLabel->setText(m_isComplete ? Tr::tr("Succeeded.") : Tr::tr("Failed."));

        wizard()->button(QWizard::BackButton)->setEnabled(true);

        if (result == DoneWith::Success)
            emit completeChanged();
    };

    const Group recipe {
        onGroupSetup(onSetup),
        vc->cloneTask({repo, FilePath::fromString(base), name, extraArgs, outCallback, errCallback}),
        onGroupDone(onDone)
    };

    m_taskTreeRunner.start(recipe);
}

void VcsCommandPage::setCheckoutData(const QString &repo, const QString &baseDir, const QString &name,
                                  const QStringList &args)
{
    m_repository = repo;
    m_directory = baseDir;
    m_name = name;
    m_arguments = args;
}

void VcsCommandPage::setVersionControlId(const QString &id)
{
    m_vcsId = id;
}

void VcsCommandPage::setRunMessage(const QString &msg)
{
    m_runMessage = msg;
}

} // namespace Internal
} // namespace VcsBase
