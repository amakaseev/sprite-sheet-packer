#ifndef PNGOPTIMIZER_H
#define PNGOPTIMIZER_H

#include <QtCore>
#include "opnglib/include/opnglib/opnglib.h"

class PngOptimizer {
public:
    PngOptimizer() {}
    ~PngOptimizer() {}
	
protected:
    virtual bool optimize() { return true; }
};

class OptiPngOptimizer : public PngOptimizer {

public:
    OptiPngOptimizer(const QString& fileName, int optLevel);
    ~OptiPngOptimizer();

    bool optimize() override;
private:
    QString _fileName;
    int _optLevel;
    opng_options options;
    opng_optimizer_t* optimizer;
    opng_transformer_t* transformer;
};

#endif // PNGOPTIMIZER_H
