var a = 3;
{
    var a = 4;
    print(a);
}
print(a);

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