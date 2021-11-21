extern printString(string str) -> number
extern printAscii(number c) -> number
extern printNumber(number c) -> number
extern input() -> string
for i = 0, i < 100, 1 then
    var string input = input()
    printString(input)
end
return 0