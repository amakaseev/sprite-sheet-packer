#include "PublishSpriteSheet.h"
#include "SpritePackerProjectFile.h"
#include "SpriteAtlas.h"
#include "PListSerializer.h"
#include <QMessageBox>

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

void JSWriter::writeData(const QString& fileName, const QJSValue& data, const QString& format) {
    qDebug() << "writeData" << fileName << "format:" << format;
    if (format == "PLIST") {
        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << PListSerializer::toPList(data.toVariant());
    } else if (format == "JSON") {
        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(QJsonDocument::fromVariant(data.toVariant()).toJson());
    } else if (format == "TEXT") {
        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << data.toString();
    }

}

void JSWriter::writeImage(const QString& fileName) {
    qDebug() << "writeImage" << fileName;
    _image.save(fileName);
}

bool PublishSpriteSheet::publish(const QString& destPath, const QString& spriteSheetName, const QString& format, const ScalingVariant& scalingVariant, const SpriteAtlas& spriteAtlas) {
    QJSEngine engine;

    auto it_format = _formats.find(format);
    if (it_format == _formats.end()) {
        QString errorString = QString("Not found script file for [%1] format").arg(format);
        qDebug() << errorString;
        QMessageBox::critical(NULL, "Export script error", errorString);
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

    // add writer object
    JSWriter writer(spriteAtlas.image());
    QJSValue writerObj = engine.newQObject(&writer);
    engine.globalObject().setProperty("writer", writerObj);

    // evaluate export plugin script
    QJSValue result = engine.evaluate(contents, scriptFileName);
    if (result.isError()) {
        QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
        qDebug() << errorString;
        QMessageBox::critical(NULL, "Export script error", errorString);
        return false;
    }

    if (engine.globalObject().hasOwnProperty("exportSpriteSheet")) {
        QJSValueList args;
        args << QJSValue(destPath);
        args << QJSValue(spriteSheetName);

        QJSValue scalingVariantValue = engine.newObject();
        scalingVariantValue.setProperty("folderName", scalingVariant.folderName);
        scalingVariantValue.setProperty("scale", scalingVariant.scale);
        args << QJSValue(scalingVariantValue);

        // collect sprite frames
        QJSValue spriteFramesValue = engine.newObject();
        auto it_f = spriteAtlas.spriteFrames().cbegin();
        for (; it_f != spriteAtlas.spriteFrames().cend(); ++it_f) {
            QJSValue spriteFrameValue = engine.newObject();
            spriteFrameValue.setProperty("frame", jsValue(engine, it_f.value().mFrame));
            spriteFrameValue.setProperty("offset", jsValue(engine, it_f.value().mOffset));
            spriteFrameValue.setProperty("rotated", it_f.value().mRotated);
            spriteFrameValue.setProperty("sourceColorRect", jsValue(engine, it_f.value().mSourceColorRect));
            spriteFrameValue.setProperty("sourceSize", jsValue(engine, it_f.value().mSourceSize));

            spriteFramesValue.setProperty(it_f.key(), spriteFrameValue);
        }
        args << QJSValue(spriteFramesValue);

        // run export
        QJSValue exportSpriteSheet = engine.globalObject().property("exportSpriteSheet");
        result = exportSpriteSheet.call(args);

        if (result.isError()) {
            QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
            qDebug() << errorString;
            QMessageBox::critical(NULL, "Export script error", errorString);
            return false;
        }
    } else {
        qDebug() << "Not found global exportSpriteSheet function!";
        QMessageBox::critical(NULL, "Export script error", "Not found global exportSpriteSheet function!");
        return false;
    }

    return true;
}
