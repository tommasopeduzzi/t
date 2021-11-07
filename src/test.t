def test(n)
    var result = 0
    for i = 1, i < n+1, 1 then
        result = result + i
    end
    return result
end
return fib(9)