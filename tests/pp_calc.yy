%define EMPTY0()
%define EMPTY1() EMPTY0 EMPTY0() ()
%define EMPTY() EMPTY1 EMPTY1() ()

%define DEFER(id) id EMPTY()
%define EXPAND(...) __VA_ARGS__

%define INC(x) INC_$$x

%define INC_0 1
%define INC_1 2
%define INC_2 3
%define INC_3 4
%define INC_4 5
%define INC_5 6
%define INC_6 7
%define INC_7 8
%define INC_8 9

%define DEC(x) DEC_$$x

%define DEC_0 0
%define DEC_1 0
%define DEC_2 1
%define DEC_3 2
%define DEC_4 3
%define DEC_5 4
%define DEC_6 5
%define DEC_7 6
%define DEC_8 7
%define DEC_9 8

%define NOT(x) NOT_$$x

%define NOT_0 1
%define NOT_1 0

%define CAT(a, ...) a $$ __VA_ARGS__

%define IF(cond, if_true, if_false) CAT(IF_, cond) (if_true, if_false)

%define IF_0(x, y) y
%define IF_1(x, y) x

%define CHECK_N(x, n, ...) n
%define CHECK(...) CHECK_N(__VA_ARGS__, 0,)
%define IS_ZERO(x) CHECK(CAT(IS_ZERO_, x))

%define IS_ZERO_0 ~ , 1

%define EVAL0(...) __VA_ARGS__
%define EVAL1(...) EVAL0 (EVAL0 (EVAL0 (__VA_ARGS__)))
%define EVAL2(...) EVAL1 (EVAL1 (EVAL1 (__VA_ARGS__)))
%define EVAL3(...) EVAL2 (EVAL2 (EVAL2 (__VA_ARGS__)))
%define EVAL4(...) EVAL3 (EVAL3 (EVAL3 (__VA_ARGS__)))
%define EVAL(...)  EVAL4 (EVAL4 (EVAL4 (__VA_ARGS__)))

%define ADD_(x, y) IF(IS_ZERO(x), y, DEFER(ADD_BACK_)  (DEC(x), INC(y)))
%define ADD_BACK_(x,y) IF(IS_ZERO(x), y, DEFER(ADD_) (DEC(x), INC(y)))

%define ADD(x, y) EVAL(ADD_(x, y))

%define TO_STR_(x) $x
%define TO_STR(x) TO_STR_(x)

ADD(1, 2)
ADD(3, 2)
ADD(3, 4)