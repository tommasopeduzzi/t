import "../../std/io.t"
import "../../std/math.t"
printString("Hello World!")
var number[4] test
for i = 0, i < 4, 1 do
    test[i] = i
end
for i = 0, i < 4, 1 do
    printNumber(test[i])
end
printNumber(test[0])
return 0