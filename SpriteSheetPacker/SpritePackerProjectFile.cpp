#include "SpritePackerProjectFile.h"

#include "TPSParser.h"
#include "PListParser.h"
#include "PListSerializer.h"

GenericObjectFactory<std::string, SpritePackerProjectFile> SpritePackerProjectFile::_factory;

SpritePackerProjectFile::SpritePackerProjectFile() {
    _algorithm = "Rect";
    _trimMode = "Rect";
    _trimThreshold = 1;
    _epsilon = 5;
    _textureBorder = 0;
    _spriteBorder = 2;
    _imageFormat = "PNG";
    _pixelFormat = "RGBA8888";
    _optMode = "None";
    _optLevel = 1;
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
    if (json.contains("textureBorder")) _textureBorder = json["textureBorder"].toInt();
    if (json.contains("spriteBorder")) _spriteBorder = json["spriteBorder"].toInt();
    if (json.contains("imageFormat")) _imageFormat = json["imageFormat"].toString();
    if (json.contains("pixelFormat")) _pixelFormat = json["pixelFormat"].toString();
    if (json.contains("optMode")) _optMode = json["optMode"].toString();
    if (json.contains("optLevel")) _optLevel = json["optLevel"].toInt();

    _scalingVariants.clear();
    QJsonArray scalingVariants = json["scalingVariants"].toArray();
    for (int i=0; i<scalingVariants.size(); ++i) {
        QJsonObject scalingVariantObject = scalingVariants[i].toObject();
        ScalingVariant scalingVariant;
        scalingVariant.name = scalingVariantObject["name"].toString();
        scalingVariant.scale = scalingVariantObject["scale"].toDouble();
        scalingVariant.maxTextureSize = scalingVariantObject.contains("maxTextureSize")? scalingVariantObject["maxTextureSize"].toInt() : 2048;
        scalingVariant.pow2 = scalingVariantObject.contains("pow2")? scalingVariantObject["pow2"].toBool() : false;

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

    return true;
}

bool SpritePackerProjectFile::write(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QJsonObject json;

    json["algorithm"] = _algorithm;
    json["trimMode"] = _trimMode;
    json["trimThreshold"] = _trimThreshold;
    json["epsilon"] = _epsilon;
    json["textureBorder"] = _textureBorder;
    json["spriteBorder"] = _spriteBorder;
    json["imageFormat"] = _imageFormat;
    json["pixelFormat"] = _pixelFormat;
    json["optMode"] = _optMode;
    json["optLevel"] = _optLevel;

    QJsonArray scalingVariants;
    for (auto scalingVariant: _scalingVariants) {
        QJsonObject scalingVariantObject;
        scalingVariantObject["name"] = scalingVariant.name;
        scalingVariantObject["scale"] = scalingVariant.scale;
        scalingVariantObject["maxTextureSize"] = scalingVariant.maxTextureSize;
        scalingVariantObject["pow2"] = scalingVariant.pow2;
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


    int maxTextureSize = 2048;
    if (tpsMap.find("maxTextureSize") != tpsMap.end()) {
        maxTextureSize = qMax(tpsMap["maxTextureSize"].toMap()["width"].toInt(), tpsMap["maxTextureSize"].toMap()["height"].toInt());
    }

    bool pow2 = false;
    if (tpsMap.find("algorithmSettings") != tpsMap.end()) {
        QVariantMap algorithmSettings = tpsMap["algorithmSettings"].toMap();
        if (algorithmSettings["sizeConstraints"].toString() == "POT") {
            pow2 = true;
        } else {
            pow2 = false;
        }
    }

    if (tpsMap.find("pngOptimizationLevel") != tpsMap.end()) {
         _optLevel = qBound(1, tpsMap["pngOptimizationLevel"].toInt(), 7);
         _optMode = "Lossless";
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
