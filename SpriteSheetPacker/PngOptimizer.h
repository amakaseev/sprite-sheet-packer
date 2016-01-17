#ifndef PNGOPTIMIZER_H
#define PNGOPTIMIZER_H

#include <QtCore>
#include "opnglib.h"

class PngOptimizer {
public:
    PngOptimizer() {}
    ~PngOptimizer() {}
	
public:
    virtual bool optimizeFiles(QStringList fileNames) { return true; }
    virtual bool optimizeFile(const QString& fileName) { return true; }

    virtual bool setOptions(int optLevel) { return true; }
};

class OptiPngOptimizer : public PngOptimizer {

public:
    OptiPngOptimizer(int optLevel = 0);
    ~OptiPngOptimizer();

    bool optimizeFiles(QStringList fileNames) override;
    bool optimizeFile(const QString& fileName) override;

    bool setOptions(int optLevel);

private:
    int _optLevel;

    opng_options options;
    opng_optimizer_t* optimizer;
    opng_transformer_t* transformer;
};

#endif // PNGOPTIMIZER_H
