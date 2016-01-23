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

QJSValue jsValue(QJSEngine& engine, const Triangles& triangles) {
    QJSValue value = engine.newObject();

    int index = 0;
    QJSValue verts = engine.newArray(triangles.verts.size());
    for (auto vert: triangles.verts) {
        verts.setProperty(index, jsValue(engine, vert));
        ++index;
    }
    value.setProperty("verts", verts);

    index = 0;
    QJSValue indices = engine.newArray(triangles.indices.size());
    for (auto idx: triangles.indices) {
        indices.setProperty(index, idx);
        ++index;
    }
    value.setProperty("indices", indices);
    return value;
}

void JSConsole::log(QString msg) {
    qDebug() << "js:"<< msg;
}

void PublishSpriteSheet::addSpriteSheet(const SpriteAtlas &atlas, const QString &fileName) {
    _spriteAtlases.append(atlas);
    _fileNames.append(fileName);
}

bool PublishSpriteSheet::publish(const QString& format, const QString& optMode, int optLevel, bool errorMessage, PublishStatusDialog* dialog) {

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
        if (dialog) dialog->progressBar()->setValue(1);

        // save png image
        qDebug() << "Save image:" << filePath + ".png";
        atlas.image().save(filePath + ".png");
        if (dialog) dialog->progressBar()->setValue(2);
    }

    if (optMode != "None") {
        qDebug() << "Begin optimize image...";
        // we use values 1-7 so that it is more user friendly, because 0 also means optimization.
        optimizePNGInThread(_fileNames, optMode, optLevel - 1, dialog);
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
            spriteFrameValue.setProperty("triangles", jsValue(engine, it_f.value().triangles));

            spriteFramesValue.setProperty(it_f.key(), spriteFrameValue);
        }
        args << QJSValue(spriteFramesValue);

        args << jsValue(engine, spriteAtlas.image().size());

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

bool PublishSpriteSheet::optimizePNG(const QString& fileName, const QString& optMode, int optLevel) {
    bool result = false;

    if (optMode == "Lossless") {
        OptiPngOptimizer optimizer(optLevel);

        _mutex.lock();
        result = optimizer.optimizeFile(fileName + ".png");
        _mutex.unlock();
    } else if (optMode == "Lossy") {
        PngQuantOptimizer optimizer(optLevel);

        _mutex.lock();
        result = optimizer.optimizeFile(fileName + ".png");
        _mutex.unlock();
    }

    return result;
}

void PublishSpriteSheet::optimizePNGInThread(QStringList fileNames, const QString& optMode, int optLevel, PublishStatusDialog* dialog) {
    QObject::connect(&_watcher, SIGNAL(finished()), this, SIGNAL(onCompleted()));

    QFuture<bool> resultFuture;

    for (const QString& fileName : fileNames) {
        resultFuture = QtConcurrent::run(this, &PublishSpriteSheet::optimizePNG, fileName, optMode, optLevel);
    }

    _watcher.setFuture(resultFuture);
}
