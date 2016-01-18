#ifndef PNGOPTIMIZER_H
#define PNGOPTIMIZER_H

#include <QtCore>
#include "opnglib.h"
#include "libimagequant.h"

class PngOptimizer {
public:
    PngOptimizer() {}
    ~PngOptimizer() {}
	
public:
    virtual bool optimizeFiles(const QStringList& fileNames) { return true; }
    virtual bool optimizeFile(const QString& fileName) { return true; }

    virtual bool setOptions(int optLevel) { return true; }
};

class OptiPngOptimizer : public PngOptimizer {

public:
    OptiPngOptimizer(int optLevel = 0);
    ~OptiPngOptimizer();

    bool optimizeFiles(const QStringList& fileNames) override;
    bool optimizeFile(const QString& fileName) override;

    bool setOptions(int optLevel) override;

private:
    int _optLevel;

    opng_options options;
    opng_optimizer_t* optimizer;
    opng_transformer_t* transformer;
};

class PngQuantOptimizer : public PngOptimizer {

public:
    PngQuantOptimizer(int optLevel = 0);
    ~PngQuantOptimizer();

    bool optimizeFiles(const QStringList& fileNames) override;
    bool optimizeFile(const QString& fileName) override;

    bool setOptions(int optLevel) override;

private:
    int _optLevel;

    liq_attr* attr;
    liq_image* image;
    liq_result* res;
};

#endif // PNGOPTIMIZER_H
