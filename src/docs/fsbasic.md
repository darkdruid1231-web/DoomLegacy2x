# FraggleScript Basics

> Note: We'd like to thank Quasar for allowing us to modify the docs of the [Eternity Engine](http://www.doomworld.com/eternity/etcengine.html) to reflect Legacy FraggleScript implementation.

## Contents

- [Getting Started](#getting-started)
- [Defining Scripts](#defining-scripts)
- [Variables and Data Types](#variables-and-data-types)
  - [Type Coercions](#type-coercions)
- [Calling Functions](#calling-functions)
- [Control Flow Structures](#control-flow-structures)
- [Script Activation Models](#script-activation-models)
- [Operators and Operator Precedence](#operators-and-operator-precedence)
- [Keyword List](#keyword-list)

---

## Getting Started

To get started using FraggleScript in your DOOM levels, you'll need to fully understand most or all aspects of DOOM level editing and WAD file manipulation. If you haven't mastered this basic stuff, it would probably be wise to check out the Doomworld tutorials section and to look up a few FAQs. These documents assume you understand basic DOOM editing.

When you first want to create scripts, you should save a blank file from a suitable text editor as something appropriate, like "map01.fs". The `.fs` extension is not required, but its useful for figuring out what and where your files are later. When you have the file, you need to place a header in it like this:

```
[scripts]
```

This tells the game that your mapinfo is declaring a section for scripts. Note that FraggleScript scripts in Legacy reside in the map header along with all other mapinfo information. If you want to define other mapinfo blocks, you can put them before or after this section. Example:

```
[level info]
levelname = The Palace of w00t
creator = Quasar

[scripts]
```

After the header is in place you can begin defining scripts and variables. When your job is done, you can use the `add_fs` utility, available with Legacy, to insert your scripts into the level.

---

## Defining Scripts

Scripts are the basic subprogram unit of FraggleScript, similar to the functions of C and the procedures of Pascal. FraggleScript scripts do not take any explicit parameters, however, and cannot return values, which is quite different from most languages. Levels can currently have up to 256 scripts, numbered from 0 to 255 (future expansion of this number is possible to allow for persistent scripts). Scripts exist only within the level to which they belong, and for the most part only affect that level, with the exception of hub variables.

To declare a script, follow the syntax of the following example:

```
[scripts]

script 0
{
}
```

The `script` keyword denotes that the script definition is starting, and the number following it is the number for this script. Script numbers should always be unique, one per defined script.

The script above is valid, but it is not very interesting because it does nothing, and a script alone cannot run without first being called. Scripts may be invoked in several manners, which is covered in the Script Activation Models section.

---

## Variables and Data Types

One way in which scripts can accomplish things is to interact with variables. Variables can be of three natures:

**Built-In**
These variables are always present and are defined by the FraggleScript runtime. They can be accessed by any script. Their value typically defaults at the beginning of each level. Built-in variables, current to Legacy 1.40, include:
- `int consoleplayer` — the player number, 0 to 3 inclusive, of the player attached to the local machine
- `int displayplayer` — the player number of the player being displayed on the local machine
- `int zoom` — the zoom level for the game camera
- `mobj trigger` — a reference to the mapthing that started this script. This variable is important and very useful.

**Global**
These variables are defined outside of any script, either in a header file or in the `[scripts]` section of the mapinfo lump. Any scripts in the current level can access these types of variables. If global variables are declared with the `const` keyword, they are constants, and if they are declared with the `hub` keyword, then the current list of hub variables will be searched by name for a match when the declaration is encountered. Hub global variables persist between levels and can be accessed and modified by scripts in any level until the current cluster ends.

Example:
```
[scripts]

const DOORSPEED = 4;
hub int visitedSwamp = 0;

int i = 0;

script 0
{
   i = i + 1;
}
```

Note that `const` variables adapt to the default type for their provided literal, while hub global variables require explicit typing.

**Local**
These variables are declared inside a script. They can only be accessed within the script itself, and are destroyed when the script completes execution.

Example:
```
[scripts]

int i = 0;

script 0
{
   int i = 1;
   print(i);
}
```

Note that the print function in this example will print the string "1" and not "0" because local variables always take precedence over any built-in or global variables. This is an important distinction to remember.

Variable names may be of arbitrary length, but should not be named with any FraggleScript reserved word. FraggleScript has four primary data types:

- **`int`** — 32-bit signed integer. Only decimal integer literals are accepted.
  ```
  int x = 0;
  ```

- **`fixed`** (also `float`) — a 32-bit fixed-point number, somewhat similar to floating-point except that the decimal place is fixed at 16 bits so that the word is evenly divided into integer and decimal parts. `fixed` numbers must be specified with a decimal point, otherwise the literal will be interpreted as an integer. `fixed` values are used for high precision interaction with the game world.
  ```
  fixed f = 2.71828;
  ```

- **`string`** — a string of ASCII characters. FraggleScript strings are limited in length to 256 characters. The following escape sequences are supported:
  - `\n` — line break
  - `\\` — a literal `\` character
  - `\"` — a literal `"` character
  - `\?` — a literal `?` character
  - `\a` — bell character — causes the console to play a sound
  - `\t` — tab
  - `\0` — write white text

  Strings must be delimited by quotation marks:
  ```
  "Hello, World!\n"
  ```

- **`mobj`** — an opaque reference to a DOOM mapthing (ie monster, lamp, fireball, player). The values of these references must either be obtained from object spawning functions, or can be specified by use of integer literals, in which case the mobj reference will point to the mapthing numbered by the map editor with that number.
  ```
  // spawn a new dwarf and keep a reference to it in "dwarf"
  mobj dwarf = spawn(HALIF, 0, 0, 0);

  // get a reference to the thing numbered 0 at map startup
  mobj firstthing = 0;
  ```

  Note that using map editor numbers for things has the distinct disadvantage that when the map is edited, the things will be automatically renumbered if any are deleted. It is suggested that the latter form of mobj reference assignment be avoided unless necessary. `mobj` references are very powerful and allow a large number of effects and fine control not available in other languages such as ACS.

  Also note that although integer literals can be used to assign mobj reference values, mobj and int are not interchangeable, and statements such as `mobj halif2 = halif + firstthing;` are not, in general, meaningful.

### Type Coercions

FraggleScript is a weakly typed language, and as such, coercions are made freely between all data types.

**Conversion to `int` from:**
- `string` — the string will be passed to the ANSI C function `atoi()`
- `fixed` — the fixed-point value will be chopped to its integer portion with no rounding
- `mobj` — coercion is not meaningful, -1 is always returned

**Conversion to `fixed` from:**
- `string` — the string will be passed to the ANSI C function `atof()`, and then the resulting double value is renormalized and chopped into fixed precision (this process incurs unavoidable round-off error).
- `int` — the integer value is renormalized as a fixed value (overflow is possible if the integer is greater than 32767 or less than -32768, and is not checked).
- `mobj` — coercion is not meaningful, `-1*FRACUNIT` is returned (FRACUNIT is equal to `1<<16` and represents the value 1 in the fixed-point number system).

**Conversion to `string` from:**
- `int` — the value is converted to a string via the rules of ANSI C's stdio functions
- `fixed` — the value is renormalized as an IEEE double floating-point number and then converted to string via the rules of ANSI C's stdio functions (this process incurs some unavoidable round-off error)
- `mobj` — coercion is not meaningful, the string `"map object"` is always returned

**Conversion to `mobj` from:**
- `int` — the level is checked for a mapthing numbered by the value of this integer and if it exists, a reference to it is returned — if it does not exist, a script error will occur
- `string` — the string will be passed to `atoi()` and the resulting integer value will be cast via the rules above
- `fixed` — the fixed value is chopped to its integer portion and then the resulting integer value will be cast via the rules above

Coercion is an automatic process that takes place when a variable is assigned the value of a variable of a different type, when values are passed to functions that do not match the specified parameter types, and when operands of multiple types are used with some operators. Some functions may perform more strict type checking at their own volition, so beware that script errors may occur if meaningless values are passed to some functions.

---

## Calling Functions

FraggleScript offers an extensible host of built-in functions that are implemented in native code. They are the primary means to manipulate the game and cause things to happen. The FraggleScript Function Reference is a definitive guide to all functions supported by the Legacy dialect; this document will provide some basic examples of function use.

Most functions accept a certain number of parameters and expect them to represent values of specific meaning, such as an integer representing a sector tag, or an mobj reference to a mapthing to affect.

Type coercions can occur when functions are passed parameters of other types. An excellent example is the following:

```
script 0
{
   startscript("1");
}
```

The `startscript` function expects an integer value corresponding to the number of a script to run. Here it has been passed a string, `"1"`. Strings will be converted to the integer they represent, if possible, so this string is automatically coerced into the integer value 1, and the script 1 will be started.

An example of a coercion that is NOT meaningful in the intended manner would be the following:

```
script 0
{
   mobj halif = 0;

   startscript(halif);
}
```

`mobj` references can be assigned using integer literals, but the rules for coercion from mobj to int state that -1 is always returned for an mobj value (this is because there is not a one-to-one mapping between mobj references, which can include objects spawned after map startup which do not have a number, and integers). This statement has the effect of `startscript(-1);` and since -1 is not in the domain of startscript, a script error occurs.

Parameter coercions of this type should be avoided for purposes of clarity and maintainability of your code.

Note that some functions, like print, can take a variable number of parameters. These types of functions generally treat all their parameters in a like manner, and can accept up to 128 of them.

Functions may return values, and in fact, most useful functions do. To capture the return value of a function, you simply assign it to a variable:

```
script 0
{
   fixed dist;

   dist = pointtodist(0, 0, 1, 1);
}
```

It is not necessary to capture the return value of a function simply because it has one. Likewise, it is possible to use function return values without explicitly capturing them in variables:

```
script 0
{
   print(spawn(IMP, 0, 0, 0));
}
```

This causes a new imp to be spawned at (0,0,0). Since `spawn` additionally returns a reference to the new object, the mobj reference returned by spawn becomes the parameter to the print function, and the resulting output to the console is "map object".

If a function is listed as being of return type void, this means that it does not return a meaningful value and that it operates by side effect only. However, unlike in C, void functions in FraggleScript will return the integer value 0, rather than causing an error when used in assignments.

Note that it is possible to make limited function calls outside of any script, in the surrounding program environment, which in FraggleScript is referred to as the **levelscript**. Function calls placed in the levelscript will be executed once (and only once) at the beginning of the level. Example:

```
[scripts]

script 0
{
   message("Welcome to the Palace of w00t, foolish mortals!");
}

startscript(0);
```

In this example, all players would see this message at the beginning of the level. It is not required, but is good style, to put any levelscript function calls after all script definitions.

---

## Control Flow Structures

Control flow structures allow your code to make decisions and repeat actions over and over.

### while

The while loop is the basic loop control structure. It will continually loop through its statements until its condition evaluates to 0, or false.

Basic syntax:
```
while(<condition>)
{
   <statements>
}
```

Unlike in C, the braces are required to surround the block statement of a while loop, even if it only contains one statement. An operative example:

```
script 0
{
   int i = 0;
   while(i < 10)
   {
      print(i, "\n");
      i = i + 1;
   }
}
```

This code would print the numbers 0 through 9 to the console.

The `continue()` and `break()` functions are capable of modifying the behavior of the while loop. `continue()` causes the loop to return to the beginning and run the next iteration, while `break()` causes the loop to exit completely, returning control to the surrounding script.

A while loop can run forever if its condition is never false, but if you write a loop like this, be sure to call one of the wait functions inside the body of the loop, or else the game will wait on the script to finish forever, effectively forcing you to reboot!

Example (also demonstrates ambient sounds with FraggleScript):
```
script 0
{
   while(1)
   {
      if(rnd() < 24)
      {
         ambientsound("dsbells");
      }
      wait(20);
   }
}
```

### for

The for loop is a more sophisticated loop structure that takes three parameters. Note that unlike C, the FraggleScript for parameters are separated by commas, not by semicolons, and all 3 are required to be present.

Basic syntax:
```
for(<initialization>, <condition>, <iteration>)
{
   <statements>
}
```

Braces are required, and the `continue()` and `break()` statements work the same way here as they do for `while()`, with the exception that a continuing for loop will check its condition and perform its iteration.

An operative example:
```
script 0
{
   int i;

   for(i=0, i<10, i=i+1)
   {
      print(i, "\n");
   }
}
```

### if, elseif, else

`if` tests its condition, and if the condition evaluates to any non-zero value, or true, the statements inside its body are executed. Note that unlike `while` and `for`, braces are not required for an `if` statement UNLESS you either intend for the if body to contain more than one statement, OR you intend to place an `elseif` or `else` clause after the if.

Basic syntax:
```
if(<condition>)
  <statement>

if(<condition>)
{
   <statements>
}
```

Examples:
```
if(i > 10)
  return();
```
```
if(i)
{
   print(i, "\n");
   return();
}
```

`elseif` and `else` are ancillary clauses that execute when their corresponding `if` statement was not true. `elseif` tests its own additional condition, and if it is false, control passes to the next elseif or else, if one exists. An if can have any number of elseif clauses, and one else clause following it. If there is an else clause, it must always be last. When the if statement evaluates to true, no elseif or else clauses will be executed.

Example:
```
script 0
{
   int i = rnd()%10;

   if(i == 1)
   {
      spawn(IMP, 0, 0, 0);
   }
   elseif(i <= 5)
   {
      spawn(DEMON, 0, 0, 0);
   }
   else
   {
      spawn(BARON, 0, 0, 0);
   }
}
```

Note that `elseif` and `else` are new to the Eternity and DOOM Legacy dialects of FraggleScript and that they are not currently supported by SMMU.

---

## Script Activation Models

Script activation models are simply the different ways in which scripts can be started. Currently supported activation models include the following:

- **startscript function** — Used to start scripts from within FraggleScript code. If used from the outer context, the script will be started automatically at the beginning of the level, and player 0 will be used as the trigger object. If used from inside another script, the current script's trigger object will propagate to the new script.

- **StartFS and StartWeaponFS codepointers** — Used to start scripts from mapthing frames, this method relies on and interacts with DeHackEd editing. To use this method, use a BEX codepointer block to place the `StartScript` pointer into a thing's frame, and then set the frame's "tics" field to the script number to call. The thing whose frame called the codepointer becomes the trigger object.

- **linedef activation** — A host of new linedef types are provided to allow activation of scripts from within levels:
  - 272  WR  Start script with tag number
  - 273  WR  Start script, 1-way trigger
  - 274  W1  Start script with tag number
  - 275  W1  Start script, 1-way trigger
  - 276  SR  Start script with tag number
  - 277  S1  Start script with tag number
  - 278  GR  Start script with tag number
  - 279  G1  Start script with tag number

  To use these lines, simply give them the appropriate type and set the linedef's tag number to the script number you want to be called. The trigger object will be set to the object that activated the line.

---

## Operators and Operator Precedence

The Legacy dialect of FraggleScript supports the following operators, in order of precedence from greatest to least. All operators except `=` are evaluated from left to right.

| Operator | Coercions | Description |
|---|---|---|
| `.` | none | The structure operator — can be used to beautify function calls by moving the first argument outside the parameter list. Example: `halif.kill();` |
| `--` | string→int, mobj→int | The decrement operator. Prefix form operates first; postfix propagates current value then decrements. |
| `++` | string→int, mobj→int | The increment operator. Similar to decrement, but increases the variable's value by 1. |
| `!` | all→int | The logical invert operator. Returns 1 if the operand has an integer value of 0, and 0 otherwise. |
| `~` | all→int | The binary invert operator. Reverses all bits in the operand's integer value. |
| `%` | (all\*all)→(int\*int) | The modulus operator. Returns the remainder when its first operand is divided by its second. |
| `/` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | The division operator. Returns the quotient. If either operand is fixed, the other is coerced to fixed. |
| `*` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | The multiplication operator. If either operand is fixed, the other is coerced to fixed. |
| `-` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | The subtraction and unary minus operators. `-1` is evaluated as `0-1`. |
| `+` | (string\*all)→(string\*string), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | The addition operator. If either operand is fixed, the other is coerced to fixed. |
| `>=` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | Returns 1 if first operand ≥ second, 0 otherwise. |
| `<=` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | Returns 1 if first operand ≤ second, 0 otherwise. |
| `>` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | Returns 1 if first operand > second, 0 otherwise. |
| `<` | (string\*all)→(int\*int), (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | Returns 1 if first operand < second, 0 otherwise. |
| `!=` | (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | Logical nonequality. Tests string equality for strings, numeric equality for numeric types. |
| `==` | (mobj\*all)→(int\*int), (fixed\*all)→(fixed\*fixed) | Logical equality. Tests string equality for strings, numeric equality for numeric types. |
| `&` | (all\*all)→(int\*int) | Binary and. Sets all bits that are set in both operands. |
| `\|` | (all\*all)→(int\*int) | Binary or. Sets all bits that are set in either operand. |
| `&&` | (all\*all)→(int\*int) | Logical and. Returns 1 if both operands are 1. Short-circuits if first operand is 0. |
| `\|\|` | (all\*all)→(int\*int) | Logical or. Returns 1 if either operand is 1. Short-circuits if first operand is 1. |
| `=` | coerces expression type to variable type | Assignment operator. Sets the variable on the left to the value of the expression on the right. |

---

## Keyword List

Variables cannot be named these words because they are reserved:

```
const
else
elseif
fixed
float
for
hub
if
int
mobj
script
string
while
```

Remember that `break`, `continue`, `return`, and `goto` are defined as special functions, and not as keywords.

While the names of functions are not reserved words in the strictest sense, you should additionally avoid naming variables with the same name as functions since these variables will hide the functions and make them inaccessible.
