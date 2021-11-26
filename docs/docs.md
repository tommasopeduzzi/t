# The t Programming Language

## The Why
I was curious Because I want the language to be more or less platform-independent and because of the ease-of-use, the compiler uses llvm as its backend.
The [Kaleidoscope Tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/) served as a basis for this project. I added features to it and used my own syntax. 
My goal with this language is to be able to teach others how to solve problems instead of writing code, so it should be as simple as possible.

## Language basics
### Types
Currently there are 4 fundamental types, three of them with their own literals:
- `number`: a number stored as a C `double`
- `string`: a string of characters stored in an equivalent form of C `char*`.
  - Can be initialized with a string literal or a string literal expression (text surrounded by `"`).
- `bool`: a boolean
  - Has a value of either `true` or `false`
- `void`: void type
There is no such thing as type-checking at the moment, so be careful.
### Variables
Variables are declared with the `var` keyword followed by the type of the variable (one of the fundamental types, except `void`).  
Examples:
```
var number x = 0
var bool y = true
var string z = "Hello World!"
```
### Conditional Statements
Conditials in t are in the form `if-else` statements.
A If-Else-Statement is structured as follows: 
<pre>
<b>if</b> condition <b>then</b> 
  statement(s) 
<i><b>else</b> 
  statements(s)</i> 
<b>end</b>
</pre>
The Else Block is optional.

If the condition evaluates to true, the first block of code is executed. If not, the second block of code is executed.
Here an example:
```
if approx(1, 1.2) do
  printString("1 is close to 1.2")
else
  printString("1 is not close to 1.2")
end
```
### Loops
#### While-Loops
While-Loops are structured as follows:
<pre>
<b>while</b> condition <b>do</b> 
  statements(s) 
<b>end</b>
</pre>
The statement(s) get executed until while the condition evaluates to `true`.
Here an example:
```
while x < 10 do
  x = x + 1
end
```
#### For-Loops
For-Loops are structured as follows:
<pre>
<b>for</b> name <b>=</b> value<b>,</b> condition<b>,</b> step <b>do</b> 
  statements(s) 
<b>end</b>
</pre>
Where `name` is the name of the variable, `value` is the initial value of the variable, `condition` is the condition that the variable has to satisfy, and `step` is the amount the variable is incremented by. Note that in the current stage of the language, the variable is always of type `number`.
Here an example:
```
for x = 0, 10, 1 do
  printString("x = ")
  printNumber(x)
  printAscii(10)
end
```
### Functions
#### Declaration
Function-Declarations are structured as follows:
<pre>
<b>def</b> name<b>(</b> parameters <b>) -> </b> type 
  statements(s) 
<b>end</b>
</pre>
Where `name` is the name of the function, `parameters` are the parameters of the function, and `type` is the return type of the function.
Each parameter is declared as follows, and separated by commas in the declaration:
<pre>
type name
</pre>
Where type is the type of the parameter, and name is the name of the parameter.
Here is an example for a function that checks if a number is approximately equal to another number:
```
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
```
#### Call
Function-Calls are structured as follows:
<pre>
name <b>(</b> parameters <b>)</b>
</pre>
Where `name` is the name of the function, and `parameters` are the parameters of the function. Note that there is still no type-checking at the moment, so be careful.
Here is an example for the function `approx`:
```
approx(1, 1.2)
```
