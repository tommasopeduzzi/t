import "../../std/io.t"
# import "../../std/math.t"
# printString("Hello World!")
struct testtype
    number test1
    string test2
end
var testtype test
test.test1 = 1
test.test2 = "Hello World!"
printString(test.test2)
printNumber(test.test1)
return 0