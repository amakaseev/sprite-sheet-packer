#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include <QtCore>
#include <QImage>

#include "PolygonImage.h"

struct SpriteFrameInfo {
public:
    QRect   frame;
    QPoint  offset;
    bool    rotated;
    QRect   sourceColorRect;
    QSize   sourceSize;

    Triangles triangles;
};

class PackContent {
public:
    PackContent();
    PackContent(const QString& name, const QImage& image);

    bool isIdentical(const PackContent& other);
    void trim(int alpha);
    void setTriangles(const Triangles& triangles) { _triangles = triangles; }

    const QString& name() const { return _name; }
    const QImage& image() const { return _image; }
    const QRect& rect() const { return _rect; }
    const Triangles& triangles() const { return _triangles; }

private:
    QString _name;
    QImage  _image;
    QRect   _rect;
    Triangles _triangles;
};

class SpriteAtlas
{
public:
    SpriteAtlas(const QStringList& sourceList, int textureBorder = 0, int spriteBorder = 1, int trim = 1, bool pow2 = false, int maxSize = 8192, float scale = 1);

    void setAlgorithm(const QString& algorithm) { _algorithm = algorithm; }
    void enablePolygonMode(bool enable, float epsilon = 2.f);
    bool generate();

    const QImage& image() const { return _atlasImage; }
    const QMap<QString, SpriteFrameInfo>& spriteFrames() const { return _spriteFrames; }
    const QMap<QString, QVector<QString>>& identicalFrames() const { return _identicalFrames; }

protected:
    bool packWithRect(const QVector<PackContent>& content);
    bool packWithPolygon(const QVector<PackContent>& content);

private:
    QStringList _sourceList;
    QString _algorithm;
    int _trim;
    int _textureBorder;
    int _spriteBorder;
    bool _pow2;
    int _maxTextureSize;
    float _scale;
    // polygon mode
    struct TPolygonMode{
        bool enable;
        float epsilon;
    } _polygonMode;

    // output data
    QImage _atlasImage;
    QMap<QString, SpriteFrameInfo> _spriteFrames;
    QMap<QString, QVector<QString>> _identicalFrames;
};

#endif // SPRITEATLAS_H
