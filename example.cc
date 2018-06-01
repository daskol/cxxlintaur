/**
 *  \file example.cc
 */

#include <iostream>
#include <vector>
#include <string>

class Test {
public:
    Test() = default;

    int Method() const {
        return 0;
    }

    static const double Static_Method() {
        return 0;
    }

private:
    int field_;
    double bad_bield;

    static const int kGood = 5;
    static const int kBad_ = 3;
    static int bad_val;

    void privateMethod(void) {}
};

int Test::bad_val = 0;

struct JustAStruct {
    int ok_field;
    int bad_field_;
    const int kGood = 3;

    size_t size() const {
        return 0;
    }
};

std::vector<int> BuildDSU(size_t size) {
    return {};
}

void Name2D() {
    static int numbers1;
}

constexpr void fail() {}

std::string BIGNAME(int F) {
    return "Wow Such big";
}

typedef int Dash;
using shitHappens = int;

std::string DeeeeeepThought(bool underscore_) {
    return "Answer to the Ultimate Question of Life, The Universe, and Everything is ...";
}

int F(int code) {
    return 42;
}

enum class Ok {
    ABC = 0,
};

int main() {
    constexpr double kValue = 3.1415926;

    int just__few_words = 3;
    double _hello_world = 42;

    int _ = just__few_words;
    char Wrong = '?';

    std::string CAPS_IS_NOT_PERMITTED;

    struct {
        short stuff;

        union unnamed_again {
            int fst_var_;
            int snd_var_;
        };
    } unnamed_struct;

    return F(0);
}
