#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>

class SpriteAtlas;
struct ScalingVariant;

class JSConsole : public QObject
{
    Q_OBJECT
public:
    explicit JSConsole() { }

public slots:
    void log(QString msg);
};

class JSWriter: public QObject {
    Q_OBJECT
public:
    explicit JSWriter(const QImage& image): _image(image) { }

public slots:
    void writeData(const QString& fileName, const QJSValue& data, const QString& format);
    void writeImage(const QString& fileName);

private:
    const QImage& _image;
};

class PublishSpriteSheet {
public:
    static bool publish(const QString& filePath, const QString& format, const SpriteAtlas& spriteAtlas);

    static void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    static QMap<QString, QString>& formats() { return _formats; }

private:
    static QMap<QString, QString> _formats;
};

#endif // PUBLISHSPRITESHEET_H
