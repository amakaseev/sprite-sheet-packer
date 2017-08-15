#include "SpritePackerProjectFile.h"
#include "PublishSpriteSheet.h"

#include "TPSParser.h"
#include "PListParser.h"
#include "PListSerializer.h"

GenericObjectFactory<std::string, SpritePackerProjectFile> SpritePackerProjectFile::_factory;

SpritePackerProjectFile::SpritePackerProjectFile() {
    _algorithm = "Rect";
    _trimMode = "Rect";
    _trimThreshold = 1;
    _epsilon = 5;
    _heuristicMask = false;
    _rotateSprites = false;
    _textureBorder = 0;
    _spriteBorder = 2;
    _imageFormat = kPNG,
    _pixelFormat = kARGB8888;
    _premultiplied = true;
    _pngOptMode = "None";
    _pngOptLevel = 7;
    _jpgQuality = 80;
    _webpQuality = 80;

    _trimSpriteNames = true;
    _prependSmartFolderName = true;
}

SpritePackerProjectFile::~SpritePackerProjectFile() {

}


bool SpritePackerProjectFile::read(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

    QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();

    if (json.isEmpty()) {
        return false;
    }

    if (json.contains("algorithm")) _algorithm = json["algorithm"].toString();
    if (json.contains("trimMode")) _trimMode = json["trimMode"].toString();
    if (json.contains("trimThreshold")) _trimThreshold = json["trimThreshold"].toInt();
    if (json.contains("epsilon")) _epsilon = json["epsilon"].toDouble();
    if (json.contains("heuristicMask")) _heuristicMask = json["heuristicMask"].toBool();
    if (json.contains("rotateSprites")) _rotateSprites = json["rotateSprites"].toBool();
    if (json.contains("textureBorder")) _textureBorder = json["textureBorder"].toInt();
    if (json.contains("spriteBorder")) _spriteBorder = json["spriteBorder"].toInt();
    if (json.contains("imageFormat")) _imageFormat = imageFormatFromString(json["imageFormat"].toString());
    if (json.contains("pixelFormat")) _pixelFormat = pixelFormatFromString(json["pixelFormat"].toString());
    if (json.contains("premultiplied")) _premultiplied = json["premultiplied"].toBool();
    if (json.contains("pngOptMode")) _pngOptMode = json["pngOptMode"].toString();
    if (json.contains("pngOptLevel")) _pngOptLevel = json["pngOptLevel"].toInt();
    if (json.contains("webpQuality")) _webpQuality = json["webpQuality"].toInt();
    if (json.contains("jpgQuality")) _jpgQuality = json["jpgQuality"].toInt();

    _scalingVariants.clear();
    QJsonArray scalingVariants = json["scalingVariants"].toArray();
    for (int i=0; i<scalingVariants.size(); ++i) {
        QJsonObject scalingVariantObject = scalingVariants[i].toObject();
        ScalingVariant scalingVariant;
        scalingVariant.name = scalingVariantObject["name"].toString();
        scalingVariant.scale = scalingVariantObject["scale"].toDouble();
        scalingVariant.maxTextureSize = scalingVariantObject.contains("maxTextureSize")? scalingVariantObject["maxTextureSize"].toInt() : 2048;
        scalingVariant.pow2 = scalingVariantObject.contains("pow2")? scalingVariantObject["pow2"].toBool() : false;
        scalingVariant.forceSquared = scalingVariantObject.contains("forceSquared")? scalingVariantObject["forceSquared"].toBool() : false;

        _scalingVariants.push_back(scalingVariant);
    }

    if (json.contains("dataFormat"))  _dataFormat = json["dataFormat"].toString();
    if (json.contains("destPath")) _destPath = QDir(dir.absoluteFilePath(json["destPath"].toString())).canonicalPath();
    if (json.contains("spriteSheetName")) _spriteSheetName = json["spriteSheetName"].toString();

    _srcList.clear();
    QJsonArray srcRelative = json["srcList"].toArray();
    for (auto src: srcRelative) {
        _srcList.push_back(dir.absoluteFilePath(src.toString()));
    }

    if (json.contains("trimSpriteNames")) _trimSpriteNames = json["trimSpriteNames"].toBool();
    if (json.contains("prependSmartFolderName")) _prependSmartFolderName = json["prependSmartFolderName"].toBool();

    if (json.contains("encryptionKey")) _encryptionKey = json["encryptionKey"].toString();

    return true;
}

bool SpritePackerProjectFile::write(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QJsonObject json;

    json["algorithm"] = _algorithm;
    json["trimMode"] = _trimMode;
    json["trimThreshold"] = _trimThreshold;
    json["epsilon"] = _epsilon;
    json["heuristicMask"] = _heuristicMask;
    json["rotateSprites"] = _rotateSprites;
    json["textureBorder"] = _textureBorder;
    json["spriteBorder"] = _spriteBorder;
    json["imageFormat"] = imageFormatToString(_imageFormat);
    json["pixelFormat"] = pixelFormatToString(_pixelFormat);
    json["premultiplied"] = _premultiplied;
    json["pngOptMode"] = _pngOptMode;
    json["pngOptLevel"] = _pngOptLevel;
    json["webpQuality"] = _webpQuality;
    json["jpgQuality"] = _jpgQuality;

    QJsonArray scalingVariants;
    for (auto scalingVariant: _scalingVariants) {
        QJsonObject scalingVariantObject;
        scalingVariantObject["name"] = scalingVariant.name;
        scalingVariantObject["scale"] = scalingVariant.scale;
        scalingVariantObject["maxTextureSize"] = scalingVariant.maxTextureSize;
        scalingVariantObject["pow2"] = scalingVariant.pow2;
        scalingVariantObject["forceSquared"] = scalingVariant.forceSquared;
        scalingVariants.append(scalingVariantObject);
    }
    json["scalingVariants"] = scalingVariants;
    json["dataFormat"] = _dataFormat;
    json["destPath"] = dir.relativeFilePath(_destPath);
    json["spriteSheetName"] = _spriteSheetName;
    QStringList srcRelative;
    for (auto src: _srcList) {
        srcRelative.push_back(dir.relativeFilePath(src));
    }
    json["srcList"] = QJsonArray::fromStringList(srcRelative);
    json["trimSpriteNames"] = _trimSpriteNames;
    json["prependSmartFolderName"] = _prependSmartFolderName;
    json["encryptionKey"] = _encryptionKey;

    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(QJsonDocument(json).toJson());

    return true;
}

bool SpritePackerProjectFileTPS::read(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QVariant tps = TPSParser::parse(&file);

    if (!tps.isValid()) {
        return false;
    }

    float globalScale = 1;

    QVariantMap tpsMap = tps.toMap();

    if (PublishSpriteSheet::formats().find(tpsMap["dataFormat"].toString()) != PublishSpriteSheet::formats().end()) {
        _dataFormat = tpsMap["dataFormat"].toString();
    }

    if (tpsMap.find("trimSpriteNames") != tpsMap.end()) {
        _trimSpriteNames = tpsMap["trimSpriteNames"].toBool();
    }

    if (tpsMap.find("prependSmartFolderName") != tpsMap.end()) {
        _prependSmartFolderName = tpsMap["prependSmartFolderName"].toBool();
    }

    if (tpsMap.find("heuristicMask") != tpsMap.end()) {
        _heuristicMask = tpsMap["heuristicMask"].toBool();
    }

    if (tpsMap.find("globalSpriteSettings") != tpsMap.end()) {
        QVariantMap globalSpriteSettings = tpsMap["globalSpriteSettings"].toMap();
        _trimThreshold = globalSpriteSettings["trimThreshold"].toInt();

        if (globalSpriteSettings.find("scale") != tpsMap.end()) {
            globalScale = globalSpriteSettings["scale"].toFloat();
        }
    }

    if (tpsMap.find("borderPadding") != tpsMap.end()) {
        _textureBorder = tpsMap["borderPadding"].toInt();
    }
    if (tpsMap.find("shapePadding") != tpsMap.end()) {
        _spriteBorder = tpsMap["shapePadding"].toInt();
    }

    if (tpsMap.find("outputFormat") != tpsMap.end()) {
        QString outputFormat = tpsMap["outputFormat"].toString();
        if (outputFormat == "RGBA8888") {
            _pixelFormat = kARGB8888;
        } else if (outputFormat == "RGBA4444") {
            _pixelFormat = kARGB4444;
        } else if (outputFormat == "RGBA5551") {
            _pixelFormat = kARGB4444;
        } else if (outputFormat == "RGBA5555") {
            _pixelFormat = kARGB8565;
        } else if (outputFormat == "BGRA8888") {
            _pixelFormat = kARGB8888;
        } else if (outputFormat == "RGB8888") {
            _pixelFormat = kRGB888;
        } else if (outputFormat == "RGB565") {
            _pixelFormat = kRGB565;
        } else if (outputFormat == "ALPHA") {
            _pixelFormat = kALPHA;
        } else if (outputFormat == "ALPHA_INTENSITY") {
            _pixelFormat = kALPHA;
        }
    }

    int maxTextureSize = 2048;
    if (tpsMap.find("maxTextureSize") != tpsMap.end()) {
        maxTextureSize = qMax(tpsMap["maxTextureSize"].toMap()["width"].toInt(), tpsMap["maxTextureSize"].toMap()["height"].toInt());
    }

    bool pow2 = false;
    bool forceSquared = false;
    if (tpsMap.find("algorithmSettings") != tpsMap.end()) {
        QVariantMap algorithmSettings = tpsMap["algorithmSettings"].toMap();
        if (algorithmSettings["sizeConstraints"].toString() == "POT") {
            pow2 = true;
        } else {
            pow2 = false;
        }

        forceSquared = algorithmSettings["forceSquared"].toBool();
    }

//    if (tpsMap.find("pngOptimizationLevel") != tpsMap.end()) {
//         _pngOptLevel = qBound(1, tpsMap["pngOptimizationLevel"].toInt(), 7);
//         _pngOptMode = "Lossless";
//    }

    if (tpsMap.find("webpQuality") != tpsMap.end()) {
         _webpQuality = qBound(0, tpsMap["webpQualityLevel"].toInt(), 100);
    }

    if (tpsMap.find("jpgQuality") != tpsMap.end()) {
         _jpgQuality = qBound(0, tpsMap["jpgQuality"].toInt(), 100);
    }

    if (tpsMap.find("dataFileNames") != tpsMap.end()) {
        QVariantMap dataFileNames = tpsMap["dataFileNames"].toMap();
        if (dataFileNames.find("data") != dataFileNames.end()) {
            QVariantMap data = dataFileNames["data"].toMap();
            if (data.find("name") != data.end()) {
                QFileInfo fi(dir.absoluteFilePath(data["name"].toString()));
                _destPath = fi.dir().canonicalPath();
                _spriteSheetName = fi.baseName();
            }
        }
    }

    if (tpsMap.find("autoSDSettings") != tpsMap.end()) {
        QVariantList autoSDSettings = tpsMap["autoSDSettings"].toList();
        for (auto autoSDSettingVariant: autoSDSettings) {
            QVariantMap autoSDSetting = autoSDSettingVariant.toMap();
            ScalingVariant scalingVariant;
            scalingVariant.name = autoSDSetting["extension"].toString();
            scalingVariant.scale = autoSDSetting["scale"].toFloat() * globalScale;

            scalingVariant.maxTextureSize = qMax(autoSDSetting["maxTextureSize"].toMap()["width"].toInt(),
                                                 autoSDSetting["maxTextureSize"].toMap()["height"].toInt());

            if (scalingVariant.maxTextureSize <= 0) {
                scalingVariant.maxTextureSize = maxTextureSize;
            }

            scalingVariant.pow2 = pow2;
            scalingVariant.forceSquared = forceSquared;
            _scalingVariants.push_back(scalingVariant);
        }
    }

    _srcList.clear();
    QVariantList spritesList = tpsMap["fileList"].toList();
    foreach (QVariant spriteFile, spritesList) {
        _srcList.append(dir.absoluteFilePath(spriteFile.toString()));
    }

    qDebug() << tps;

    return true;
}
