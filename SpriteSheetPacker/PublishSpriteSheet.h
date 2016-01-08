#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QtCore>
#include <QJSEngine>

class SpriteAtlas;
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
    bool publish(const QString& filePath, const QString& format, int optLevel, const SpriteAtlas& spriteAtlas, bool errorMessage = true);
    bool optimizePNG(const QString& fileName, int optLevel);
    void optimizePNGInThread(const QString& fileName, int optLevel);

    void addFormat(const QString& format, const QString& scriptFileName) { _formats[format] = scriptFileName; }
    QMap<QString, QString>& formats() { return _formats; }

    static PublishSpriteSheet* instance() {
        if (_instance == nullptr) {
            _instance = new PublishSpriteSheet();
        }
        return _instance;
    }

private slots:
    void on_completed();

private:
    static QMap<QString, QString> _formats;
    static PublishSpriteSheet* _instance;

    bool _customComplete;

signals:
    void log(const QString& log, const QColor& color);
    void completed();
};

class OptimizationWorker : public QObject {
    Q_OBJECT

public:
    OptimizationWorker(PublishSpriteSheet* parent, const QString& fileName, int optLevel);

    static void app_printf(const char *fmt, ...);
    static void app_print_cntrl(int cntrl_code);
    static void app_progress(unsigned long current_step, unsigned long total_steps);
    static void panic(const char *msg);

private:
    QString _fileName;
    int _optLevel;
    PublishSpriteSheet* _parent;

signals:
    void finished();

public slots:
    void doWork();
};


#endif // PUBLISHSPRITESHEET_H
