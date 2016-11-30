#ifndef SPRITEPACKERPROJECTFILE_H
#define SPRITEPACKERPROJECTFILE_H

#include <QtCore>
#include "GenericObjectFactory.h"

struct ScalingVariant{
    QString name;
    float   scale;
    int     maxTextureSize;
    bool    pow2;
};

class SpritePackerProjectFile
{
public:
    SpritePackerProjectFile();
    virtual ~SpritePackerProjectFile();

    void setAlgorithm(const QString& algorithm) { _algorithm = algorithm; }
    QString algorithm() const { return _algorithm; }

    void setTrimMode(const QString& trimMode) { _trimMode = trimMode; }
    QString trimMode() const { return _trimMode; }

    void setTrimThreshold(int trimThreshold) { _trimThreshold = trimThreshold; }
    int trimThreshold() const { return _trimThreshold; }

    void setEpsilon(float epsilon) { _epsilon = epsilon; }
    float epsilon() const { return _epsilon; }

    void setTextureBorder(int textureBorder) { _textureBorder = textureBorder; }
    int textureBorder() const { return _textureBorder; }

    void setSpriteBorder(int spriteBorder) { _spriteBorder = spriteBorder; }
    int spriteBorder() const { return _spriteBorder; }

    void setImageFormat(const QString& imageFormat) { _imageFormat = imageFormat; }
    const QString& imageFormat() const { return _imageFormat; }

    void setPixelFormat(const QString& pixelFormat) { _pixelFormat = pixelFormat; }
    const QString pixelFormat() const { return _pixelFormat; }

    void setOptMode(const QString& optMode) { _optMode = optMode; }
    const QString& optMode() const { return _optMode; }

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
    QString _algorithm;
    QString _trimMode;
    int     _trimThreshold;
    float   _epsilon;
    int     _textureBorder;
    int     _spriteBorder;
    QString _imageFormat;
    QString _pixelFormat;
    QString _optMode;
    int     _optLevel;

    QVector<ScalingVariant> _scalingVariants;
    QString _dataFormat;

    QString     _destPath;
    QString     _spriteSheetName;
    QStringList _srcList;

private:
    static GenericObjectFactory<std::string, SpritePackerProjectFile> _factory;
};

class SpritePackerProjectFileTPS: public SpritePackerProjectFile {
public:
    virtual bool write(const QString&) { return false; }
    virtual bool read(const QString& fileName);
};


#endif // SPRITEPACKERPROJECTFILE_H
