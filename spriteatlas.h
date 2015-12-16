#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include <QtCore>

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
    SpriteAtlas();

    static bool generate(const QStringList& sourceList, int textureBorder, int spriteBorder, int trim, float scale, QImage& atlasImage, QMap<QString, SpriteFrameInfo>& spriteFrames);
};

#endif // SPRITEATLAS_H
