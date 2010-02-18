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

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/ifile.h>

#include <QtCore/QStringList>
#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QScrollArea>

namespace BINEditor {
namespace Internal {

class ImageViewerFactory : public Core::IEditorFactory
{
Q_OBJECT
public:
    explicit ImageViewerFactory(QObject *parent = 0);

    Core::IEditor *createEditor(QWidget *parent);

    QStringList mimeTypes() const;

    QString id() const;
    QString displayName() const;

    Core::IFile *open(const QString &fileName);

private:
    QStringList m_mimeTypes;
};

class ImageViewer;

class ImageViewerFile : public Core::IFile
{
    Q_OBJECT
public:
    explicit ImageViewerFile(ImageViewer *parent = 0);

    bool save(const QString &fileName = QString()) { Q_UNUSED(fileName); return false; }
    QString fileName() const { return m_fileName; }

    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }
    QString mimeType() const { return m_mimeType; }

    bool isModified() const { return false; }
    bool isReadOnly() const { return true; }
    bool isSaveAsAllowed() const { return false; }

    void modified(ReloadBehavior *behavior);

    void checkPermissions() {}

    void setMimetype(const QString &mimetype) { m_mimeType = mimetype; emit changed(); }
    void setFileName(const QString &filename) { m_fileName = filename; emit changed(); }
private:
    QString m_fileName;
    QString m_mimeType;
    ImageViewer *m_editor;
};

class ImageViewer : public Core::IEditor
{
    Q_OBJECT
public:
    explicit ImageViewer(QObject *parent = 0);
    ~ImageViewer();

    QList<int> context() const;
    QWidget *widget();

    bool createNew(const QString &contents = QString());
    bool open(const QString &fileName = QString());
    Core::IFile *file();
    QString id() const;
    QString displayName() const;
    void setDisplayName(const QString &title);

    bool duplicateSupported() const { return false; }
    IEditor *duplicate(QWidget * /* parent */) { return 0; }

    QByteArray saveState() const { return QByteArray(); }
    bool restoreState(const QByteArray & /* state */) { return true; }

    int currentLine() const { return 0; }
    int currentColumn() const { return 0; }

    bool isTemporary() const { return false; }

    QWidget *toolBar() { return 0; }

private:
    QList<int> m_context;
    QString m_displayName;
    ImageViewerFile *m_file;
    QScrollArea *m_scrollArea;
    QWidget *m_imageview;
    QLabel *m_label;
};

}
}

#endif // IMAGEVIEWER_H
