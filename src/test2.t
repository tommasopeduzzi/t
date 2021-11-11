def sin(x)
    var result = 1
    var pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/(pi*i)))*(1-(x/(pi*i*(0-1))))
    end
    return result
end