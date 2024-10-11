#include "context.h"

std::string Context::currentFileName;
int Context::currentLineNumber;
std::stack<std::unique_ptr<InputSource>> Context::inputStack;

void Context::init(const std::string &filename)
{
    bool success;
    auto fileInput = std::make_unique<FileInputSource>(filename, success);
    if (success) {
        pushInputStack(std::move(fileInput));
    } else {
        ErrorReporter::reportError("Failed to open file '" + filename + "'", 0, 0, "", filename);
    }
}

void Context::pushInputStack(std::unique_ptr<InputSource> inputSrc)
{
    inputStack.push(std::move(inputSrc));
    FileInputSource *fileInputSourcePtr = dynamic_cast<FileInputSource *>(inputSrc.get());
    if (fileInputSourcePtr) {
        currentFileName = fileInputSourcePtr->getSourceName();
    }
}

std::unique_ptr<InputSource> &Context::topInputStack() { return inputStack.top(); }

void Context::popInputStack()
{
    inputStack.pop();
    if (!inputStack.empty()) {
        InputSource *InputSourcePtr = topInputStack().get();
        FileInputSource *fileInputSourcePtr = dynamic_cast<FileInputSource *>(InputSourcePtr);
        if (fileInputSourcePtr) {
            currentFileName = fileInputSourcePtr->getSourceName();
        }
    } else {
        currentFileName = "";
    }
}

bool Context::emptyInputStack() { return inputStack.empty(); }

int Context::getLineNumber() { return currentLineNumber; }

std::string Context::getFileName() { return currentFileName; }
