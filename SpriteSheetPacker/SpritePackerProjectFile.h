#ifndef SPRITEPACKERPROJECTFILE_H
#define SPRITEPACKERPROJECTFILE_H

#include <QtCore>
#include "GenericObjectFactory.h"

struct ScalingVariant{
    QString name;
    float   scale;
};

class SpritePackerProjectFile
{
public:
    SpritePackerProjectFile();


    void setTrimThreshold(int trimThreshold) { _trimThreshold = trimThreshold; }
    int trimThreshold() const { return _trimThreshold; }

    void setTextureBorder(int textureBorder) { _textureBorder = textureBorder; }
    int textureBorder() const { return _textureBorder; }

    void setSpriteBorder(int spriteBorder) { _spriteBorder = spriteBorder; }
    int spriteBorder() const { return _spriteBorder; }

    void setMaxTextureSize(int maxTextureSize) { _maxTextureSize = maxTextureSize; }
    int maxTextureSize() const { return _maxTextureSize; }

    void setPot2(bool pot2) { _pot2 = pot2; }
    bool pot2() const { return _pot2; }

    void setOptLevel(int optLevel) { _optLevel = optLevel; }
    int optLevel() const { return _optLevel; }

    void setScalingVariants(const QVector<ScalingVariant>& scalingVariants) { _scalingVariants = scalingVariants; }
    const QVector<ScalingVariant>& scalingVariants() const { return _scalingVariants; }

    void setDataFormat(const QString& dataFormat) { _dataFormat = dataFormat; }
    const QString& dataFormat() const { return _dataFormat; }

    void setDestPath(const QString& destPath) { _destPath = destPath; }
    const QString& destPath() const { return _destPath; }

    void setSpriteSheetName(const QString& spriteSheetName) { _spriteSheetName = spriteSheetName; }
    const QString& spriteSheetName() const { return _spriteSheetName; }

    void setSrcList(const QStringList& srcList) { _srcList = srcList; }
    const QStringList& srcList() const { return _srcList; }

    virtual bool write(const QString& fileName);
    virtual bool read(const QString& fileName);

    static GenericObjectFactory<std::string, SpritePackerProjectFile>& factory() {
        return _factory;
    }

protected:
    int     _trimThreshold;
    int     _textureBorder;
    int     _spriteBorder;
    int     _maxTextureSize;
    bool    _pot2;
    int     _optLevel;

    QVector<ScalingVariant> _scalingVariants;
    QString _dataFormat;

    QString     _destPath;
    QString     _spriteSheetName;
    QStringList _srcList;

private:
    static GenericObjectFactory<std::string, SpritePackerProjectFile> _factory;
};

class SpritePackerProjectFileOLD: public SpritePackerProjectFile {
public:
    virtual bool write(const QString&) { return false; }
    virtual bool read(const QString& fileName);
};

class SpritePackerProjectFileTPS: public SpritePackerProjectFile {
public:
    virtual bool write(const QString&) { return false; }
    virtual bool read(const QString& fileName);
};


#endif // SPRITEPACKERPROJECTFILE_H
