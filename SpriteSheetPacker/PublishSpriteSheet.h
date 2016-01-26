#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>
#include <QtConcurrent>
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

enum ImageFormat {
    kPNG = 0,
    kPKM,
    kPVR,
    kPVR_CCZ
};

enum PixelFormat {
    kRGB888 = 0,
    kRGBA8888,
    kETC1,
    kPVRTC2,
    kPVRTC2A,
    kPVRTC4,
    kPVRTC4A
};


class PublishSpriteSheet: public QObject {
    Q_OBJECT

public:
    PublishSpriteSheet();

    void addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName);
    void setImageFormat(const ImageFormat& imageFormat) { _imageFormat = imageFormat; }
    void setPixelFormat(const PixelFormat& pixelFormat) { _pixelFormat = pixelFormat; }

    bool publish(const QString& format, const QString& optMode, int optLevel, bool errorMessage = true);
    bool generateDataFile(const QString& filePath, const QString& format, const SpriteAtlas &atlas, bool errorMessage = true);

    bool optimizePNG(const QString& fileName, const QString& optMode, int optLevel);
    void optimizePNGInThread(QStringList fileNames, const QString& optMode, int optLevel);

    static void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    static QMap<QString, QString>& formats() { return _formats; }

signals:
    void onCompleted();

protected:
    QFutureWatcher<bool> _watcher;
    QMutex _mutex;

    QList<SpriteAtlas> _spriteAtlases;
    QStringList _fileNames;

    ImageFormat _imageFormat;
    PixelFormat _pixelFormat;

    static QMap<QString, QString> _formats;
};

#endif // PUBLISHSPRITESHEET_H
