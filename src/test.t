import "../../std/io.t"
#import "../../std/math.t"
#import "../../std/string.t"
var list of number test2
for i = 0, i<1000, 1 do
    test2[i] = i + 1
end
for i = 0, i<1000, 1 do
    printNumber(test2[i])
    printString("\n")
end
return 0
