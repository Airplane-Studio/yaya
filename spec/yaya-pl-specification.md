# Yaya 编程语言标准 v0.0

## 零、前言

Yaya 编程语言标准（下称本标准）规定了 Yaya 编程语言的基本词法、语法和语义。任何与本标准正文部分（不包括附录）相悖的 Yaya 编程语言实现，均视为**非法实现**，可能存在各种与标准和预期不符的行为，不建议使用。

目前处于 v0.0 版，极度不稳定，请您时刻关注本标准内容的变化。

## 一、基本信息

Yaya 是一门**面向对象**的**动态类型**脚本语言，支持顶层代码书写，以分号分割语句，大括号分割语句块。

Yaya 语言的核心要义是：怎么写起来顺手，就要怎么写。`Handiness Above All.`

Yaya 语言的源代码文件后缀名为 `.yy`，中间代码的后缀名可能包括 `.yyi`（预处理后文件）、`.yyt`（AST 文件）、`.yyc`（字节码文件）等。

## 二、注释

Yaya 语言的单行注释为 `#`，多行注释暂无。

## 三、基本语法

### 3.1 变量声明

Yaya 语言要求使用 `var` 关键字声明可变变量，`val` 关键字声明不可变变量如下：

```yaya
var a = 3; # 可变
val b = 4; # 不可变
```

对于可变变量，可以重新赋值，对于不可变变量则不能：

```yaya
var a = 3;
a = 4; # OK
val b = 4;
b = 5; # Error: `b` is immutable
b = b; # 即使这样也不行
```

同一作用域内重复声明同一个变量也将导致错误：

```yaya
var a = 3;
var a = 4; # Error: `a` has already been declared
```

`val` 声明的变量必须提供初始值。`var` 声明的变量如不提供初始值，则认为初始值为 `null`。

### 3.2 语句、语句块、控制流结构

语句以分号为结束标志。语句块使用一对大括号包裹，**不要求缩进**。例如，以下两个代码块含义相同：

```yaya
{
    a;
    b;
}
{a;
  b
;}
```

单独一个分号也是一种语句，称为**空语句**。

控制流结构包括 `if` 语句，`for` 语句，`while` 语句，匿名函数和函数声明。

`if` 语句的结构如下：

```yaya
if (<condition>) <statement>
else <statement>
```

此时 `<statement>` **不可以为变量声明**。

`if` 语句也可以表达式化，可以充当三目运算符：

```yaya
a = b if <condition> [else c];
```

当 `else c` 部分不存在时，默认添加 `else null`。当 `condition` 为真时，`a` 被赋值为 `b`，否则赋值为 `c`。

`while` 语句的结构如下：

```yaya
while (<condition>) <statement>
```

其结构与一般的 `while` 一致。

`for` 语句分为两种。一种是 `C-like for`，一种是 `range-based for`。`C-like for` 的语法如下：

```yaya
for (<init-statement>; <condition>; <updater>) <statement>
```

与 C 逻辑相同。

`range-based for` 语法如下：

```yaya
for (<declaration> from <container>) <statement>
```

`range-based for` 的变量声明部分**不能包括初始值**。例如，下面的代码是不允许的：

```yaya
for (var a = 3 from [1, 2, 3, 4]); # not allowed
```

匿名函数的语法如下：

```yaya
|<arguments>| <statement>;
```

这里的 `statement` 必须是表达式或是以表达式结尾的语句块（后者情况下，以表达式语句的值为返回值）。

普通函数的语法如下：

```yaya
func <name>(<arguments>) {
    <statement>
    <statement>
    ...
}
```

它实质是表达式化的函数声明的语法糖：

```yaya
var <name> = func (<arguments>) {
    <statement>
    <statement>
    ...
};
```

### 3.3 类

Yaya 语言的类声明如下：

```yaya
class ClassName {
    member = 0; # 所有实例默认自带的成员，无需 var 声明
    func init() {
        # 构造函数
    }
    func method() {
        # 正常成员方法
    }
    static func staticmethod() {
        # 静态方法
    }
};
```

方法没有 public 与 private 之分，一切方法均可以被自由调用。

Yaya 语言不支持多重继承，继承一个类需使用 `extends` 关键字：

```yaya
class Base {}
class Sub extends Base {
    # 继承 Base 的所有属性和方法
}
```

若不想让一个类中的方法被继承，可以使用 `final` 修饰在方法之前（属性不能被修饰）。若不想让一个类被继承，可在类声明前添加 `final`。如果不能确定一个方法是否被继承，可在方法前添加 `override`，标注“我要继承祖先类中的对应方法”。若继承树中没有对应方法将会报错。

可以使用 `interface` 替换 `class` 来声明一个接口，接口不能被实例化，没有属性。一个类需用 `implements` 关键字来实现接口，若一个 `class` 标记为实现一个接口，则必须对接口内定义的所有成员函数提供实现。

可以在 `class` 前添加 `abstract` 声明为抽象基类，抽象基类可以有属性，可以有零个或多个抽象方法，抽象方法需在方法前添加 `abstract` 关键字。抽象基类不能被实例化，即使没有抽象方法也如此。

类的实例化通过调用类名来实现。例如，`var a = A();` 创建一个 `A` 类的实例。

所有类都是 `class` 类型的对象。

类属性可以任意添加，但是类方法在类创建完后即保持固定。例如，下列代码：

```yaya
class A {}
var a = A();
a.test = 1;
```

是正确的；与此同时，下列代码：

```yaya
class A {}
func method() {
    print(this.name);
}

A.method = method;
var a = A();
a.name = "Hello";
a.method();
```

就是错误的。

### 3.4 类型标注

类型标注是可选项。Yaya 语言中自带类型包括 `int32`、`int`、`float`、`bool`、`string`、`list`、`map`、`function`、`class` 以及内置异常类型。除异常类型外，所有内置类型都是 `final` 的。

类型标注可添加在变量声明、函数声明或方法声明后，分别如下：

```yaya
var x: int32 = 2147483647;
func add(x: int, y: int) -> int {
    return x + y;
}
```

方法中的类型标注与函数或变量中一致。返回类型可以不标注，参数的类型可以只标注一部分。

在添加类型标注后，可通过运行时为 `yaya` 可执行程序提供选项来确认是否打开运行时类型检查。在添加运行时类型检查后，以下代码将在运行时报错：

```yaya
var x: int32 = 2147483647;
x = 2147483648; # Error: out of int32 range
x = "Hello"; # Error: "Hello" is not an instance of type int32

func add(x: int, y: int) -> int {
    return x + y;
}

add(2, "hello"); # Error: "hello" is not an instance of type int
```

对于用户自定义类型，也可以使用类型标注。运行时类型检查应该考虑到继承：

```yaya
class Base {}
class Sub extends Base {}

func foo(x: Base) {}
foo(Sub()); # OK
```

特别注意的是，`float` 与 `int` **没有继承关系**，`float` 与 `int32` 也没有。也即下面的代码不能通过运行时检查：

```yaya
func add(a: float, b: float) {
    return a + b;
}
add(1, 3); # Illegal
add(1i32, 3i32); # Illegal
```

`null` 没有类型，也不能通过任何运行时类型检查。

类型标注如遇复杂结构（如列表、映射等），则可以限制其内部元素类型。

对于列表，应当类似这样：`list[int, str, int, ...]`，其意为要求一个第 1 个元素为 str、其余元素全为 int、长度不定的列表。`...` 表示后续有任意数量元素，类型同 `...` 前的最后一个。若真的想表示任意，可用 `list[object, ...]` 或单走一个 `list`。

对于映射，由于映射是无序数据结构，所以只能 `map[K, V]` 分别约束键类型和值类型。

运行时类型检查也可以手动进行，参见标准库规范中的 `typeof` 函数部分。

显然运行时类型检查将带来大量开销，不建议用户手动开启，仅将类型标注作为辅助阅读代码的手段。

### 3.5 字面量、表达式

Yaya 语言共有以下几种字面量：

**整数字面量**：即普通整数。支持十六进制 `0x`、八进制 `0o` 和二进制 `0b`，在数字后可跟 `i32` 表示 `int32` 变体。**`int32` 与 `int` 没有继承关系，前者是 32 位整数，后者是任意精度整数**。若 `int32` 字面量前的整数不在 `int32` 范围内，将在运行时报错。例如，`2147483648i32` 是不允许的。

**浮点数字面量**：即普通浮点数。支持科学计数法。

**字符串字面量**：顾名思义。支持的转义包括 `\n`、`\r`、`\t`、`\f`、`\x` 系列和 `\u` 系列。不支持多行字符串。

**列表字面量**：`[1, 2, 3]` 等。两边的中括号是必需的。

**映射字面量**：`{a: b, c: d, e: f, ...}`。两边的大括号是必需的。

**内置常量**：`true`、`false` 和 `null`。

**特殊字面量**：`this` 和 `super`。`this` 表示当前类实例。`super` 除传统 OOP 中的意思之外，还可以表示词法作用域的上层。

Yaya 语言的运算符及其优先级罗列如下，按优先级从高到低排列，同一行表示优先级相同：

`-` `not` 单目取负运算符 单目逻辑非运算符/按位反（取决于两边操作数的类型）

`.` 取成员运算符

`^` 求幂运算符

`*` `/` `%` 乘、除、余运算符

`+` `-` 加、减运算符

`<<` `>>` 左移、右移运算符

`<` `<=` `>` `>=` `==` `!=` 比较运算符

`&` 按位与

`xor` 按位异或

`|` 按位或

`and` 逻辑与

`or` 逻辑或

`=` `+=` `-=` `*=` `/=` `%=` `^=` `<<=` `>>=` `&=` `|=` 赋值系列运算符

表达式化的 `if` 中的 `if` 也可以看作一种抽象运算符，其优先级位于 `or` 之下。

还有一个特殊之处，Yaya 语言的运算符采取**最长匹配原则**。例如，`1+-2` 不会被解析为 `1 + - 2`，而是 `1 +- 2`。这段代码将在运行时因为不存在 `int` 之间的 `+-` 运算符而报错。

### 3.6 运算符重载及自定义

Yaya 语言的运算符重载基于最长匹配原则之上，用户可以自定义运算符。想要重载一个运算符有两种方法，但大部分运算符只能使用其中一种。

重载运算符方法一：使用 `operator` 关键字。适用于大部分比较简单的运算符。

使用 `operator` 关键字重载 `+` 运算符的一个典型代码示例如下：

```yaya
class Point {
    func init(x, y) {
        this.x = x;
        this.y = y;
    }
}

class Point3D extends Point {
    func init(x, y, z) {
        super.init(x, y);
        this.z = z;
    }
}

operator Point + Point (a, b) {
    return Point(a.x + b.x, a.y + b.y);
}

a = Point(1, 2);
b = Point(3, 4);
c = a + b;
print(c.x, c.y); # 输出 4 6
d = Point3D(5, 6, 7);
e = c + d; # 不合法：`operator` 关键字定义的运算符不考虑继承
```

`operator` 对参数自带类型检查，因此不必添加类型标注（其实也不能添加）。

重载单目运算符的方法与之类似，例如想给 `Point` 重载单目负运算符，只需要在声明时 `operator - Point(a)` 即可。`operator` 的声明**不能放在一个类中作为方法**。

`operator` 关键字还可以用于创建新运算符。其具体流程如下：

首先在全局声明新运算符在表达式中的位置、优先级、结合性。具体格式是：

```yaya
infix operator +- 150 lassoc;
```

第一个词可以是 prefix、infix 或 postfix，若为 prefix/postfix，则无需指定优先级和结合性。

第二个词必须是 operator。

第三个词是接下来要定义的运算符。

接下来是一个数字，表示优先级。再后面是 `lassoc` 或 `rassoc`，分别对应左结合与右结合。如果不填结合性，默认左结合。优先级数字在上面的优先级表中，是从 `or` 为 0 开始往上依次增 10 来计算的，不能为负数，可以为小数。

接下来，就可以使用 `operator` 关键字来声明运算符了。例如：

```yaya
infix operator +- 150 lassoc;

class Point {
    func init(x: int = 0, y: int = 0) {
        this.x = x;
        this.y = y;
    }
};

operator Point +- Point (a, b) {
    return Point(a.x - b.x, a.y - b.y);
}
```

`operator` 语句块中遇到的运算符，必须已经事先用 `infix`、`prefix` 或 `postfix` 声明过。运算符左右分别为两个操作数的类型，若某操作数不存在，则应省略之。此处不考虑继承。运算符操作数的数量及位置，应与前面的声明相符。

重载运算符方法二：部分抽象运算符无法直接重载，例如类实例的调用运算符。

重载一个类实例的某运算符，需要在类中需要对应的魔术方法来重载。例如，想要重载类实例的调用，就要在类中实现魔术方法 `__call__`，依此类推。

有关这种方法的具体内容，请参阅标准库规范中的“魔术方法”一节。

### 3.7 异常系统

所有异常均基于基类 `Throwable`。只有 `Throwable` 及其子类的实例能跟在 `throw` 表达式后被抛出。

`Throwable` 之上共分两大类：一类为 `Exception`，可处理的运行时错误集中于此；一类为 `Error`，多为无法处理的问题（`OOM`、栈溢出）或是语言内部错误（如 `StopIteration` 等）。本标准不对具体异常类作出规定，有关具体异常类的设计，请参见标准库文档。以下对语法上对异常的处理做进一步说明。

在语法上使用 `throw` 抛出一个异常。原则上禁止直接抛出 `Throwable` 或是 `Error` 及 `Error` 的子类的实例。

```yaya
throw Exception(); # Correct
```

而在接收端，使用 `try` 和 `catch` 来接收异常。若存在 `finally`，则除非 `try` 中直接退出了 Yaya 程序，否则应该保证 `finally` 的执行。

`catch` 如不使用类型标注，则相当于接受全部异常。如使用类型标注，则只接受标注的异常类及其子类的异常，此时可以连用多个 `catch`，遵循先到先得原则。即若某异常类 B 是异常类 A 共同的子类，又同时写了 A 和 B 的 `catch`，当有 B 被抛出时，哪个 `catch` 先写则被谁捕获。

例如，如下的代码是可以接受的。

```yaya
try {
    some_function();
} catch (exc) {
    # 所有异常都会聚集于此
    exc.print_stack_trace();
} finally {
    something_else();
}
```

如下也可以。

```yaya
try {
    
} catch (exc: ZeroDivisionException) {

} catch (exc: IOException) {

} catch (exc) {
    # 其他异常
} finally {

}
```

若 `finally` 抛出异常或返回值，则 `try` 中异常将被自动丢弃，即使没有 `catch` 亦如此。

### 3.8 函数细则

函数可以指定默认参数，从而在调用时填充默认值。例如：

```yaya
func add(x, y = 0) {
    return x + y;
}

add(3); # 与 add(3, 0) 意同
add(3, 2); # y = 0 被忽略
```

默认参数的值应被放在类型标注之后：

```yaya
func add(x, y: int = 0) {
    return x + y;
}
```

默认参数的默认值同样适用运行时检查。也即，以下代码不会报错：

```yaya
func add(x: float, y: int32 = 0) { # 0 并不是 int32 类型的值
    return x + y;
}

add(1.0, 2i32);
add(3.0, 5i32);
add(7.13, 2022i32);
# 然而上述三个调用均覆盖了默认参数，从而通过了运行时检查，不会报错
# add(3); # 报错：0 不是 int32 类型的值
```

通过在参数前添加 `...`，可以让这个函数（理论上）接收无数参数，这些参数都将被收入与此参数同名的一个 `list` 中传递。

```yaya
func sum(...args) {
    var s = 0;
    for (var i = 0; i < len(args); i++) {
        s = s + args[i];
    }
    return s;
}
```

在列表后添加 `...`，可将其展开传入函数中。

```yaya
func sum(...args) {
    var s = 0;
    for (var i = 0; i < len(args); i++) {
        s = s + args[i];
    }
    return s;
}

sum([1, 2, 3, 4, 5]...); # 15
```

这样就可以写出如下代码：

```yaya
func sum(first, ...args) {
    return first + sum(args...);
}
```

对 `...args`，也可以进行类型标注，具体效果与标注一个 `list` 相同。

### 3.9 类型转换

Yaya 语言中类型转换非常简单，如想要定义从 A 类型转换为 B 类型的函数，只需在 A 类型中添加 `toB` 方法，然后在 A 类实例后添加 `as B` 即可。需要注意的是：此时语言实现既不会检查 `B` 是否存在，也不会检查 `toB` 的返回值是否确实为 B 类型。

也即，下面的代码是完全合法的：

```yaya
class A {
    func toString() {
        print("Nothing wrong!!");
    }
}

# toString 返回 null，String 也不存在
A as String; # 输出 Nothing wrong!!
```

## 四、词法作用域

一个能说明 Yaya 语言词法作用域的典型示例程序如下：

```yaya
var a = 3;
{
    var a = 4;
    print(a);
}
print(a);
```

分别输出 `4` 和 `3`。

Yaya 语言的词法作用域与 C++ 等主流语言基本一致，即每一个语句块对应单独的词法作用域。查找变量时，查找优先级按词法作用域从近到远排序。

与主流语言不同的是，Yaya 语言的 `super` 关键字当不在类方法中时，可以**强行访问上层词法作用域**。例如，下面的程序：

```yaya
var a = 3;
{
    var a = 4;
    print(a);
    print(super.a);
}
```

分别输入 `4` 和 `3`。

需要注意的是，`super` 有以下两大特性：第一，对于不存在的成员（即上层作用域不存在的变量）进行写入，将**不会影响上层作用域**。也就是说，下面的代码：

```yaya
var a = 3;
{
    var a = 4;
    super.b = 2;
}
print(b);
```

不会输出 `2`，而是会报出“找不到变量 `b`”之类的错误。

第二，如果出现了函数调用的情况，则认为函数顶层作用域的上层作用域为**调用者的作用域**。例如，下面的代码：

```yaya
func factorial(n)
{
    if (n < 0) return -1;
    var result = 1;
    fact_inner();
    return result;
}

func fact_inner()
{
    var n = super.n - 1;
    var result = super.result;
    if (n <= 0) return;
    fact_inner();
    super.result = result * (n + 1);
}
```

就通过完全混乱邪恶的方法实现了可以正常工作的阶乘函数 `factorial`。

该方法用到的细节太多，需结合实例进行分析。例如，当调用 `factorial(3)` 时，其工作逻辑是这样的：

- `factorial(3)` 作用域内有 `n = 3, result = 1`，然后调用 `fact_inner()`。

- 第一个 `fact_inner()` 作用域内有 `n = 2, result = 1`，然后进入递归。

- 第二个 `fact_inner()` 作用域内有 `n = 1, result = 1`，然后进入递归。

- 第三个 `fact_inner()` 作用域内有 `n = 0, result = 1`，此时进入 `if (n <= 0)` 分支回到第二个 `fact_inner()`。

- 在第二个 `fact_inner()` 中，`super.result`，也即**第一个 `fact_inner()` 作用域**内的 `result` 变为**第二个 `fact_inner()` 作用域**内的 `result` 乘以**第二个 `fact_inner()` 作用域**内的 `n` 加一的和，也即 `1 * (1 + 1) = 2`。然后程序结束，回到第一个 `fact_inner()`。

- 回到第一个 `fact_inner()` 时，作用域内变量**已经发生变化**：`n = 2` 不变，但 `result` 变为 `2`。接下来重复上述逻辑，将 `factorial(3)` 作用域内的 `result` 变为 `2 * (2 + 1)` 也即 `6`。

- 回到 `factorial(3)`，`result` 内即为正确结果 `6`，直接返回。可以用数学归纳法证明，以上逻辑对任意正整数 `n` 都成立。

## 五、标准库函数

参见单独的标准库规范。