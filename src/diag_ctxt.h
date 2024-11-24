#pragma once

#include <vector>
#include <memory>

#include "emitter.h"

class DiagCtxt {
public:
    explicit DiagCtxt(std::shared_ptr<Emitter> emitter);

    void addDiagnostic(const Diagnostic &diag);
    std::shared_ptr<Diagnostic> getLastDiagnostic();
    bool hasErrors() const;
    void emitDiagnostics();
    void emitJsonDiagnostics();

private:
    std::vector<std::shared_ptr<Diagnostic>> diagnostics;
    std::shared_ptr<Emitter> emitter;
};