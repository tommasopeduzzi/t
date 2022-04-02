extern isEqual(string str1, string str2) -> bool

def len(string str) -> number
   var number length = 0
   while 1 do
      if str[length] == "" do
         return length
      end
      length = length + 1
   end
   return length
end