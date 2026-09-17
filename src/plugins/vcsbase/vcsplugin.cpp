// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsplugin.h"

#include "commonvcssettings.h"
#include "nicknamedialog.h"
#include "vcsbaseconstants.h"
#include "vcsbasetr.h"
#include "vcschangesview.h"
#include "vcsoutputformatter.h"
#include "vcsoutputwindow.h"
#include "wizard/vcscommandpage.h"
#include "wizard/vcsconfigurationpage.h"
#include "wizard/vcsjsextension.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDebug>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace VcsBase::Internal {

#ifdef WITH_TESTS

class VcsOutputFormatterTest final : public QObject
{
    Q_OBJECT

private slots:
    void testLinkHelpers_data();
    void testLinkHelpers();
    void testLinkDetection_data();
    void testLinkDetection();
};

void VcsOutputFormatterTest::testLinkHelpers_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("isRevision");
    QTest::addColumn<bool>("shouldOfferFileLink");

    QTest::newRow("plain path") << QString("file.cpp") << QString("file.cpp") << false << true;
    QTest::newRow("path with spaces") << QString("\"file with spaces.cpp\"")
                                      << QString("file with spaces.cpp") << false << true;
    QTest::newRow("escaped quote") << QString("\"file\\\"name.cpp\"")
                                    << QString("file\"name.cpp") << false << true;
    QTest::newRow("octal escape")
        << QString::fromLatin1("\"file\\303\\244.cpp\"")
        << QString::fromUtf8("fileä.cpp") << false << true;
    QTest::newRow("tag") << QString("v1.2.3") << QString("v1.2.3") << true << false;
    QTest::newRow("revision") << QString("0123456789abcdef")
                               << QString("0123456789abcdef") << true << false;
    QTest::newRow("revision filename collision") << QString("deadbeef")
                                                  << QString("deadbeef") << true << false;
    QTest::newRow("revision parent") << QString("0123456~2") << QString("0123456~2") << true
                                      << false;
    QTest::newRow("ordinary name") << QString("file0123456") << QString("file0123456") << false
                                    << true;
}

void VcsOutputFormatterTest::testLinkHelpers()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(bool, isRevision);
    QFETCH(bool, shouldOfferFileLink);

    QCOMPARE(VcsOutputLineParser::unquoteGitPath(input), expected);
    QCOMPARE(VcsOutputLineParser::isRevisionLink(input), isRevision);
    QCOMPARE(VcsOutputLineParser::shouldOfferFileLink(input), shouldOfferFileLink);
}

void VcsOutputFormatterTest::testLinkDetection_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QStringList>("targets");
    QTest::addColumn<bool>("hasLinks");

    QTest::newRow("tag") << QString("git checkout v1.2.3") << QStringList{"v1.2.3"} << true;
    QTest::newRow("tag with suffix") << QString("git checkout v1.2.3-rc1")
                                     << QStringList{"v1.2.3-rc1"} << true;
    QTest::newRow("hash") << QString("git show 0123456789abcdef ")
                           << QStringList{"0123456789abcdef"} << true;
    QTest::newRow("six-character hash") << QString("git show 012345 ")
                                        << QStringList{"012345"} << true;
    QTest::newRow("short hash") << QString("git show 01234 ") << QStringList{} << false;
    QTest::newRow("hash range") << QString("git diff 012345..abcdef ")
                                 << QStringList{"012345..abcdef"} << true;
    QTest::newRow("hash three-dot range") << QString("git diff 012345...abcdef ")
                                          << QStringList{"012345...abcdef"} << true;
    QTest::newRow("hash parent") << QString("git show 012345^ ") << QStringList{"012345^"}
                                  << true;
    QTest::newRow("hash ancestor") << QString("git show 012345~2 ") << QStringList{"012345~2"}
                                    << true;
    QTest::newRow("quoted hash") << QString("git show \"0123456789abcdef\"")
                                 << QStringList{"0123456789abcdef"} << true;
    QTest::newRow("git paths") << QString("a/src/file.cpp b/include/file.h")
                               << QStringList{"src/file.cpp", "include/file.h"} << true;
    QTest::newRow("url") << QString("See https://example.org/change/123")
                          << QStringList{"https://example.org/change/123"} << true;
    QTest::newRow("http url") << QString("See http://example.org/change/123")
                              << QStringList{"http://example.org/change/123"} << true;
    QTest::newRow("url with punctuation") << QString("See https://example.org/change/123,")
                                          << QStringList{"https://example.org/change/123"} << true;
    QTest::newRow("tab delimiters") << QString("git\tshow\t0123456789abcdef\t")
                                    << QStringList{"0123456789abcdef"} << true;
    QTest::newRow("multiple spaces") << QString("git  show  0123456789abcdef  ")
                                     << QStringList{"0123456789abcdef"} << true;
    QTest::newRow("multiple hashes") << QString("git diff 012345 6789ab ")
                                     << QStringList{"012345", "6789ab"} << true;
    QTest::newRow("mixed links")
        << QString("git v1.2.3 0123456789abcdef a/src/file.cpp https://example.org/change/123")
        << QStringList{"v1.2.3", "0123456789abcdef", "src/file.cpp",
                       "https://example.org/change/123"}
        << true;
    QTest::newRow("mode") << QString("mode 100644") << QStringList{} << false;
    QTest::newRow("hash with prefix") << QString("prefix0123456789abcdef") << QStringList{}
                                      << false;
    QTest::newRow("hash with suffix") << QString("0123456789abcdefsuffix") << QStringList{}
                                      << false;
    QTest::newRow("hash with punctuation") << QString("git show 0123456789abcdef)")
                                           << QStringList{} << false;
}

void VcsOutputFormatterTest::testLinkDetection()
{
    QFETCH(QString, text);
    QFETCH(QStringList, targets);
    QFETCH(bool, hasLinks);

    VcsOutputLineParser parser;
    OutputLineParser &lineParser = parser;
    const auto result = lineParser.handleLine(text, OutputFormat::StdOutFormat);

    QCOMPARE(result.status, hasLinks ? OutputLineParser::Status::Done
                                     : OutputLineParser::Status::NotHandled);
    QCOMPARE(result.linkSpecs.size(), targets.size());
    for (int i = 0; i < targets.size(); ++i) {
        const auto &link = result.linkSpecs.at(i);
        QCOMPARE(link.target, targets.at(i));
        QCOMPARE(link.startPos, text.indexOf(link.target));
        QCOMPARE(link.length, link.target.size());
    }
}

#endif

class VcsPluginPrivate
{
public:
    explicit VcsPluginPrivate(VcsPlugin *plugin)
        : q(plugin)
    {
        QObject::connect(&commonSettings(), &AspectContainer::changed, q,
                         [this] { slotSettingsChanged(); });
        slotSettingsChanged();
    }

    QStandardItemModel *nickNameModel()
    {
        if (!m_nickNameModel) {
            m_nickNameModel = NickNameDialog::createModel(q);
            populateNickNameModel();
        }
        return m_nickNameModel;
    }

    void populateNickNameModel()
    {
        const Result<> res =
                NickNameDialog::populateModelFromMailCapFile(commonSettings().nickNameMailMap(),
                                                             m_nickNameModel);
        if (!res) {
            VcsOutputWindow::appendError(
                {},
                Tr::tr("Cannot open user/alias configuration file: %1").arg(res.error()));
        }
    }

    void slotSettingsChanged()
    {
        if (m_nickNameModel)
            populateNickNameModel();
    }

    VcsPlugin *q;
    QStandardItemModel *m_nickNameModel = nullptr;

    VcsConfigurationPageFactory m_vcsConfigurationPageFactory;
    VcsCommandPageFactory m_vcsCommandPageFactory;
    ChangesViewFactory m_changesViewFactory;
};

static VcsPlugin *m_instance = nullptr;

VcsPlugin::VcsPlugin()
{
    m_instance = this;
}

VcsPlugin::~VcsPlugin()
{
    QTC_ASSERT(d, return);
    VcsOutputWindow::destroy();
    m_instance = nullptr;
    delete d;
}

void VcsPlugin::initialize()
{
    d = new VcsPluginPrivate(this);

#ifdef WITH_TESTS
    addTest<VcsOutputFormatterTest>();
#endif

    IOptionsPage::registerCategory(
        Constants::VCS_SETTINGS_CATEGORY,
        Tr::tr("Version Control"),
        ":/vcsbase/images/settingscategory_vcs.png");

    JsExpander::registerGlobalObject<VcsJsExtension>("Vcs");

    MacroExpander *expander = globalMacroExpander();
    expander->registerVariable(Constants::VAR_VCS_NAME,
        Tr::tr("Name of the version control system in use by the current project."), [] {
            IVersionControl *vc = nullptr;
            if (Project *project = ProjectTree::currentProject())
                vc = VcsManager::findVersionControlForDirectory(project->projectDirectory());
            return vc ? vc->displayName() : QString();
        });

    expander->registerVariable(Constants::VAR_VCS_TOPIC,
        Tr::tr("The current version control topic (branch or tag) identification "
               "of the current project."), [] {
            IVersionControl *vc = nullptr;
            FilePath topLevel;
            if (Project *project = ProjectTree::currentProject())
                vc = VcsManager::findVersionControlForDirectory(project->projectDirectory(), &topLevel);
            return vc ? vc->vcsTopic(topLevel) : QString();
        });

    expander->registerVariable(Constants::VAR_VCS_TOPLEVELPATH,
        Tr::tr("The top level path to the repository the current project is in."), [] {
            if (Project *project = ProjectTree::currentProject())
                return VcsManager::findTopLevelForDirectory(project->projectDirectory()).toUrlishString();
            return QString();
        });

    // Just touch Version Control Output Pane before initialization
    VcsOutputWindow::instance();
}

VcsPlugin *VcsPlugin::instance()
{
    return m_instance;
}

/* Delayed creation/update of the nick name model. */
QStandardItemModel *VcsPlugin::nickNameModel()
{
    QTC_ASSERT(d, return nullptr);
    return d->nickNameModel();
}

} // VcsBase::Internal

#include "vcsplugin.moc"
