def sin(number x) -> number
    var number result = 1
    var number pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/(pi*i)))*(1-(x/(pi*-i)))
    end
    return result
end
def factorial(number n) -> number
    var number result = 1
    for i = 1, i < n, 1 then
        result = result * i
    end
    return result
end

def power(number a, number x) -> number
    var number result = 1
    for i = 0, i < x, 1 then
        result = result * a
    end
    return result
end

def max(number a, number b) -> number
    if a > b then
        return a
    end
    return b
end

def cos(number x) -> number
    var number result = 1
    var number pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/((pi/2)*i)))*(1-(x/((pi/2)*-i)))
    end
    return result
end

def tan(number x) -> number
    var number result = 1
    var number pi = 3.1415926535
    for i = 0, i < 3000, 1 then
        result = result * (1-(x/((pi/2)*i)))*(1-(x/((pi/2)*-i)))
    end
    return result
end

def sqrt(number x) -> number
    var number a = x/2
    for i = 0, i < 3000, 1 then
        a = (a + x/a)/2
    end
    return a
end

def root(number x, number k) -> number
    var number a = x/k
    for i = 0, i < 3000, 1 then
        a = ((k-1)*a + (x/power(a,k-1)))/k
    end
    return a
end