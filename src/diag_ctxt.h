#pragma once

#include <vector>
#include <memory>

#include "emitter.h"

class DiagCtxt {
public:
    DiagCtxt(std::shared_ptr<Emitter> emitter);

    void addDiagnostic(const Diagnostic &diag);
    bool hasErrors() const;
    void emitDiagnostics();
    void emitJsonDiagnostics();

private:
    std::vector<Diagnostic> diagnostics;
    std::shared_ptr<Emitter> emitter;
};