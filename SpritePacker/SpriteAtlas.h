#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include <QtCore>
#include <QImage>

struct SpriteFrameInfo {
public:
    QRect   mFrame;
    QPoint  mOffset;
    bool    mRotated;
    QRect   mSourceColorRect;
    QSize   mSourceSize;
};

class SpriteAtlas
{
public:
    SpriteAtlas(const QStringList& sourceList, int textureBorder = 0, int spriteBorder = 1, int trim = 1, bool pot2 = false, int maxSize = 8192, float scale = 1);

    void generate();

    const QImage& image() const { return _atlasImage; }
    const QMap<QString, SpriteFrameInfo>& spriteFrames() const { return _spriteFrames; }

private:
    QStringList _sourceList;
    int _textureBorder;
    int _spriteBorder;
    int _trim;
    bool _pot2;
    int _maxTextureSize;
    float _scale;

    QImage _atlasImage;
    QMap<QString, SpriteFrameInfo> _spriteFrames;
};

#endif // SPRITEATLAS_H
