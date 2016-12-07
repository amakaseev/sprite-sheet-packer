#ifndef SPRITEPACKERPROJECTFILE_H
#define SPRITEPACKERPROJECTFILE_H

#include <QtCore>
#include "ImageFormat.h"
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

    void setImageFormat(ImageFormat imageFormat) { _imageFormat = imageFormat; }
    ImageFormat imageFormat() const { return _imageFormat; }

    void setPixelFormat(PixelFormat pixelFormat) { _pixelFormat = pixelFormat; }
    PixelFormat pixelFormat() const { return _pixelFormat; }

    void setPremultiplied(bool premultiplied) { _premultiplied = premultiplied; }
    bool premultiplied() const { return _premultiplied; }

    void setPngOptMode(const QString& optMode) { _pngOptMode = optMode; }
    const QString& pngOptMode() const { return _pngOptMode; }

    void setPngOptLevel(int optLevel) { _pngOptLevel = optLevel; }
    int pngOptLevel() const { return _pngOptLevel; }

    void setJpgQuality(int quality) { _jpgQuality = quality; }
    int jpgQuality() const { return _jpgQuality; }

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

    void setTrimSpriteNames(bool trimSpriteNames) { _trimSpriteNames = trimSpriteNames; }
    bool trimSpriteNames() const { return _trimSpriteNames; }

    void setPrependSmartFolderName(bool prependSmartFolderName) { _prependSmartFolderName = prependSmartFolderName; }
    bool prependSmartFolderName() const { return _prependSmartFolderName; }

    virtual bool write(const QString& fileName);
    virtual bool read(const QString& fileName);

    static GenericObjectFactory<std::string, SpritePackerProjectFile>& factory() {
        return _factory;
    }

protected:
    QString     _algorithm;
    QString     _trimMode;
    int         _trimThreshold;
    float       _epsilon;
    int         _textureBorder;
    int         _spriteBorder;
    ImageFormat _imageFormat;
    PixelFormat _pixelFormat;
    bool        _premultiplied;

    QString     _pngOptMode;
    int         _pngOptLevel;
    int         _jpgQuality;

    QVector<ScalingVariant> _scalingVariants;

    QString     _dataFormat;
    QString     _destPath;
    QString     _spriteSheetName;
    QStringList _srcList;

    bool        _trimSpriteNames;
    bool        _prependSmartFolderName;

private:
    static GenericObjectFactory<std::string, SpritePackerProjectFile> _factory;
};

class SpritePackerProjectFileTPS: public SpritePackerProjectFile {
public:
    virtual bool write(const QString&) { return false; }
    virtual bool read(const QString& fileName);
};


#endif // SPRITEPACKERPROJECTFILE_H
