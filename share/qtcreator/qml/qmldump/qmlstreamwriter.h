#ifndef QMLSTREAMWRITER_H
#define QMLSTREAMWRITER_H

#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QScopedPointer>
#include <QtCore/QPair>

class QmlStreamWriter
{
public:
    QmlStreamWriter(QByteArray *array);

    void writeStartDocument();
    void writeEndDocument();
    void writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as = QString());
    //void writeFilesystemImport(const QString &file, const QString &as = QString());
    void writeStartObject(const QString &component);
    void writeEndObject();
    void writeScriptBinding(const QString &name, const QString &rhs);
    void writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue);
    void writeArrayBinding(const QString &name, const QStringList &elements);
    void write(const QString &data);

private:
    void writeIndent();
    void writePotentialLine(const QByteArray &line);
    void flushPotentialLinesWithNewlines();

    int m_indentDepth;
    QList<QByteArray> m_pendingLines;
    int m_pendingLineLength;
    bool m_maybeOneline;
    QScopedPointer<QIODevice> m_stream;
};

#endif // QMLSTREAMWRITER_H
