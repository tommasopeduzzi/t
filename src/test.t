import "../../std/io.t"
import "../../std/math.t"
#import "../../std/string.t"
struct testtype
    number test1
    string test2
end
var testtype test
test.test1 = 1
test.test2 = "Hello World!"
if test.test2 == "Hello World!" do
    printString("equal\n")
end
return 0