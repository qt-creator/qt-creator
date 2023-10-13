// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "screenrecorderconstants.h"

#include "cropandtrim.h"
#include "export.h"
#include "record.h"
#include "screenrecorderconstants.h"
#include "screenrecordersettings.h"
#include "screenrecordertr.h"

#ifdef WITH_TESTS
#include "screenrecorder_test.h"
#endif // WITH_TESTS

#include <extensionsystem/iplugin.h>

#include <utils/layoutbuilder.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/temporaryfile.h>
#include <utils/utilsicons.h>

#include <solutions/spinner/spinner.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <QAction>
#include <QDialog>
#include <QPushButton>

using namespace Utils;
using namespace Core;

namespace ScreenRecorder::Internal {

class ScreenRecorderDialog : public QDialog
{
public:
    ScreenRecorderDialog(QWidget *parent = nullptr)
        : QDialog(parent)
        , m_recordFile("XXXXXX" + RecordWidget::recordFileExtension())
    {
        setWindowTitle(Tr::tr("Record Screen"));
        StyleHelper::setPanelWidget(this);

        m_recordFile.open();
        m_recordWidget = new RecordWidget(FilePath::fromString(m_recordFile.fileName()));

        m_cropAndTrimStatusWidget = new CropAndTrimWidget;

        m_exportWidget = new ExportWidget;

        using namespace Layouting;
        Column {
            m_recordWidget,
            Row { m_cropAndTrimStatusWidget, m_exportWidget },
            noMargin(), spacing(0),
        }.attachTo(this);

        auto setLowerRowEndabled = [this] (bool enabled) {
            m_cropAndTrimStatusWidget->setEnabled(enabled);
            m_exportWidget->setEnabled(enabled);
        };
        setLowerRowEndabled(false);
        connect(m_recordWidget, &RecordWidget::started,
                this, [setLowerRowEndabled] { setLowerRowEndabled(false); });
        connect(m_recordWidget, &RecordWidget::finished,
                this, [this, setLowerRowEndabled] (const ClipInfo &clip) {
            m_cropAndTrimStatusWidget->setClip(clip);
            m_exportWidget->setClip(clip);
            setLowerRowEndabled(true);
        });
        connect(m_cropAndTrimStatusWidget, &CropAndTrimWidget::cropRectChanged,
                m_exportWidget, &ExportWidget::setCropRect);
        connect(m_cropAndTrimStatusWidget, &CropAndTrimWidget::trimRangeChanged,
                m_exportWidget, &ExportWidget::setTrimRange);
        connect(m_exportWidget, &ExportWidget::started, this, [this] {
            setEnabled(false);
            m_spinner->show();
        });
        connect(m_exportWidget, &ExportWidget::finished, this, [this] {
            setEnabled(true);
            m_spinner->hide();
        });

        m_spinner = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Medium, this);
        m_spinner->hide();

        layout()->setSizeConstraint(QLayout::SetFixedSize);
    }

    static void showDialog()
    {
        static QPointer<QDialog> staticInstance;

        if (staticInstance.isNull()) {
            staticInstance = new ScreenRecorderDialog(Core::ICore::dialogParent());
            staticInstance->setAttribute(Qt::WA_DeleteOnClose);
        }
        staticInstance->show();
        staticInstance->raise();
        staticInstance->activateWindow();
    }

private:
    RecordWidget *m_recordWidget;
    TemporaryFile m_recordFile;
    CropAndTrimWidget *m_cropAndTrimStatusWidget;
    ExportWidget *m_exportWidget;
    SpinnerSolution::Spinner *m_spinner;
};

class ScreenRecorderPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ScreenRecorder.json")

public:
    void initialize() final
    {
        const QIcon menuIcon = Icon({{":/utils/images/filledcircle.png", Theme::IconsStopColor}},
                                    Icon::MenuTintedStyle).icon();
        auto action = new QAction(menuIcon, Tr::tr("Record Screen..."), this);
        Command *cmd = ActionManager::registerAction(action, Constants::ACTION_ID,
                                                     Context(Core::Constants::C_GLOBAL));
        connect(action, &QAction::triggered, this, &ScreenRecorderPlugin::showDialogOrSettings);
        ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
        mtools->addAction(cmd);

#ifdef WITH_TESTS
        addTest<FFmpegOutputParserTest>();
#endif
    }

private:
    void showDialogOrSettings()
    {
        if (!Internal::settings().toolsRegistered() &&
            !Core::ICore::showOptionsDialog(Constants::TOOLSSETTINGSPAGE_ID)) {
            return;
        }

        ScreenRecorderDialog::showDialog();
    }
};

} // namespace ScreenRecorder::Internal

#include "screenrecorderplugin.moc"
