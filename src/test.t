import "../../std/math.t"
extern printString(string str) -> number
extern printAscii(number c) -> number
extern printNumber(number c) -> number
for i = 0, i < 100, 1 then
    printNumber(i)
    printAscii(10)
end
return 0