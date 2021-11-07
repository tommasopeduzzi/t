def test(n)
    var result = 0
    var i = 10000
    while result < n then
        result = result + 1
        var i = 10000000
    end
    return i
end
return test(9)