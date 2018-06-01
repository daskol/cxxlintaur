/**
 *  \file check_names.cpp
 */

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <queue>
#include <utility>

#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory name_checker_category("check-names options");
static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp more_help(
R"(This tool helps the detection function, variable or class declaration which
do not satisfy Code Style Guide.

https://google.github.io/styleguide/cppguide.html#Naming
)");

enum class Entity {
    kVariable,
    kField,
    kType,
    kConst,
    kFunction
};

struct BadNameEntry {
    ::Entity entity;
    std::string name;
    std::string filename;
    int line;
};

struct MistakeEntry {
    std::string name;
    std::string wrong_word;
    std::string ok_word;
    std::string filename;
    int line;
};

std::string Str(Entity entity) {
    switch (entity) {
    case Entity::kVariable: return "variable";
    case Entity::kField: return "field";
    case Entity::kType: return "type";
    case Entity::kConst: return "const";
    case Entity::kFunction: return "function";
    }
}

std::string GetFilename(const std::string& path) {
    size_t i = path.size() - 1;
    while (i < path.size() && path[i] != '/') {
        --i;
    }
    ++i;
    return path.substr(i);
}

void PrintStatistics(
    const std::string& filename,
    int bad_names,
    llvm::raw_ostream& os = llvm::outs())
{
    os << "===== Processed Stat " << GetFilename(filename) << " =====\n";
    os << "Bad names found: " << bad_names << "\n";
}

void BadName(
    Entity entity,
    const std::string& name, const std::string& filename,
    int line, llvm::raw_ostream& os = llvm::outs())
{
    os << "Entity's name \"" << name << "\" does not meet the requirements (" << Str(entity) << ")\n";
    os << "In " << GetFilename(filename) << " at line " << line << "\n\n";
}

class NameCheckerASTVisitor : public RecursiveASTVisitor<NameCheckerASTVisitor> {
public:
    explicit NameCheckerASTVisitor(ASTContext *context,
                                   std::queue<BadNameEntry> &badnames)
        : context_{context}
        , badnames_{badnames}
    {}


    bool VisitEnumConstantDecl(EnumConstantDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();

        if (!mgr.isInMainFile(loc) || loc.isMacroID()) {
            return true;
        }

        if (!CheckEnumConstantName(declaration->getName())) {
            ReportBadName(Entity::kConst, declaration);
        }

        return true;
    }

    bool VisitEnumDecl(EnumDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();

        if (!mgr.isInMainFile(loc) || loc.isMacroID()) {
            return true;
        }

        if (!CheckTypeName(declaration->getName())) {
            ReportBadName(Entity::kType, declaration);
        }

        return true;
    }

    bool VisitFieldDecl(FieldDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();

        if (!mgr.isInMainFile(loc) || loc.isMacroID()) {
            return true;
        }

        RecordDecl *record = declaration->getParent();
        std::string name = declaration->getName();
        bool is_class_member = record->isClass();
        bool is_const = declaration->getType().isConstQualified();

        if (is_const && is_class_member) {
            if (!CheckFieldName(name)) {
                ReportBadName(Entity::kConst, declaration);
            }
        } else if (is_class_member) {
            if (!CheckFieldName(name)) {
                ReportBadName(Entity::kField, declaration);
            }
        } else {
            if (!CheckVarName(name)) {
                ReportBadName(Entity::kVariable, declaration);
            }
        }

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();

        if (!mgr.isInMainFile(loc) || loc.isMacroID()) {
            return true;
        }

        auto *method = dyn_cast<CXXConstructorDecl>(declaration);

        if (method) {
            auto name = method->getNameInfo().getAsString();
            auto pos = name.find('<');

            if (pos != std::string::npos) {
                name = name.substr(0, pos);
            }

            if (!CheckFunctionName(name)) {
                ReportBadName(Entity::kFunction, method, name);
            }
        } else {
            std::string name = declaration->getName();

            if (!CheckFunctionName(name)) {
                ReportBadName(Entity::kFunction, declaration);
            }
        }

        return true;
    }

    bool VisitRecordDecl(RecordDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();

        if (!mgr.isInMainFile(loc) || loc.isMacroID()) {
            return true;
        }

        std::string name = declaration->getName();

        if (!CheckTypeName(declaration->getName())) {
            ReportBadName(Entity::kType, declaration);
        }

        return true;
    }

    bool VisitTypedefNameDecl(TypeDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();

        if (!mgr.isInMainFile(loc) || loc.isMacroID()) {
            return true;
        }

        std::string name = declaration->getName();

        if (!CheckTypeName(name)) {
            ReportBadName(Entity::kType, declaration);
        }

        return true;
    }

    bool VisitVarDecl(VarDecl *declaration) {
        auto &mgr = context_->getSourceManager();
        auto loc = declaration->getLocation();
        auto full_loc = context_->getFullLoc(loc);

        if (!mgr.isInMainFile(loc)) {
            return true;
        }

        if (full_loc.isMacroID()) {
            return true;
        }

        auto tag = clang::dyn_cast<TagDecl>(declaration->getDeclContext());

        bool is_const = declaration->getType().isConstQualified()
            || declaration->isConstexpr();
        bool is_in_class = tag ? tag->isClass() : false;
        bool is_in_struct = tag ? tag->isStruct() : false;

        if (is_const && is_in_class) {
            std::string name = declaration->getName();

            if (!CheckConstantName(name)) {
                ReportBadName(Entity::kConst, declaration);
            }
        } else if (is_in_class) {
            std::string name = declaration->getName();

            if (!CheckFieldName(declaration->getName())) {
                ReportBadName(Entity::kField, declaration);
            }
        } else if (is_const && (!is_in_struct || !is_in_class)) {
            std::string name = declaration->getName();

            if (!CheckConstantName(declaration->getName())) {
                ReportBadName(Entity::kConst, declaration);
            }
        } else {
            std::string name = declaration->getName();

            if (!CheckVarName(declaration->getName())) {
                ReportBadName(Entity::kVariable, declaration);
            }
        }

        return true;
    }

private:
    ASTContext *context_;
    std::queue<BadNameEntry> &badnames_;

private:
    void ReportBadName(Entity entity, NamedDecl *declaration) {
        auto loc = declaration->getLocStart();
        auto &mgr = context_->getSourceManager();
        auto full_loc = context_->getFullLoc(loc);
        badnames_.push({entity,
                        declaration->getName().str(),
                        mgr.getFilename(loc).str(),
                        static_cast<int>(full_loc.getSpellingLineNumber())});
    }

    void ReportBadName(Entity entity, CXXConstructorDecl *declaration, std::string method_name) {
        auto loc = declaration->getLocStart();
        auto &mgr = context_->getSourceManager();
        auto full_loc = context_->getFullLoc(loc);
        badnames_.push({entity,
                        method_name,
                        mgr.getFilename(loc).str(),
                        static_cast<int>(full_loc.getSpellingLineNumber())});
    }

    bool IsCamelCase(const StringRef &name) {
        for (char c : name) {
            if (c == '_') {
                return false;
            }
        }
        return true;
    }

    bool IsCapitalized(const StringRef &name) {
        return name[0] == std::toupper(name[0]);
    }

    bool IsConstant(const StringRef &name) {
        return name[0] == 'k';
    }

    bool IsField(const StringRef &name) {
        return name.back() == '_';
    }

    bool IsSnakeCase(const StringRef &name) {
        if (name.front() == '_') {
            return false;
        }

        int length = 0;

        for (char c : name) {
            if (c != std::tolower(c) || std::isdigit(c)) {
                return false;
            }

            length = c == '_' ? length + 1 : 0;

            if (length > 1) {
                return false;
            }
        }

        return true;
    }

    bool IsUpperCase(const StringRef &name) {
        for (char c : name) {
            if (c != std::toupper(c)) {
                return false;
            }
        }
        return true;
    }

    bool IsUpperRule(const StringRef &name) {
        int length = 0;
        for (char c : name) {
            if (c == std::toupper(c) && !std::isdigit(c)) {
                ++length;
            } else if (length > 1 && length < 3) {
                return false;
            } else {
                length = 0;
            }
        }

        if (length > 0 && length < 3) {
            return false;
        }

        return length != name.size();
    }

    bool CheckTypeName(const StringRef &name) {
        return name.empty()
            || (IsCapitalized(name) && IsCamelCase(name) && IsUpperRule(name));
    }

    bool CheckFunctionName(const StringRef &name) {
        if (name == "main") {
            return true;
        }
        return name.empty()
            || (IsCapitalized(name) && IsCamelCase(name) && IsUpperRule(name));
    }

    bool CheckConstantName(const StringRef &name) {
        return name.empty() || (IsConstant(name) && IsCamelCase(name));
    }

    bool CheckEnumConstantName(const StringRef &name) {
        return name.empty()
            || ((IsConstant(name) && IsCamelCase(name)) || IsUpperCase(name));
    }

    bool CheckVarName(const StringRef &name) {
        return name.empty() || (!IsField(name) && IsSnakeCase(name));
    }

    bool CheckFieldName(const StringRef &name) {
        return name.empty() || (IsField(name) && IsSnakeCase(name));
    }
};

class NameCheckerASTConsumer : public ASTConsumer {
public:
    NameCheckerASTConsumer(ASTContext *context,
                           std::queue<BadNameEntry> &badnames)
        : visitor_(context, badnames)
    {}

    void HandleTranslationUnit(ASTContext &context) override {
        auto *unit = context.getTranslationUnitDecl();
        visitor_.TraverseDecl(unit);
    }

private:
    NameCheckerASTVisitor visitor_;
};

class NameCheckerFrontendAction : public ASTFrontendAction {
public:
    NameCheckerFrontendAction(void) = default;

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci,
                                                   StringRef file) override {
        filename_ = file.str();
        return llvm::make_unique<NameCheckerASTConsumer>(&ci.getASTContext(),
                                                         badnames_);
    }

    void EndSourceFileAction() override {
        size_t nobadnames = badnames_.size();

        while (!badnames_.empty()) {
            auto &el = badnames_.front();
            ::BadName(el.entity, el.name, el.filename, el.line);
            badnames_.pop();
        }

        ::PrintStatistics(filename_, nobadnames);
    }

private:
    std::string filename_;
    std::queue<BadNameEntry> badnames_;
};

int main(int argc, const char *argv[]) {
    CommonOptionsParser options(argc, argv, name_checker_category);
    ClangTool tool(options.getCompilations(), options.getSourcePathList());
    return tool.run(newFrontendActionFactory<NameCheckerFrontendAction>().get());
}
