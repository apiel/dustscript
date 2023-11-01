# dustscript

DustScript is ultra minimalistic scripting language. Minimalistic as it is only few hundred lines of code, not even 1000 lines. Therefor the feature are limited to if statement, loop, variables and mathematical calculation. 

The syntax is also simplified to make it easy to parse:

```js
$width=100

someAction: some variable, 1 + 2, should be equal to 3
  anotherAction: indented of 2 spaces

print: this should print a variable $width

if: $width, <, 100
  print: this should not be printed as $width is not inferior to 100

if: $width, >, 50
  print: this should be printed as $width is superior to 50

$i=0
while: $i, <, 5
  print: This is a loop at index position $i
  $i=$i + 1

print: 10 + sqr(2) - 1 + 4 / 2, should be equal to 26
```

`command: param1, param2, param3` the value before the colon is the command, the values after are the parameter seprated by coma. `if` and `while` are reserved command name.

`$var=1` if a line start with dollar sign `$` it will assign a variable. The variable are all global, so even if they are indented, they will stay global.

For the `if` and `while` statement, indentation matter. Spaces ` ` are use for indention, tab will not work. A code block will be defined his indentation (similar to python).

`if` statement take 3 parameters where the second parameter is the operator to compare parameter 1 and 3. The available operators are `==` (equal), `!=` (not equal), `>` (superior), `<` (inferior), `>=` (superior or equal), `<=` (inferior or equal). There if no `elseif` or `else` statement.

`while` will loop till the condition is true. The condtion work in the same way as the if statement.

> To get styling for .dust extension in VScode, use `Ctrl` + `Shift` + `P` and type `Change Language Mode`. Then select `Configure File Association for '.dust'...` and select `CoffeeScript` or `JavaScript` (the color seem to work pretty well).

To use it, just include `dustscript.h`. 

> As example, see `dustscript.cpp`.

```cpp
#include <stdio.h>
#include "dustscript.h"

void scriptCallback(char *command, std::vector<string> params, const char *filename, uint16_t indentation)
{
    // callback function to bind custom function and params
}

int main()
{
    DustScript::load("demo.dust", scriptCallback);
    return 0;
}
```

Simply call `DustScript::load` with script path as first parameter and the callback function as second parameter. The callback function is used to bind the custom function. For example,  to print out the first parameter with the command `print` do:

```cpp
void scriptCallback(char *command, std::vector<string> params, const char *filename, uint16_t indentation)
{
    if (strcmp(command, "print") == 0)
    {
        printf(">> LOG: %s\n", params[0].c_str());
    }
}
```
