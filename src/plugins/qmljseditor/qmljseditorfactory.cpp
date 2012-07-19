/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmljseditorfactory.h"
#include "qmljseditoreditable.h"
#include "qmljseditor.h"
#include "qmljseditoractionhandler.h"
#include "qmljseditorconstants.h"
#include "qmljseditorplugin.h"

#include <qmljstools/qmljstoolsconstants.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QPushButton>
#include <QMainWindow>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;

QmlJSEditorFactory::QmlJSEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    m_mimeTypes
            << QLatin1String(QmlJSTools::Constants::QML_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::JS_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::JSON_MIMETYPE)
            ;
}

Core::Id QmlJSEditorFactory::id() const
{
    return C_QMLJSEDITOR_ID;
}

QString QmlJSEditorFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", C_QMLJSEDITOR_DISPLAY_NAME);
}

Core::IEditor *QmlJSEditorFactory::createEditor(QWidget *parent)
{
    QmlJSEditor::QmlJSTextEditorWidget *rc = new QmlJSEditor::QmlJSTextEditorWidget(parent);
    QmlJSEditorPlugin::instance()->initializeEditor(rc);
    return rc->editor();
}

QStringList QmlJSEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
