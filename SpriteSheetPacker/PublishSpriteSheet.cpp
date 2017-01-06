#include "PublishSpriteSheet.h"
#include "SpritePackerProjectFile.h"
#include "SpriteAtlas.h"
#include "PListSerializer.h"
#include <QMessageBox>
#include "PngOptimizer.h"
#include "PVRTexture.h"
#include "PVRTextureUtilities.h"

/////////////////////////////////////////////////////////////////////////////////////////////
unsigned int checksumPvr(const unsigned int *data, unsigned int len) {
    unsigned int cs = 0;
    const int cslen = 128;

    len = (len < cslen) ? len : cslen;

    for(unsigned int i = 0; i < len; i++) {
        cs = cs ^ data[i];
    }

    return cs;
}

void encodePvr(unsigned int *data, unsigned int len, unsigned int keys[4]) {
    unsigned int encryptionKey[1024];
    memset(encryptionKey, 0, sizeof(encryptionKey));

    bool s_bEncryptionKeyIsValid = false;

    const int enclen = 1024;
    const int securelen = 512;
    const int distance = 64;

    // create long key
    if(!s_bEncryptionKeyIsValid) {
        unsigned int y, p, e;
        unsigned int rounds = 6;
        unsigned int sum = 0;
        unsigned int z = encryptionKey[enclen-1];

        do {
#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (keys[(p&3)^e] ^ z)))

            sum += DELTA;
            e = (sum >> 2) & 3;

            for (p = 0; p < enclen - 1; p++)
            {
                y = encryptionKey[p + 1];
                z = encryptionKey[p] += MX;
            }

            y = encryptionKey[0];
            z = encryptionKey[enclen - 1] += MX;

        } while (--rounds);

        s_bEncryptionKeyIsValid = true;
    }

    int b = 0;
    unsigned int i = 0;

    // encrypt first part completely
    for(; i < len && i < securelen; i++) {
        data[i] ^= encryptionKey[b++];

        if(b >= enclen) {
            b = 0;
        }
    }

    // encrypt second section partially
    for(; i < len; i += distance) {
        data[i] ^= encryptionKey[b++];

        if(b >= enclen) {
            b = 0;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////

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
            QString fileName = outputFilePath + imagePrefix(_imageFormat);
            qDebug() << "Save image:" << fileName;
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
                                            outputData._atlasImage.height(),
                                            outputData._atlasImage.width());
                // create the texture
                CPVRTexture pvrTexture(pvrHeader, outputData._atlasImage.bits());
                switch (_pixelFormat) {
                    case kETC1: Transcode(pvrTexture, PixelType(ePVRTPF_ETC1), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eETCFast, true); break;
                    case kETC2: Transcode(pvrTexture, PixelType(ePVRTPF_ETC2_RGB), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eETCFast, true); break;
                    case kETC2A: Transcode(pvrTexture, PixelType(ePVRTPF_ETC2_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eETCFast, true); break;
                    case kPVRTC2: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_2bpp_RGB), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, ePVRTCBest, true); break;
                    case kPVRTC2A: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_2bpp_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, ePVRTCBest, true); break;
                    case kPVRTC4: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_4bpp_RGB), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, ePVRTCBest, true); break;
                    case kPVRTC4A: Transcode(pvrTexture, PixelType(ePVRTPF_PVRTCI_4bpp_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, ePVRTCBest, true); break;
                    default: Transcode(pvrTexture, PixelType(ePVRTPF_ETC2_RGBA), ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eETCFast, true); break;
                }

                qDebug() << "Transcode complete.";
                // save the file
                if (_imageFormat == kPVR_CCZ) {
                    QString tempFileName = outputFilePath + "_temp.pvr";
                    pvrTexture.saveFile(tempFileName.toStdString().c_str());

                    // read and compress
                    QFile file(tempFileName);
                    file.open(QIODevice::ReadOnly);
                    unsigned int uncompressedLen = file.size();
                    QByteArray compressedData = qCompress(file.readAll());
                    file.close();
                    QFile::remove(tempFileName);

                    //  Strip the first six bytes (a 4-byte length put on by qCompress)
                    compressedData.remove(0, 4);

                    struct CCZHeader {
                        unsigned char   sig[4];             /** Signature. Should be 'CCZ!' 4 bytes. */
                        unsigned short  compression_type;   /** Should be 0. */
                        unsigned short  version;            /** Should be 2 (although version type==1 is also supported). */
                        unsigned int    reserved;           /** Reserved for users. */
                        unsigned int    len;                /** Size of the uncompressed file. */
                    };

                    CCZHeader cczHeader;
                    cczHeader.sig[0] = 'C';
                    cczHeader.sig[1] = 'C';
                    cczHeader.sig[2] = 'Z';
                    cczHeader.sig[3] = _encryptionKey.isEmpty()? '!':'p';
                    cczHeader.compression_type = qToBigEndian<unsigned short>(0);
                    cczHeader.version = qToBigEndian<unsigned short>(0);
                    cczHeader.reserved = qToBigEndian<unsigned int>(0);
                    cczHeader.len = qToBigEndian<unsigned int>(uncompressedLen);

                    compressedData.insert(0, QByteArray((const char *)&cczHeader, sizeof(CCZHeader)));

                    // encrypt
                    if (!_encryptionKey.isEmpty()) {
                        QString key = _encryptionKey;
                        uint32_t keys[4];
                        keys[0] = key.left(8).toUInt(nullptr, 16); key.remove(0, 8);
                        keys[1] = key.left(8).toUInt(nullptr, 16); key.remove(0, 8);
                        keys[2] = key.left(8).toUInt(nullptr, 16); key.remove(0, 8);
                        keys[3] = key.left(8).toUInt(nullptr, 16); key.remove(0, 8);

                        unsigned int* ints = (unsigned int*)(compressedData.data()+12);
                        unsigned int enclen = (compressedData.length()-12)/4;

                        CCZHeader* header = (CCZHeader*)compressedData.data();
                        header->reserved = qToBigEndian<unsigned int>(checksumPvr(ints, enclen));

                        encodePvr(ints, enclen, keys);
                    }

                    // write compressed data
                    file.setFileName(fileName);
                    file.open(QIODevice::WriteOnly);
                    file.write(compressedData);
                    file.close();
                } else {
                    pvrTexture.saveFile(fileName.toStdString().c_str());
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
