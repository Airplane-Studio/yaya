#pragma once

#include "dyn_array.hpp"
#include "printer.hpp"
#include "utf8.hpp"
#include "pair.hpp"

class Integer {
private:
    DynamicArray<uint8_t> decs;
    int sign; // +, -, 0
    bool is_int(const char *str, int len) {
        return is_int(UTF8String(str, len));
    }
    bool is_int(UTF8String str) {
        if (str.size() == 0) return false;
        if (str.size() == 1 && (str[0] == '+' || str[0] == '-')) return false;
        for (int i = (str[0] == '+' || str[0] == '-'); i < str.size(); i++) {
            if (str[i] < '0' || str[i] > '9') return false;
        }
        return true;
    }
    void remove_leading_zeros() {
        while (decs.size() && decs[decs.size() - 1] == 0) decs.remove(decs.size() - 1);
        if (!decs.size()) sign = 0;
    }
    int cmp_absolute(Integer &other) {
        if (decs.size() != other.decs.size()) return decs.size() - other.decs.size();
        for (int i = decs.size() - 1; i >= 0; i--) {
            if (decs[i] != other.decs[i]) return decs[i] - other.decs[i];
        }
        return 0;
    }
    int compare(Integer &other) {
        if (sign != other.sign) return sign - other.sign;
        return sign * cmp_absolute(other);
    }
    Pair<Integer, Integer> divmod(int other) {
        if (other == 0) return Pair(Integer(0), *this);
        Integer res = *this;
        if (other < 0) res.sign = 0 - sign, other = 0 - other;
        long long r = 0;
        for (int i = decs.size() - 1; i >= 0; i--) {
            r = r * 10 + res.decs[i];
            res.decs[i] = r / other;
            r %= other;
        }
        res.remove_leading_zeros();
        if (res.sign < 0) r = other - r, res--;
        return Pair(res, Integer(r));
    }
    Pair<Integer, Integer> divmod(Integer other) {
        if (other.sign == 0) return Pair(Integer(0), *this);
        if (cmp_absolute(other) < 0) {
            if (sign != other.sign) return Pair(Integer(-1), other + *this);
            return Pair(Integer(0), *this);
        }
        if (other.decs.size() <= 9) {
            return divmod(other.c_int());
        }
        Integer a = absolute(), b = other.absolute();
        Integer temp = 1, quot = 0;
        while (a.cmp_absolute(b) >= 0) {
            b *= 2;
            temp *= 2;
        }

        while (temp.sign > 0) {
            if (a.cmp_absolute(b) >= 0) {
                a -= b;
                quot += temp;
            }
            b /= 2;
            temp /= 2;
        }

        return Pair(sign == other.sign ? quot : --quot, sign == other.sign ? a : b - a);
    }
    void abs_inc() {
        decs.append(0);
        int i = 0;
        while (decs[i] == 9) i++;
        decs[i]++;
        while (i) decs[--i] = 0;
        remove_leading_zeros(); 
    }
    void abs_dec() {
        int i = 0;
        while (decs[i] == 0) i++;
        decs[i]--;
        while (i) decs[--i] = 9;
        remove_leading_zeros(); 
    }
public:
    Integer(long long num = 0) {
        decs.clear();
        if (num == 0) sign = 0;
        else {
            if (num < 0) sign = -1, num = 0 - num;
            else sign = 1;
            while (num) {
                decs.append(num % 10);
                num /= 10;
            }
            remove_leading_zeros();
        }        
    }
    Integer(int num) : Integer((long long) num) {}
    Integer(const char *str) {
        decs.clear();
        int len = strlen(str);
        if (is_int(str, len)) {
            for (int i = len - 1; i >= (str[0] == '+' || str[0] == '-'); i--) {
                decs.append(str[i] - '0');
            }
            if (str[0] == '-') sign = -1;
            else sign = 1;
            remove_leading_zeros();
        }
    }
    Integer(UTF8String &str) {
        decs.clear();
        int len = str.size();
        if (is_int(str)) {
            for (int i = len - 1; i >= (str[0] == '+' || str[0] == '-'); i--) {
                decs.append(str[i] - '0');
            }
            if (str[0] == '-') sign = -1;
            else sign = 1;
            remove_leading_zeros();
        }
    }

    Integer operator+(Integer other) {
        if (sign == 0) return other;
        if (other.sign == 0) return *this;
        if (sign == 1 && other.sign == -1) {
            return *this - (-other);
        }
        if (sign == -1 && other.sign == 1) {
            return other - (-*this);
        }
        Integer res;
        int carry = 0;
        int len = max(decs.size(), other.decs.size());
        for (int i = 0; i < len; i++) {
            int ai = i < decs.size() ? decs[i] : 0;
            int bi = i < other.decs.size() ? other.decs[i] : 0;
            int ci = ai + bi + carry;
            if (ci >= 10) ci -= 10, carry = 1;
            else carry = 0;
            res.decs.append(ci);
        }
        if (carry) res.decs.append(carry);
        res.remove_leading_zeros();
        res.sign = sign;
        return res;
    }

    Integer operator+=(Integer other) {
        return (*this) = (*this) + other;
    }

    Integer operator-(Integer other) {
        if (sign == 0) return -other;
        if (other.sign == 0) return *this;
        if (sign == 1 && other.sign == -1) {
            // a - (-b)
            return *this + (-other);
        }
        if (sign == -1 && other.sign == 1) {
            // -a - b
            return -(-(*this) + other);
        }
        if (cmp_absolute(other) < 0) {
            return -(other - (*this));
        }
        // a - b && |a| > |b|
        Integer res;
        int borrow = 0;
        for (int i = 0; i < decs.size(); i++) {
            int ai = decs[i];
            int bi = i < other.decs.size() ? other.decs[i] : 0;
            int ci = ai - bi - borrow;
            if (ci < 0) ci += 10, borrow = 1;
            else borrow = 0;
            res.decs.append(ci);
        }
        res.sign = sign;
        res.remove_leading_zeros();
        return res;
    }

    Integer operator-=(Integer other) {
        return (*this) = (*this) - other;
    }

    Integer operator-() {
        Integer res = *this;
        res.sign = -res.sign;
        return res;
    }

    Integer operator*(int other) {
        // high * low
        if (sign == 0) return *this;
        if (other == 0) return Integer();
        Integer res;
        if (other < 0) other = 0 - other, res.sign = 0 - sign;
        else res.sign = sign;
        long long carry = 0;
        for (int i = 0; i < decs.size(); i++) {
            long long ai = carry + (long long) other * decs[i];
            if (ai >= 10) {
                carry = ai / 10;
                ai %= 10;
            } else carry = 0;
            res.decs.append(ai);
        }
        if (carry) res.decs.append(carry);
        res.remove_leading_zeros();
        return res;
    }

    Integer operator*(Integer other) {
        if (sign == 0) return *this;
        if (other.sign == 0) return other;
        Integer res;
        for (int i = 0; i < decs.size() + other.decs.size(); i++) res.decs.append(0);
        for (int i = 0; i < decs.size(); i++) {
            for (int j = 0; j < other.decs.size(); j++) {
                res.decs[i + j] += decs[i] * other.decs[j];
            }
            for (int j = 0; j < res.decs.size() - 1; j++) {
                if (res.decs[j] >= 10) {
                    res.decs[j + 1] += res.decs[j] / 10;
                    res.decs[j] %= 10;
                }
            }
        }
        res.sign = sign == other.sign ? 1 : -1;
        res.remove_leading_zeros();
        return res;
    }

    Integer operator*=(int other) {
        return (*this) = (*this) * other;
    }

    Integer operator*=(Integer other) {
        return (*this) = (*this) * other;
    }

    Integer operator/(int other) {
        return divmod(other).first;
    }

    Integer operator%(int other) {
        return divmod(other).second;
    }

    Integer operator/=(int other) {
        return (*this) = (*this) / other;
    }
    Integer operator%=(int other) {
        return (*this) = (*this) % other;
    }

    Integer operator/(Integer &other) {
        return divmod(other).first;
    }

    Integer operator%(Integer &other) {
        return divmod(other).second;
    }

    Integer operator/=(Integer &other) {
        return (*this) = (*this) / other;
    }
    Integer operator%=(Integer &other) {
        return (*this) = (*this) % other;
    }

    bool operator>(Integer &other) { return compare(other) > 0; }
    bool operator<(Integer &other) { return compare(other) < 0; }
    bool operator>=(Integer &other) { return compare(other) >= 0; }
    bool operator<=(Integer &other) { return compare(other) <= 0; }
    bool operator==(Integer &other) { return compare(other) == 0; }
    bool operator!=(Integer &other) { return compare(other) != 0; }

    Integer &operator++() {
        if (sign > 0) abs_inc();
        else if (sign < 0) abs_dec();
        else {
            sign = 1;
            decs.append(1);
        }
        return *this;
    }

    Integer &operator--() {
        if (sign < 0) abs_inc();
        else if (sign > 0) abs_dec();
        else {
            sign = -1;
            decs.append(1);
        }
        return *this;
    }

    Integer operator++(int) {
        Integer a = *this;
        ++(*this);
        return a;
    }

    Integer operator--(int) {
        Integer a = *this;
        --(*this);
        return a;
    }

    Integer absolute() const {
        Integer res = *this;
        if (res.sign < 0) res.sign = 0 - res.sign;
        return res;
    }

    bool is_zero() const { return sign == 0; }
    operator bool() const { return is_zero(); }

    int c_int() const {
        int res = 0;
        for (int i = decs.size() - 1; i >= 0; i--) {
            res = res * 10 + decs[i];
        }
        return res;
    }

    static Integer gcd(Integer a, Integer b) {
        if (b.sign == 0) return a;
        return gcd(b, a % b);
    }

    static Integer pow(Integer base, Integer power) {
        if (power.sign < 0) return 0;
        if (power.sign == 0) return 1;
        if (base.decs.size() == 1 && base.decs[0] == 1) {
            return power.decs[0] % 2 && base.sign == -1 ? -base : base;
        }
        if (base.sign == 0) return base;
        if (base == 10) {
            Integer res;
            res.sign = 1;
            while (power--) res.decs.append(0);
            res.decs.append(1);
            return res;
        }

        Integer ans = 1;
        while (power.sign != 0) {
            if (power.decs[0] % 2) ans *= base;
            base *= base;
            power /= 2;
        }
        return ans;
    }

    UTF8String tostring() {
        UTF8String res;
        if (sign == 0) {
            res = "0";
            return res;
        }
        if (sign < 0) res += "-";
        for (int i = decs.size() - 1; i >= 0; i--) {
            res += decs[i] + '0';
        }
        return res;
    }

    int sgn() const { return sign; }

    void output() {
        if (sign == 0) {
            io.print('0');
            return;
        }
        if (sign == -1) io.print("-");
        for (int i = decs.size() - 1; i >= 0; i--) {
            io.print((int) decs[i]);
        }
    }
};
