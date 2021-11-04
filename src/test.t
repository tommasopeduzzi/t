def fib(n)
    var result = 1
    for i = 0 , i < n, i + 1 then
        result = result + i
    end
    return result
end
var test = 2
test = 3
return fib(9)