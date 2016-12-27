#include "PublishSpriteSheet.h"
#include "SpritePackerProjectFile.h"
#include "SpriteAtlas.h"
#include "PListSerializer.h"
#include <QMessageBox>
#include "PngOptimizer.h"
#include "PVRTexture.h"
#include "PVRTextureUtilities.h"

using namespace pvrtexture;

QMap<QString, QString> PublishSpriteSheet::_formats;

QString imagePrefix(ImageFormat imageFormat) {
    switch (imageFormat) {
        case kPNG: return ".png";
        case kJPG: return ".jpg";
        case kPKM: return ".pvr";
        case kPVR: return ".pvr";
        case kPVR_CCZ: return ".pvr.ccz";
        default: return ".png";
    }
}

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


PublishSpriteSheet::PublishSpriteSheet() {
    _imageFormat = kPNG;
    _pixelFormat = kARGB8888;
    _premultiplied = true;
    _jpgQuality = 80;

    _trimSpriteNames = true;
    _prependSmartFolderName = true;
}

void PublishSpriteSheet::addSpriteSheet(const SpriteAtlas &atlas, const QString &fileName) {
    _spriteAtlases.append(atlas);
    _fileNames.append(fileName);
}

bool PublishSpriteSheet::publish(const QString& format, bool errorMessage) {

    if (_spriteAtlases.size() != _fileNames.size()) {
        return false;
    }

    QStringList outputFilePaths;
    for (int i = 0; i < _spriteAtlases.size(); i++) {
        const SpriteAtlas& atlas = _spriteAtlases.at(i);
        const QString& filePath = _fileNames.at(i);

        for (int n=0; n<atlas.outputData().size(); ++n) {
            const auto& outputData = atlas.outputData().at(n);

            QString outputFilePath = filePath;
            if (outputFilePath.contains("{n}")) {
                outputFilePath.replace("{n}", QString::number(n));
            } else if (outputFilePath.contains("{n1}")) {
                outputFilePath.replace("{n1}", QString::number(n + 1));
            } else if (atlas.outputData().size() > 1) {
                outputFilePath = outputFilePath + "_" + QString::number(n);
            }

            // save this name for optimize png
            outputFilePaths.push_back(outputFilePath);

            // generate the data file and the image
            if (!generateDataFile(outputFilePath, format, outputData._spriteFrames, outputData._atlasImage, errorMessage)) {
                return false;
            }

            // save image
            qDebug() << "Save image:" << outputFilePath + imagePrefix(_imageFormat);
            if ((_imageFormat == kPNG) || (_imageFormat == kJPG) || (_imageFormat == kJPG_PNG)) {
                QImage image = convertImage(outputData._atlasImage, _pixelFormat, _premultiplied);
                if (_imageFormat == kPNG) {
                    QImageWriter writer(outputFilePath + imagePrefix(kPNG), "png");
                    writer.setOptimizedWrite(true);
                    writer.setCompression(100);
                    writer.setQuality(0);
                    writer.write(image);
                } else if ((_imageFormat == kJPG) || (_imageFormat == kJPG_PNG)) {
                    QImageWriter writer(outputFilePath + imagePrefix(kJPG), "jpg");
                    writer.setOptimizedWrite(true);
                    writer.setCompression(100);
                    writer.setQuality(_jpgQuality);
                    writer.write(image);

                    if (_imageFormat == kJPG_PNG) {
                        QImage maskImage = convertImage(outputData._atlasImage, kALPHA, _premultiplied);
                        QImageWriter writer(outputFilePath + imagePrefix(kPNG), "png");
                        writer.setOptimizedWrite(true);
                        writer.setCompression(100);
                        writer.setQuality(0);
                        writer.write(maskImage);
                    }
                }
            } else if ((_imageFormat == kPKM) || (_imageFormat == kPVR) || (_imageFormat == kPVR_CCZ)) {
                CPVRTextureHeader pvrHeader(PVRStandard8PixelType.PixelTypeID,
                                            outputData._atlasImage.width(),
                                            outputData._atlasImage.height());
                // create the texture
                CPVRTexture pvrTexture(pvrHeader, outputData._atlasImage.bits());
                switch (_pixelFormat) {
                    case kETC1: Transcode(pvrTexture, PixelType(ePVRTPF_ETC1), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    case kETC2: Transcode(pvrTexture, PixelType(ePVRTPF_ETC2_RGB), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    case kETC2A: Transcode(pvrTexture, PixelType(ePVRTPF_ETC2_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    case kPVRTC2: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_2bpp_RGB), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    case kPVRTC2A: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_2bpp_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    case kPVRTC4: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_4bpp_RGB), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    case kPVRTC4A: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_4bpp_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                    default: Transcode(pvrTexture, PixelType(ePVRTPF_ETC1), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB); break;
                }

                qDebug() << "Transcode complete.";
                // save the file
                if (_imageFormat == kPVR_CCZ) {
                    //TODO: use qCompress
                    //QByteArray
                    //pvrTexture.
                    //pvrTexture.saveFile((outputFilePath + imagePrefix(_imageFormat)).toStdString().c_str());
                } else {
                    pvrTexture.saveFile((outputFilePath + imagePrefix(_imageFormat)).toStdString().c_str());
                }
                qDebug() << "Write to file complete.";
            }
        }
    }

    if ((_imageFormat == kPNG) && (_pngQuality.optMode != "None")) {
        qDebug() << "Begin optimize image...";
        // we use values 1-7 so that it is more user friendly, because 0 also means optimization.
        optimizePNGInThread(outputFilePaths, _pngQuality.optMode, _pngQuality.optLevel - 1);
    }

    _spriteAtlases.clear();
    _fileNames.clear();

    return true;
}

bool PublishSpriteSheet::generateDataFile(const QString& filePath, const QString& format,  const QMap<QString, SpriteFrameInfo>& spriteFrames, const QImage& atlasImage, bool errorMessage) {
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
    QJSValue result = engine.evaluate(contents);
    if (result.isError()) {
        QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
        qDebug() << errorString;
        if (errorMessage) QMessageBox::critical(NULL, "Export script error", errorString);
        return false;
    }

    if (engine.globalObject().hasOwnProperty("exportSpriteSheet")) {
        QJSValueList args;
        args << QJSValue(filePath);
        if (_imageFormat == kJPG_PNG) {
            QJSValue imageFilePathsValue = engine.newObject();
            imageFilePathsValue.setProperty("rgb", QJSValue(filePath + imagePrefix(kJPG)));
            imageFilePathsValue.setProperty("mask", QJSValue(filePath + imagePrefix(kPNG)));
            args << imageFilePathsValue;
        } else {
            args << QJSValue(filePath + imagePrefix(_imageFormat));
        }

        // collect sprite frames
        QJSValue spriteFramesValue = engine.newObject();
        auto it_f = spriteFrames.cbegin();
        for (; it_f != spriteFrames.cend(); ++it_f) {
            QJSValue spriteFrameValue = engine.newObject();
            spriteFrameValue.setProperty("frame", jsValue(engine, it_f.value().frame));
            spriteFrameValue.setProperty("offset", jsValue(engine, it_f.value().offset));
            spriteFrameValue.setProperty("rotated", it_f.value().rotated);
            spriteFrameValue.setProperty("sourceColorRect", jsValue(engine, it_f.value().sourceColorRect));
            spriteFrameValue.setProperty("sourceSize", jsValue(engine, it_f.value().sourceSize));
            spriteFrameValue.setProperty("triangles", jsValue(engine, it_f.value().triangles));

            QString name = it_f.key();
            // remove root folder if needed
            if (!_prependSmartFolderName) {
                auto idx = name.indexOf('/');
                if (idx != -1) {
                    name = name.right(name.length() - idx - 1);
                }
            }            
            if (_trimSpriteNames) {
                name = QDir::fromNativeSeparators(QFileInfo(name).path() + QDir::separator() + QFileInfo(name).baseName());
            }
            spriteFramesValue.setProperty(name, spriteFrameValue);
        }
        args << QJSValue(spriteFramesValue);

        args << jsValue(engine, atlasImage.size());

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

void PublishSpriteSheet::optimizePNGInThread(QStringList fileNames, const QString& optMode, int optLevel) {
    QObject::connect(&_watcher, SIGNAL(finished()), this, SIGNAL(onCompletedOptimizePNG()));

    QFuture<bool> resultFuture;

    for (const QString& fileName : fileNames) {
        resultFuture = QtConcurrent::run(this, &PublishSpriteSheet::optimizePNG, fileName, optMode, optLevel);
    }

    _watcher.setFuture(resultFuture);
}
