#include "input.h"

// File input
FileInputSource::FileInputSource(const std::string &filename, bool &success) : lineNumber(0), filename(filename)
{
    fileStream.open(filename);
    success = fileStream.is_open();
}

bool FileInputSource::getNextLine(std::string &line)
{
    if (std::getline(fileStream, line)) {
        ++lineNumber;
        return true;
    }
    return false;
}

int FileInputSource::getCurrentLineNumber() const { return lineNumber; }

std::string FileInputSource::getSourceName() const { return filename; }



// Macro Input

MacroInputSource::MacroInputSource(const std::string &macroName) : macroName(macroName), currentLineIndex(0) 
{
    // TODO: implement look up from table of macros (symbols)
}

bool MacroInputSource::getNextLine(std::string &line)
{
    // TODO: implement
    return false;
}

int MacroInputSource::getCurrentLineNumber() const { return currentLineIndex; }

std::string MacroInputSource::getSourceName() const { return "<macro:" + macroName + ">"; }
