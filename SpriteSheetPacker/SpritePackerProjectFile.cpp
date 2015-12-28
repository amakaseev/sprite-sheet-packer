#include "SpritePackerProjectFile.h"

#include "TPSParser.h"
#include "PListParser.h"
#include "PListSerializer.h"

GenericObjectFactory<std::string, SpritePackerProjectFile> SpritePackerProjectFile::_factory;

SpritePackerProjectFile::SpritePackerProjectFile() {
    _trimThreshold = 1;
    _textureBorder = 0;
    _spriteBorder = 2;
    _maxTextureSize = 8192;
    _pot2 = false;
}


bool SpritePackerProjectFile::read(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

    QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();

    _trimThreshold = json["trimThreshold"].toInt();
    _textureBorder = json["textureBorder"].toInt();
    _spriteBorder = json["spriteBorder"].toInt();
    _maxTextureSize = json["maxTextureSize"].toInt();
    _pot2 = json["pot2"].toBool();

    _scalingVariants.clear();
    QJsonArray scalingVariants = json["scalingVariants"].toArray();
    for (int i=0; i<scalingVariants.size(); ++i) {
        QJsonObject scalingVariantObject = scalingVariants[i].toObject();
        ScalingVariant scalingVariant;
        scalingVariant.name = scalingVariantObject["name"].toString();
        scalingVariant.scale = scalingVariantObject["scale"].toDouble();
        _scalingVariants.push_back(scalingVariant);
    }

    _dataFormat = json["dataFormat"].toString();
    _destPath = QDir(dir.absoluteFilePath(json["destPath"].toString())).canonicalPath();
    _spriteSheetName = json["spriteSheetName"].toString();

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

    json["trimThreshold"] = _trimThreshold;
    json["textureBorder"] = _textureBorder;
    json["spriteBorder"] = _spriteBorder;
    json["maxTextureSize"] = _maxTextureSize;
    json["pot2"] = _pot2;

    QJsonArray scalingVariants;
    for (auto scalingVariant: _scalingVariants) {
        QJsonObject scalingVariantObject;
        scalingVariantObject["name"] = scalingVariant.name;
        scalingVariantObject["scale"] = scalingVariant.scale;
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

bool SpritePackerProjectFileOLD::read(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QVariant plist = PListParser::parsePList(&file);

    if (!plist.isValid()) {
        return false;
    }

    QVariantMap plistDict = plist.toMap();

    // load property
    QVariantMap propertyMap = plistDict["property"].toMap();

    // PACKING
    QVariantMap packingMap = propertyMap["packing"].toMap();
    _trimThreshold = packingMap["threshold"].toInt();
    _textureBorder = packingMap["textureBorder"].toInt();
    _spriteBorder = packingMap["spriteBorder"].toInt();

    // OUTPUT
    QVariantMap outputMap = propertyMap["output"].toMap();
    switch (outputMap["dataFormat"].toInt()) {
    case 0: _dataFormat = "cocos2d"; break;
    case 1: _dataFormat = "json"; break;
    default: _dataFormat = "cocos2d"; break;
    }
    _destPath = QDir(dir.absoluteFilePath(outputMap["destPath"].toString())).canonicalPath();
    _spriteSheetName = outputMap["spriteSheetName"].toString();

    _scalingVariants.clear();
    QVariantMap hdrMap = outputMap["HDR"].toMap();
    if (hdrMap["enable"].toBool()) {
        ScalingVariant scalingVariant;
        scalingVariant.scale = hdrMap["scale"].toInt() / 100.f;
        scalingVariant.name = hdrMap["folder"].toString();
        _scalingVariants.push_back(scalingVariant);
    }
    QVariantMap hdMap = outputMap["HD"].toMap();
    if (hdMap["enable"].toBool()) {
        ScalingVariant scalingVariant;
        scalingVariant.scale = hdMap["scale"].toInt() / 100.f;
        scalingVariant.name = hdMap["folder"].toString();
        _scalingVariants.push_back(scalingVariant);
    }
    QVariantMap sdMap = outputMap["SD"].toMap();
    if (sdMap["enable"].toBool()) {
        ScalingVariant scalingVariant;
        scalingVariant.scale = sdMap["scale"].toInt() / 100.f;
        scalingVariant.name = sdMap["folder"].toString();
        _scalingVariants.push_back(scalingVariant);
    }

    _srcList.clear();
    QVariantList spritesList = plistDict["sprites"].toList();
    foreach (QVariant spriteFile, spritesList) {
        _srcList.append(dir.absoluteFilePath(spriteFile.toString()));
    }

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

    QVariantMap tpsMap = tps.toMap();

    if (tpsMap.find("globalSpriteSettings") != tpsMap.end()) {
        QVariantMap globalSpriteSettings = tpsMap["globalSpriteSettings"].toMap();
        _trimThreshold = globalSpriteSettings["trimThreshold"].toInt();
    }
    if (tpsMap.find("borderPadding") != tpsMap.end()) {
        _textureBorder = tpsMap["borderPadding"].toInt();
    }
    if (tpsMap.find("shapePadding") != tpsMap.end()) {
        _spriteBorder = tpsMap["shapePadding"].toInt();
    }
    if (tpsMap.find("maxTextureSize") != tpsMap.end()) {
        _maxTextureSize = qMax(tpsMap["maxTextureSize"].toMap()["width"].toInt(), tpsMap["maxTextureSize"].toMap()["height"].toInt());
    }

    if (tpsMap.find("globalSpriteSettings") != tpsMap.end()) {
        QVariantMap globalSpriteSettings = tpsMap["globalSpriteSettings"].toMap();
        _trimThreshold = globalSpriteSettings["trimThreshold"].toInt();
    }
    if (tpsMap.find("algorithmSettings") != tpsMap.end()) {
        QVariantMap algorithmSettings = tpsMap["algorithmSettings"].toMap();
        if (algorithmSettings["sizeConstraints"].toString() == "POT") {
            _pot2 = true;
        } else {
            _pot2 = false;
        }
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
            scalingVariant.scale = autoSDSetting["scale"].toFloat();
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
