import "../../std/math.t"
extern printString(string str) -> number
extern printAscii(number c) -> number
for j = 0, j<10, 1 then
    for i = 0, i < 10-j, 1 then
        printString("*")
    end
    printAscii(10)
end
printString("Hello World!")
return 0