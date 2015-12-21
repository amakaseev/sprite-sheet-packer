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
    explicit JSWriter(const QImage& image): _image(image) {
        qDebug() << "JSWriter::JSWriter";
    }

public slots:
    void writeData(const QString& fileName, const QJSValue& data, const QString& format);
    void writeImage(const QString& fileName);

private:
    const QImage& _image;
};

bool PublishSpriteSheet(const QString& destPath, const QString& spriteSheetName, const ScalingVariant& scalingVariant, const SpriteAtlas& spriteAtlas);

#endif // PUBLISHSPRITESHEET_H
