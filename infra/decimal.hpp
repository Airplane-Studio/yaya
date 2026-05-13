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
        numerator = 0;
        denominator = 1;
        sign = 1;
        if (string.size() == 0 || (string.size() == 1 && (string[0] == '+' || string[0] == '-')));
        else {
            int idx = 0;
            if (string[0] == '+' || string[0] == '-') {
                sign = 2 * (string[0] == '+') - 1;
                idx++;
            }
            if (idx < string.size() && (string[idx] >= '0' && string[idx] <= '9')) {
                while (idx < string.size() && (string[idx] >= '0' && string[idx] <= '9')) {
                    numerator = numerator * 10 + (string[idx] - '0');
                    idx++;
                }
            }
            if (idx < string.size() && string[idx] == '.') {
                // .xxexx
                int dec_len = 0;
                idx++;
                while (idx < string.size() && string[idx] >= '0' && string[idx] <= '9') {
                    numerator = numerator * 10 + (string[idx] - '0');
                    idx++;
                    dec_len++;
                }
                denominator = Integer::pow(10, dec_len);
            }
            if (idx < string.size() && string[idx] == 'e') {
                idx++;
                Integer exp_part = 0;
                int exp_sign = 1;
                if (idx < string.size() && (string[idx] == '+' || string[idx] == '-')) {
                    exp_sign = 2 * (string[idx] == '+') - 1;
                    idx++;
                }
                while (idx < string.size() && string[idx] >= '0' && string[idx] <= '9') {
                    exp_part = exp_part * 10 + (string[idx] - '0');
                    idx++;
                }
                if (exp_sign == 1) numerator *= Integer::pow(10, exp_part);
                else denominator *= Integer::pow(10, exp_part);
            }
        }
        reduce();
    }
    Decimal(const Integer &numerator, const Integer &denominator = 1) {
        this->numerator = numerator.absolute();
        this->denominator = denominator.absolute();
        this->sign = numerator.sgn() * denominator.sgn();
        reduce();
    }

    Decimal operator+(Decimal &other) {
        return Decimal(numerator * other.denominator + denominator * other.numerator, denominator * other.denominator * sign);
    }
    Decimal operator-(Decimal &other) {
        return Decimal(numerator * other.denominator - denominator * other.numerator, denominator * other.denominator * sign);
    }
    Decimal operator*(const Decimal &other) const {
        return Decimal(numerator * other.numerator, denominator * other.denominator * sign);
    }
    Decimal operator/(Decimal &other) {
        return Decimal(numerator * other.denominator, denominator * other.numerator * sign);
    }

    bool exact_eq(const Decimal &other) const {
        return numerator == other.numerator && denominator == other.denominator && sign == other.sign;
    }
    bool exact_lt(const Decimal &other) const {
        return numerator * other.denominator < denominator * other.numerator;
    }
    bool exact_gt(const Decimal &other) const {
        return numerator * other.denominator > denominator * other.numerator;
    }

    bool operator==(const Decimal &other) const {
        Decimal d = (other - *this).absolute();
        Decimal prec = Decimal(1, Integer::pow(10, max_prec));
        return d.exact_lt(prec);
    }
    bool operator!=(const Decimal &other) const {
        Decimal d = (other - *this).absolute();
        Decimal prec = Decimal(1, Integer::pow(10, max_prec));
        return !d.exact_lt(prec);
    }
    bool operator<(const Decimal &other) const {
        Decimal d = other - *this;
        Decimal prec = Decimal(1, Integer::pow(10, max_prec));
        return d.exact_gt(prec);
    }
    bool operator>(const Decimal &other) const {
        Decimal d = *this - other;
        Decimal prec = Decimal(1, Integer::pow(10, max_prec));
        return d.exact_gt(prec);
    }
    bool operator<=(const Decimal &other) const {
        return *this < other || *this == other;
    }
    bool operator>=(const Decimal &other) const {
        return *this > other || *this == other;
    }

    Decimal round(int prec) {
        Decimal d = *this;
        d.set_precision(prec);
        return d;
    }

    Decimal absolute() {
        Decimal d = *this;
        d.sign = d.sign > 0 ? d.sign : 0 - d.sign;
        return d;
    }

    Pair<Integer, Integer> as_integer_ratio() {
        return Pair(numerator, denominator);
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
