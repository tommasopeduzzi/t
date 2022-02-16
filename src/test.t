import "../../std/io.t"
# import "../../std/math.t"
printString("Hello World!")
struct testtype
    number[4] test1
    number[4] test2
end
var testtype[4] test
for i = 0, i < 4, 1 do
    test = i
end
return 0