import "../../std/io.t"
def approx(number x, number target) -> bool
   if target-0.5 <= x then
       if target + 0.5 >= x  then
           return true
       end
   else
       return false
   end
   return false
end

if approx(1, 1.2) then
    printString("works")
else
    printString("doesn't work")
end
return 0