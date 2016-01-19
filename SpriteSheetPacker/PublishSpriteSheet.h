#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>
#include <QtConcurrent>
#include "PngOptimizer.h"
#include "SpriteAtlas.h"
#include "PublishStatusDialog.h"

struct ScalingVariant;

class JSConsole : public QObject {
    Q_OBJECT
public:
    explicit JSConsole() { }

public slots:
    void log(QString msg);
};

class PublishSpriteSheet : public QObject {
    Q_OBJECT

public:
    void addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName);

    bool publish(const QString& format, const QString& optMode, int optLevel, bool errorMessage = true, PublishStatusDialog* dialog = nullptr);
    bool generateDataFile(const QString& filePath, const QString& format, const SpriteAtlas &atlas, bool errorMessage = true);

    bool optimizePNG(const QString& fileName, const QString& optMode, int optLevel);
    void optimizePNGInThread(QStringList fileNames, const QString& optMode, int optLevel, PublishStatusDialog* dialog = nullptr);

    static void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    static QMap<QString, QString>& formats() { return _formats; }

signals:
    void onCompleted();

private:
    static QMap<QString, QString> _formats;

    QFutureWatcher<bool> _watcher;
    QMutex _mutex;

    QList<SpriteAtlas> _spriteAtlases;
    QStringList _fileNames;

};

#endif // PUBLISHSPRITESHEET_H
