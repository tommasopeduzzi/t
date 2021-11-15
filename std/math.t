def sin(x)
    var result = 1
    var pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/(pi*i)))*(1-(x/(pi*-i)))
    end
    return result
end
def factorial(n)
    var result = 1
    for i = 1, i < n, 1 then
        result = result * i
    end
    return result
end

def power(a,x)
    var result = 1
    for i = 0, i < x, 1 then
        result = result * a
    end
    return result
end

def max(a,b)
    if a > b then
        return a
    end
    return b
end

def cos(x)
    var result = 1
    var pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/((pi/2)*i)))*(1-(x/((pi/2)*-i)))
    end
    return result
end

def tan(x)
    var result = 1
    var pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/((pi/2)*i)))*(1-(x/((pi/2)*-i)))
    end
    return result
end

