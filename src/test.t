def fib(i)
    var x = 0
    if i < 2 then
        return i
    else
        return fib(i-1) + fib(i-2)
    end
end
return fib(14)