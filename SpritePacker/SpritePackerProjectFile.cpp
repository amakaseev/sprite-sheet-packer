#include "SpritePackerProjectFile.h"

#include "PListParser.h"
#include "PListSerializer.h"

GenericObjectFactory<std::string, SpritePackerProjectFile> SpritePackerProjectFile::_factory;

SpritePackerProjectFile::SpritePackerProjectFile() {

}


bool SpritePackerProjectFile::read(const QString &fileName) {
    QDir dir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

    QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();

    _spritesPrefix = json["spritesPrefix"].toString();
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
        scalingVariant.folderName = scalingVariantObject["folderName"].toString();
        scalingVariant.scale = scalingVariantObject["scale"].toDouble();
        _scalingVariants.push_back(scalingVariant);
    }

    if (json["dataFormat"].toString() == "Cocos2D") {
        _dataFormat = kCocos2D;
    } else if (json["dataFormat"].toString() == "Json") {
        _dataFormat = kJson;
    }

    _destPath = dir.absoluteFilePath(json["destPath"].toString());
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

    json["spritesPrefix"] = _spritesPrefix;
    json["trimThreshold"] = _trimThreshold;
    json["textureBorder"] = _textureBorder;
    json["spriteBorder"] = _spriteBorder;
    json["maxTextureSize"] = _maxTextureSize;
    json["pot2"] = _pot2;

    QJsonArray scalingVariants;
    for (auto scalingVariant: _scalingVariants) {
        QJsonObject scalingVariantObject;
        scalingVariantObject["folderName"] = scalingVariant.folderName;
        scalingVariantObject["scale"] = scalingVariant.scale;
        scalingVariants.append(scalingVariantObject);
    }
    json["scalingVariants"] = scalingVariants;

    switch (_dataFormat) {
        case kCocos2D: json["dataFormat"] = "Cocos2D"; break;
        case kJson: json["dataFormat"] = "Json"; break;
    }

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

    // unsupported variables
    _maxTextureSize = 2048;
    _pot2 = false;

    QVariantMap plistDict = plist.toMap();

    // load property
    QVariantMap propertyMap = plistDict["property"].toMap();
    _spritesPrefix = propertyMap["spritesPrefix"].toString();

    // PACKING
    QVariantMap packingMap = propertyMap["packing"].toMap();
    _trimThreshold = packingMap["threshold"].toInt();
    _textureBorder = packingMap["textureBorder"].toInt();
    _spriteBorder = packingMap["spriteBorder"].toInt();

    // OUTPUT
    QVariantMap outputMap = propertyMap["output"].toMap();
    _dataFormat = (TDataFormat)outputMap["dataFormat"].toInt();
    _destPath = QDir(dir.absoluteFilePath(outputMap["destPath"].toString())).absolutePath();
    _spriteSheetName = outputMap["spriteSheetName"].toString();

    _scalingVariants.clear();
    QVariantMap hdrMap = outputMap["HDR"].toMap();
    if (hdrMap["enable"].toBool()) {
        ScalingVariant scalingVariant;
        scalingVariant.scale = hdrMap["scale"].toInt() / 100.f;
        scalingVariant.folderName = hdrMap["folder"].toString();
        _scalingVariants.push_back(scalingVariant);
    }
    QVariantMap hdMap = outputMap["HD"].toMap();
    if (hdMap["enable"].toBool()) {
        ScalingVariant scalingVariant;
        scalingVariant.scale = hdMap["scale"].toInt() / 100.f;
        scalingVariant.folderName = hdMap["folder"].toString();
        _scalingVariants.push_back(scalingVariant);
    }
    QVariantMap sdMap = outputMap["SD"].toMap();
    if (sdMap["enable"].toBool()) {
        ScalingVariant scalingVariant;
        scalingVariant.scale = sdMap["scale"].toInt() / 100.f;
        scalingVariant.folderName = sdMap["folder"].toString();
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
    return true;
}
