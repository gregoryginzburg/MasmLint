#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>

class InputSource {
public:
    virtual ~InputSource() = default;
    virtual bool getNextLine(std::string& line) = 0;
    virtual int getCurrentLineNumber() const = 0;
    virtual std::string getSourceName() const = 0;
};

class FileInputSource : public InputSource {
public:
    explicit FileInputSource(const std::string& filename, bool& success);
    bool getNextLine(std::string& line) override;
    int getCurrentLineNumber() const override;
    std::string getSourceName() const override;

private:
    std::ifstream fileStream;
    int lineNumber;
    std::string filename;
};

class MacroInputSource : public InputSource {
public:
    MacroInputSource(const std::string& macroName);
    bool getNextLine(std::string& line) override;
    int getCurrentLineNumber() const override;
    std::string getSourceName() const override;

private:
    int currentLineIndex;
    std::string macroName;
};


