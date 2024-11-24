#include "diag_ctxt.h"
#include "emitter.h"
#include "log.h"

DiagCtxt::DiagCtxt(std::shared_ptr<Emitter> emitter) : emitter(std::move(emitter)) {}

void DiagCtxt::addDiagnostic(const Diagnostic &diag) { diagnostics.push_back(std::make_shared<Diagnostic>(diag)); }

std::shared_ptr<Diagnostic> DiagCtxt::getLastDiagnostic()
{
    if (diagnostics.empty()) {
        LOG_DETAILED_ERROR("No last diagnostics exists!");
        return nullptr;
    }
    return diagnostics.back();
}

bool DiagCtxt::hasErrors() const { return !diagnostics.empty(); }

void DiagCtxt::emitDiagnostics()
{
    for (const auto &diag : diagnostics) {
        emitter->emit(diag);
    }
}

void DiagCtxt::emitJsonDiagnostics() { emitter->emitJSON(diagnostics); }