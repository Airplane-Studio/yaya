#pragma once

#include "integer.hpp"

class Decimal {
private:
    Integer numerator;
    Integer denominator;
    int sign;
    int max_prec = 15;

    void reduce() {
        Integer g = Integer::gcd(numerator, denominator);
        numerator /= g;
        denominator /= g;
    }
public:
    Decimal(const UTF8String &string) {
        
    }
    Decimal(const Integer &numerator, const Integer &denominator = 1) {
        this->numerator = numerator.absolute();
        this->denominator = denominator.absolute();
        this->sign = numerator.sgn() * denominator.sgn();
        reduce();
    }

    UTF8String tostring() {
        Integer tenpow_maxprec = Integer::pow(10, max_prec);
        Integer num = numerator.absolute();
        Integer deno = denominator.absolute();
        Integer res = num * tenpow_maxprec / deno;
        UTF8String str = res.tostring();
        while (str.size() <= max_prec) str = "0" + str;
        UTF8String res_str;
        for (int i = max_prec; i < str.size(); i++) {
            res_str += str[i - max_prec];
        }
        if (res_str.size() == 0) res_str += "0";
        res_str += ".";
        for (int i = str.size() - max_prec; i < str.size(); i++) {
            res_str += str[i];
        }
        int end_idx = res_str.size() - 1;
        for (; res_str[end_idx] == '0'; end_idx--);
        if (res_str[end_idx] == '.') end_idx--;
        end_idx++;
        UTF8String final_res;
        for (int i = 0; i < end_idx; i++) final_res += res_str[i];
        return final_res;
    }

    void output() {
        io.print(tostring());
    }

    void set_precision(int max_prec) {
        this->max_prec = max_prec;
    }
};
