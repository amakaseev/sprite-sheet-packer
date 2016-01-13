#ifndef PNGOPTIMIZER_H
#define PNGOPTIMIZER_H

#include <QtCore>
#include "opnglib/include/opnglib/opnglib.h"

class PngOptimizer {
public:
    PngOptimizer() {}
    ~PngOptimizer() {}
	
public:
    virtual bool optimizeFiles(QList<QString> fileNames) { return true; }
    virtual bool optimizeFile(const QString& fileName) { return true; }

    virtual bool setOptions(int optLevel) { return true; }
};

class OptiPngOptimizer : public PngOptimizer {

public:
    OptiPngOptimizer(int optLevel = 0);
    ~OptiPngOptimizer();

    bool optimizeFiles(QList<QString> fileNames) override;
    bool optimizeFile(const QString& fileName) override;

    bool setOptions(int optLevel);

private:
    int _optLevel;

    opng_options options;

    QAtomicPointer<opng_optimizer_t> optimizer;
    QAtomicPointer<opng_transformer_t> transformer;
};

#endif // PNGOPTIMIZER_H
