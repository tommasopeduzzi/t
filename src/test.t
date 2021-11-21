import "../../std/io.t"
for i = 0, i < 5, 1 then
    printString("> ")
    var string input = input()
    printString(input)
    printAscii(10)
end
return 0