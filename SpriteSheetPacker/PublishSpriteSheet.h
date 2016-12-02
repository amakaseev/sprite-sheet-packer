#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>
#include <QtConcurrent>
#include "ImageFormat.h"
#include "PngOptimizer.h"
#include "SpriteAtlas.h"

struct ScalingVariant;

class JSConsole : public QObject {
    Q_OBJECT
public:
    explicit JSConsole() { }

public slots:
    void log(QString msg);
};


class PublishSpriteSheet: public QObject {
    Q_OBJECT

public:
    PublishSpriteSheet();

    void addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName);
    void setImageFormat(ImageFormat imageFormat) { _imageFormat = imageFormat; }
    void setPixelFormat(PixelFormat pixelFormat) { _pixelFormat = pixelFormat; }
    void setPremultiplied(bool premultiplied) { _premultiplied = premultiplied; }
    void setPngQuality(const QString& optMode, int optLevel) { _pngQuality.optMode = optMode; _pngQuality.optLevel = optLevel; }
    void setJpgQuality(int quality) { _jpgQuality = quality; }

    bool publish(const QString& format, bool errorMessage = true);
    bool generateDataFile(const QString& filePath, const QString& format, const SpriteAtlas &atlas, bool errorMessage = true);

    static void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    static QMap<QString, QString>& formats() { return _formats; }

signals:
    void onCompletedOptimizePNG();

protected:
    bool optimizePNG(const QString& fileName, const QString& optMode, int optLevel);
    void optimizePNGInThread(QStringList fileNames, const QString& optMode, int optLevel);

protected:
    QFutureWatcher<bool> _watcher;
    QMutex _mutex;

    QList<SpriteAtlas> _spriteAtlases;
    QStringList _fileNames;

    ImageFormat _imageFormat;
    PixelFormat _pixelFormat;
    bool        _premultiplied;

    struct {
        QString optMode;
        int     optLevel;
    } _pngQuality;

    int         _jpgQuality;

    static QMap<QString, QString> _formats;
};

#endif // PUBLISHSPRITESHEET_H
