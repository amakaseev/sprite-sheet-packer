#include "PublishSpriteSheet.h"
#include "SpritePackerProjectFile.h"
#include "SpriteAtlas.h"
#include "PListSerializer.h"
#include <QMessageBox>
#include "PngOptimizer.h"

QMap<QString, QString> PublishSpriteSheet::_formats;

QJSValue jsValue(QJSEngine& engine, const QRect& rect) {
    QJSValue value = engine.newObject();
    value.setProperty("x", rect.left());
    value.setProperty("y", rect.top());
    value.setProperty("width", rect.width());
    value.setProperty("height", rect.height());
    return value;
}

QJSValue jsValue(QJSEngine& engine, const QSize& size) {
    QJSValue value = engine.newObject();
    value.setProperty("width", size.width());
    value.setProperty("height", size.height());
    return value;
}

QJSValue jsValue(QJSEngine& engine, const QPoint& point) {
    QJSValue value = engine.newObject();
    value.setProperty("x", point.x());
    value.setProperty("y", point.y());
    return value;
}

void JSConsole::log(QString msg) {
    qDebug() << "js:"<< msg;
}

void PublishSpriteSheet::addSpriteSheet(const SpriteAtlas &atlas, const QString &fileName) {
    _spriteAtlases.append(atlas);
    _fileNames.append(fileName);
}

bool PublishSpriteSheet::publish(const QString& format, int optLevel, bool errorMessage) {

    if (_spriteAtlases.size() != _fileNames.size()) {
        return false;
    }

    for (int i = 0; i < _spriteAtlases.size(); i++) {
        const SpriteAtlas& atlas = _spriteAtlases.at(i);
        const QString& filePath = _fileNames.at(i);

        // generate the data file and the image
        if (!generateDataFile(filePath, format, atlas, errorMessage)) {
            return false;
        }
        atlas.image().save(filePath + ".png");
    }

    if (optLevel > 0) {
        optimizePNGInThread(_fileNames, optLevel);
    }

    _spriteAtlases.clear();
    _fileNames.clear();

    return true;
}

bool PublishSpriteSheet::generateDataFile(const QString& filePath, const QString& format, const SpriteAtlas &spriteAtlas, bool errorMessage) {
    QJSEngine engine;

    auto it_format = _formats.find(format);
    if (it_format == _formats.end()) {
        QString errorString = QString("Not found script file for [%1] format").arg(format);
        qDebug() << errorString;
        if (errorMessage) QMessageBox::critical(NULL, "Export script error", errorString);
        return false;
    }

    QString scriptFileName = it_format.value();
    QFile scriptFile(scriptFileName);
    if (!scriptFile.open(QIODevice::ReadOnly)) {
        qDebug() << "File [" << scriptFileName << "] not found!";
        return false;
    }

    QTextStream stream(&scriptFile);
    QString contents = stream.readAll();
    scriptFile.close();

    // add console object
    JSConsole console;
    QJSValue consoleObj = engine.newQObject(&console);
    engine.globalObject().setProperty("console", consoleObj);

    // evaluate export plugin script
    qDebug() << "Run script...";
    QJSValue result = engine.evaluate(contents, scriptFileName);
    if (result.isError()) {
        QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
        qDebug() << errorString;
        if (errorMessage) QMessageBox::critical(NULL, "Export script error", errorString);
        return false;
    }

    if (engine.globalObject().hasOwnProperty("exportSpriteSheet")) {
        QJSValueList args;
        args << QJSValue(filePath);
        args << QJSValue(filePath + ".png");

        // collect sprite frames
        QJSValue spriteFramesValue = engine.newObject();
        auto it_f = spriteAtlas.spriteFrames().cbegin();
        for (; it_f != spriteAtlas.spriteFrames().cend(); ++it_f) {
            QJSValue spriteFrameValue = engine.newObject();
            spriteFrameValue.setProperty("frame", jsValue(engine, it_f.value().frame));
            spriteFrameValue.setProperty("offset", jsValue(engine, it_f.value().offset));
            spriteFrameValue.setProperty("rotated", it_f.value().rotated);
            spriteFrameValue.setProperty("sourceColorRect", jsValue(engine, it_f.value().sourceColorRect));
            spriteFrameValue.setProperty("sourceSize", jsValue(engine, it_f.value().sourceSize));

            spriteFramesValue.setProperty(it_f.key(), spriteFrameValue);
        }
        args << QJSValue(spriteFramesValue);

        // run export
        QJSValue exportSpriteSheet = engine.globalObject().property("exportSpriteSheet");
        result = exportSpriteSheet.call(args);

        if (result.isError()) {
            QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
            qDebug() << errorString;
            if (errorMessage) QMessageBox::critical(NULL, "Export script error", errorString);
            return false;
        } else {
            // write data
            if (!result.hasProperty("data") || !result.hasProperty("format")) {
                QString errorString = "Script function must be return object: {data:data, format:'plist|json|other'}";
                qDebug() << errorString;
                if (errorMessage) QMessageBox::critical(NULL, "Export script error", errorString);
                return false;
            } else {
                QJSValue data = result.property("data");
                QString format = result.property("format").toString();
                QFile file(filePath + "." + format);
                file.open(QIODevice::WriteOnly | QIODevice::Text);
                QTextStream out(&file);
                if (format == "plist") {
                    out << PListSerializer::toPList(data.toVariant());
                } else {
                    out << data.toString();
                }
            }
        }

    } else {
        qDebug() << "Not found global exportSpriteSheet function!";
        if (errorMessage) QMessageBox::critical(NULL, "Export script error", "Not found global exportSpriteSheet function!");
        return false;
    }

    return true;
}

bool PublishSpriteSheet::optimizePNG(const QString& fileName, int optLevel) {
    OptiPngOptimizer optimizer(optLevel);

    QMutexLocker loker(&_mutex);
    bool result = optimizer.optimizeFile(fileName + ".png");

    return result;
}

void PublishSpriteSheet::optimizePNGInThread(QStringList fileNames, int optLevel) {
    QObject::connect(&_watcher, SIGNAL(finished()), this, SIGNAL(onCompleted()));
    QFuture<bool> resultFuture;

    for (const QString& fileName : fileNames) {
        resultFuture = QtConcurrent::run(this, &PublishSpriteSheet::optimizePNG, fileName, optLevel);
    }

    _watcher.setFuture(resultFuture);
}
