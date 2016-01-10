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
    static void app_printf(const char *fmt, ...);
    static void app_print_cntrl(int cntrl_code);
    static void app_progress(unsigned long current_step, unsigned long total_steps);
    static void panic(const char *msg);

    QString _fileName;
    int _optLevel;
    opng_optimizer_t* optimizer;
    opng_transformer_t* transformer;
};

#endif // PNGOPTIMIZER_H
