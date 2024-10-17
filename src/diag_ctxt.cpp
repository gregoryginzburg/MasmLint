#include "diag_ctxt.h"
#include "emitter.h"

DiagCtxt::DiagCtxt(std::shared_ptr<Emitter> emitter)
    : emitter(std::move(emitter))
{
}

void DiagCtxt::addDiagnostic(const Diagnostic &diag)
{
    diagnostics.push_back(diag);
}

bool DiagCtxt::hasErrors() const
{
    for (const auto &diag : diagnostics) {
        if (diag.getLevel() == Diagnostic::Level::Error) {
            return true;
        }
    }
    return false;
}

void DiagCtxt::emitDiagnostics()
{
    for (const auto &diag : diagnostics) {
        emitter->emit(diag);
    }
}