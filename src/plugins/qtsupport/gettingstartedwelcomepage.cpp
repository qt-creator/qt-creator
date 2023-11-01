// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gettingstartedwelcomepage.h"

#include "exampleslistmodel.h"
#include "examplesparser.h"
#include "qtsupporttr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/welcomepagehelper.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/winutils.h>

#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace QtSupport {
namespace Internal {

const char C_FALLBACK_ROOT[] = "ProjectsFallbackRoot";

Q_GLOBAL_STATIC(ExampleSetModel, s_exampleSetModel)

ExamplesWelcomePage::ExamplesWelcomePage(bool showExamples)
    : m_showExamples(showExamples)
{
}

QString ExamplesWelcomePage::title() const
{
    return m_showExamples ? Tr::tr("Examples") : Tr::tr("Tutorials");
}

int ExamplesWelcomePage::priority() const
{
    return m_showExamples ? 30 : 40;
}

Id ExamplesWelcomePage::id() const
{
    return m_showExamples ? "Examples" : "Tutorials";
}

FilePath ExamplesWelcomePage::copyToAlternativeLocation(const FilePath &proFile,
                                                        FilePaths &filesToOpen,
                                                        const FilePaths &dependencies)
{
    const FilePath projectDir = proFile.canonicalPath().parentDir();
    QDialog d(ICore::dialogParent());
    auto lay = new QGridLayout(&d);
    auto descrLbl = new QLabel;
    d.setWindowTitle(Tr::tr("Copy Project to writable Location?"));
    descrLbl->setTextFormat(Qt::RichText);
    descrLbl->setWordWrap(false);
    const QString nativeProjectDir = projectDir.toUserOutput();
    descrLbl->setText(QString::fromLatin1("<blockquote>%1</blockquote>").arg(nativeProjectDir));
    descrLbl->setMinimumWidth(descrLbl->sizeHint().width());
    descrLbl->setWordWrap(true);
    descrLbl->setText(Tr::tr("<p>The project you are about to open is located in the "
                             "write-protected location:</p><blockquote>%1</blockquote>"
                             "<p>Please select a writable location below and click \"Copy Project and Open\" "
                             "to open a modifiable copy of the project or click \"Keep Project and Open\" "
                             "to open the project in location.</p><p><b>Note:</b> You will not "
                             "be able to alter or compile your project in the current location.</p>")
                      .arg(nativeProjectDir));
    lay->addWidget(descrLbl, 0, 0, 1, 2);
    auto txt = new QLabel(Tr::tr("&Location:"));
    auto chooser = new PathChooser;
    txt->setBuddy(chooser);
    chooser->setExpectedKind(PathChooser::ExistingDirectory);
    chooser->setHistoryCompleter("Qt.WritableExamplesDir.History");
    const FilePath defaultRootDirectory = DocumentManager::projectsDirectory();
    QtcSettings *settings = ICore::settings();
    chooser->setFilePath(
        FilePath::fromSettings(settings->value(C_FALLBACK_ROOT, defaultRootDirectory.toVariant())));
    lay->addWidget(txt, 1, 0);
    lay->addWidget(chooser, 1, 1);
    enum { Copy = QDialog::Accepted + 1, Keep = QDialog::Accepted + 2 };
    auto bb = new QDialogButtonBox;
    QPushButton *copyBtn = bb->addButton(Tr::tr("&Copy Project and Open"), QDialogButtonBox::AcceptRole);
    connect(copyBtn, &QAbstractButton::released, &d, [&d] { d.done(Copy); });
    copyBtn->setDefault(true);
    QPushButton *keepBtn = bb->addButton(Tr::tr("&Keep Project and Open"), QDialogButtonBox::RejectRole);
    connect(keepBtn, &QAbstractButton::released, &d, [&d] { d.done(Keep); });
    lay->addWidget(bb, 2, 0, 1, 2);
    connect(chooser, &PathChooser::validChanged, copyBtn, &QWidget::setEnabled);
    int code = d.exec();
    if (code == Copy) {
        const QString exampleDirName = projectDir.fileName();
        const FilePath destBaseDir = chooser->filePath();
        settings->setValueWithDefault(C_FALLBACK_ROOT, destBaseDir.toSettings(),
                                                       defaultRootDirectory.toSettings());
        const FilePath targetDir = destBaseDir / exampleDirName;
        if (targetDir.exists()) {
            QMessageBox::warning(ICore::dialogParent(),
                                 Tr::tr("Cannot Use Location"),
                                 Tr::tr("The specified location already exists. "
                                        "Please specify a valid location."),
                                 QMessageBox::Ok,
                                 QMessageBox::NoButton);
            return {};
        } else {
            expected_str<void> result = projectDir.copyRecursively(targetDir);

            if (result) {
                // set vars to new location
                const FilePaths::Iterator end = filesToOpen.end();
                for (FilePaths::Iterator it = filesToOpen.begin(); it != end; ++it) {
                    const FilePath relativePath = it->relativeChildPath(projectDir);
                    *it = targetDir.resolvePath(relativePath);
                }

                for (const FilePath &dependency : dependencies) {
                    const FilePath targetFile = targetDir.pathAppended(dependency.fileName());
                    result = dependency.copyRecursively(targetFile);
                    if (!result) {
                        QMessageBox::warning(ICore::dialogParent(),
                                             Tr::tr("Cannot Copy Project"),
                                             result.error());
                        // do not fail, just warn;
                    }
                }

                return targetDir / proFile.fileName();
            } else {
                QMessageBox::warning(ICore::dialogParent(),
                                     Tr::tr("Cannot Copy Project"),
                                     result.error());
            }
        }
    }
    if (code == Keep)
        return proFile.absoluteFilePath();
    return {};
}

void ExamplesWelcomePage::openProject(const ExampleItem *item)
{
    using namespace ProjectExplorer;
    FilePath proFile = item->projectPath;
    if (proFile.isEmpty())
        return;

    FilePaths filesToOpen = item->filesToOpen;
    if (!item->mainFile.isEmpty()) {
        // ensure that the main file is opened on top (i.e. opened last)
        filesToOpen.removeAll(item->mainFile);
        filesToOpen.append(item->mainFile);
    }

    if (!proFile.exists())
        return;

    // If the Qt is a distro Qt on Linux, it will not be writable, hence compilation will fail
    // Same if it is installed in non-writable location for other reasons
    const bool needsCopy = withNtfsPermissions<bool>([proFile] {
        return !proFile.isWritableFile()
               || !proFile.parentDir().isWritableDir() /* path of project file */
               || !proFile.parentDir().parentDir().isWritableDir() /* shadow build directory */;
    });
    if (needsCopy)
        proFile = copyToAlternativeLocation(proFile, filesToOpen, item->dependencies);

    // don't try to load help and files if loading the help request is being cancelled
    if (proFile.isEmpty())
        return;
    ProjectExplorerPlugin::OpenProjectResult result = ProjectExplorerPlugin::openProject(proFile);
    if (result) {
        ICore::openFiles(filesToOpen);
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
        QUrl docUrl = QUrl::fromUserInput(item->docUrl);
        if (docUrl.isValid())
            HelpManager::showHelpUrl(docUrl, HelpManager::ExternalHelpAlways);
        ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    } else {
        ProjectExplorerPlugin::showOpenProjectError(result);
    }
}

class ExampleDelegate : public ListItemDelegate
{
public:

    void setShowExamples(bool showExamples) { m_showExamples = showExamples; goon(); }

protected:
    void clickAction(const ListItem *item) const override
    {
        QTC_ASSERT(item, return);
        const auto exampleItem = static_cast<const ExampleItem *>(item);

        if (exampleItem->isVideo)
            QDesktopServices::openUrl(QUrl::fromUserInput(exampleItem->videoUrl));
        else if (exampleItem->hasSourceCode)
            ExamplesWelcomePage::openProject(exampleItem);
        else
            HelpManager::showHelpUrl(QUrl::fromUserInput(exampleItem->docUrl),
                                     HelpManager::ExternalHelpAlways);
    }

    void drawPixmapOverlay(const ListItem *item, QPainter *painter,
                           const QStyleOptionViewItem &option,
                           const QRect &currentPixmapRect) const override
    {
        QTC_ASSERT(item, return);
        const auto exampleItem = static_cast<const ExampleItem *>(item);
        if (exampleItem->isVideo) {
            painter->save();
            painter->setFont(option.font);
            painter->setCompositionMode(QPainter::CompositionMode_Difference);
            painter->setPen(Qt::white);
            painter->drawText(currentPixmapRect.translated(0, -WelcomePageHelpers::ItemGap),
                              exampleItem->videoLength, Qt::AlignBottom | Qt::AlignHCenter);
            painter->restore();
            static const QPixmap playOverlay =
                    StyleHelper::dpiSpecificImageFile(":/qtsupport/images/icons/playoverlay.png");
            const QSize playOverlaySize =  playOverlay.size() / playOverlay.devicePixelRatio();
            const QPoint playOverlayPos =
                    QPoint((currentPixmapRect.width() - playOverlaySize.width()) / 2,
                           (currentPixmapRect.height() - playOverlaySize.height()) / 2)
                    + currentPixmapRect.topLeft();
            painter->drawPixmap(playOverlayPos, playOverlay);
        }
    }

    bool m_showExamples = true;
};

class ExamplesPageWidget : public QWidget
{
public:
    ExamplesPageWidget(bool isExamples)
        : m_isExamples(isExamples)
    {
        m_exampleDelegate.setShowExamples(isExamples);

        auto searchBox = new SearchBox(this);
        m_searcher = searchBox->m_lineEdit;

        auto grid = new QGridLayout(this);
        grid->setContentsMargins(0, 0, 0, WelcomePageHelpers::ItemGap);
        grid->setHorizontalSpacing(0);
        grid->setVerticalSpacing(WelcomePageHelpers::ItemGap);

        auto searchBar = WelcomePageHelpers::panelBar(this);
        auto hbox = new QHBoxLayout(searchBar);
        hbox->setContentsMargins(0, 0, 0, 0);
        if (m_isExamples) {
            m_searcher->setPlaceholderText(Tr::tr("Search in Examples..."));

            auto exampleSetSelector = new QComboBox(this);
            QPalette pal = exampleSetSelector->palette();
            // for macOS dark mode
            pal.setColor(QPalette::Text, Utils::creatorTheme()->color(Theme::Welcome_TextColor));
            exampleSetSelector->setPalette(pal);
            exampleSetSelector->setMinimumWidth(Core::WelcomePageHelpers::GridItemWidth);
            exampleSetSelector->setMaximumWidth(Core::WelcomePageHelpers::GridItemWidth);
            exampleSetSelector->setModel(s_exampleSetModel);
            exampleSetSelector->setCurrentIndex(s_exampleSetModel->selectedExampleSet());
            connect(exampleSetSelector,
                    &QComboBox::activated,
                    s_exampleSetModel,
                    &ExampleSetModel::selectExampleSet);
            connect(s_exampleSetModel,
                    &ExampleSetModel::selectedExampleSetChanged,
                    exampleSetSelector,
                    &QComboBox::setCurrentIndex);

            hbox->setSpacing(Core::WelcomePageHelpers::HSpacing);
            hbox->addWidget(exampleSetSelector);
        } else {
            m_searcher->setPlaceholderText(Tr::tr("Search in Tutorials..."));
        }
        hbox->addWidget(searchBox);
        grid->addWidget(WelcomePageHelpers::panelBar(this), 0, 0);
        grid->addWidget(searchBar, 0, 1);
        grid->addWidget(WelcomePageHelpers::panelBar(this), 0, 2);

        auto gridView = new SectionedGridView(this);
        m_viewController
            = new ExamplesViewController(s_exampleSetModel, gridView, m_searcher, isExamples, this);

        gridView->setItemDelegate(&m_exampleDelegate);
        grid->addWidget(gridView, 1, 1, 1, 2);

        connect(&m_exampleDelegate, &ExampleDelegate::tagClicked,
                this, &ExamplesPageWidget::onTagClicked);
    }

    void onTagClicked(const QString &tag)
    {
        const QString text = m_searcher->text();
        m_searcher->setText((text.startsWith("tag:\"") ? text.trimmed() + " " : QString())
                            + QString("tag:\"%1\" ").arg(tag));
    }

    void showEvent(QShowEvent *event) override
    {
        m_viewController->setVisible(true);
        QWidget::showEvent(event);
    }

    void hideEvent(QHideEvent *event) override
    {
        m_viewController->setVisible(false);
        QWidget::hideEvent(event);
    }

    const bool m_isExamples;
    ExampleDelegate m_exampleDelegate;
    QLineEdit *m_searcher;
    ExamplesViewController *m_viewController = nullptr;
};

QWidget *ExamplesWelcomePage::createWidget() const
{
    return new ExamplesPageWidget(m_showExamples);
}

} // namespace Internal
} // namespace QtSupport
