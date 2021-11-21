import "../../std/io.t"
for i = 0, i <= 1, 1 then
    printString("> ")
    var string input = input()
    printString(input)
    printAscii(10)
end
if 1 == 1 then
    printString("1 == 1")
else
    printString("1 != 1")
end
return 0