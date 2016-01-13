#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>
#include <QtConcurrent>
//#include <QtAtom
#include "PngOptimizer.h"

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

class PublishSpriteSheet : public QObject {
    Q_OBJECT

public:
    bool publish(const QString& filePath, const QString& format, int optLevel, const SpriteAtlas& spriteAtlas, bool errorMessage = true);
    bool optimizePNG(const QString& fileName, int optLevel);
    void optimizePNGInThread(QList<QString> fileNames, int optLevel);

    static void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    static QMap<QString, QString>& formats() { return _formats; }

signals:
    void onCompleted();

private:
    static QMap<QString, QString> _formats;
    QFutureWatcher<void> watcher;
    QMutex _mutex;
};

#endif // PUBLISHSPRITESHEET_H
