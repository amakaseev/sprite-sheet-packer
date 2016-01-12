#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include <QtCore>
#include <QImage>

#include "PolygonImage.h"

struct SpriteFrameInfo {
public:
    QRect   mFrame;
    QPoint  mOffset;
    bool    mRotated;
    QRect   mSourceColorRect;
    QSize   mSourceSize;

    Triangles triangles;
};

class SpriteAtlas
{
public:
    SpriteAtlas(const QStringList& sourceList, int textureBorder = 0, int spriteBorder = 1, int trim = 1, bool pow2 = false, int maxSize = 8192, float scale = 1);

    bool enablePolygonMode(bool enable, float epsilon = 2.f);
    bool generate();

    const QImage& image() const { return _atlasImage; }
    const QMap<QString, SpriteFrameInfo>& spriteFrames() const { return _spriteFrames; }
    const QMap<QString, QVector<QString>>& identicalFrames() const { return _identicalFrames; }

private:
    QStringList _sourceList;
    int _textureBorder;
    int _spriteBorder;
    int _trim;
    bool _pow2;
    int _maxTextureSize;
    float _scale;
    // polygon mode
    struct TPolygonMode{
        bool enable;
        float epsilon;
    } _polygonMode;

    QImage _atlasImage;
    QMap<QString, SpriteFrameInfo> _spriteFrames;
    QMap<QString, QVector<QString>> _identicalFrames;
};

#endif // SPRITEATLAS_H
