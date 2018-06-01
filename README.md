# C++lintaur

*C++LinTaur is a toy linter that checks name for compliance to Google Style Guide*

## Overview

This is another one toy tool based on inimitable
[LLVM](https://github.com/llvm-mirror/llvm) and
[CLang](https://github.com/llvm-mirror/clang). In my project
RecursiveASTVisitor is implemented.  It walks through AST and looking for name
declarations(which are represented with classes NameDecl, TypeDecl,
FunctionDecl, etc). When linter meets an AST node of the type, it checks name
of the Node for compliance to
[Google Style Guide](https://google.github.io/styleguide/cppguide.html#Naming).
Surely, this tools does not support that convention completely. In order to
lint everething and to refactor whatever, one should look at
[clang-tidy](http://clang.llvm.org/extra/clang-tidy/) and
[clang-format](https://github.com/llvm-mirror/clang/tree/master/tools/clang-format).

Let's build the project. Run the following.

```bash
    mkdir -p build/release && cd build/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release
    cmake --build .
```

That is it. Now, it is possible to check names in source files. Run the linter
in the same directory and then the tool will generate report. As could be
expected, there is no any warning in [name-checker.cc](name-checker.cc) but
there are 26 in [example.cc](example.cc).

```bash
    $ ./c++lintaur -p `pwd` ../../example.cc ../../name-checker.cc
    Entity's name "Static_Method" does not meet the requirements (function)
    In example.cc at line 17

    Entity's name "bad_bield" does not meet the requirements (field)
    In example.cc at line 23

    ..............................................................


    Entity's name "fst_var_" does not meet the requirements (variable)
    In example.cc at line 88

    Entity's name "snd_var_" does not meet the requirements (variable)
    In example.cc at line 89

    ===== Processed Stat example.cc =====
    Bad names found: 26
    ===== Processed Stat name-checker.cc =====
    Bad names found: 0
```

The project has natural requirements which are Clang & LLVM not older then
4.0.0.
