## What is this?

This is a hackable and extensible lexer, parser and interpreter for a minimalistic, imperative, C-like language. It can also be used as an educational tool for understanding lexing and parsing.

## How does it work?

The lexer produces a list of tokens from the input (a `PROT_READ` memory-mapped file). Each individual token is produced by consuming a character from the input and then asking each of the token functions whether they accept it. Each such token function has an internal state and can return any of `STS_ACCEPT`, `STS_REJECT` or `STS_HUNGRY` on a character consumed. The lexer reads in characters until all of the functions return `STS_REJECT` (at which point they reset their internal state), and then the accepted token is determined by looking back for an `STS_ACCEPT` from the previous iteration.

Essentially, this is a "maximal munch" algorithm.

The parser takes the list of tokens and produces a tree. It does that by continuously shifting tokens off the input to the parse stack and then reducing any matching suffix of the stack to a non-terminal, according to the rules of the grammar. The grammar is defined as a static array of structs, where each struct is a rule. When a rule matches a suffix of the stack, a reduction is made. The reduction essentially creates a single level of child nodes (the symbols that matched the rule) and they get parented by a new non-terminal symbol on the stack (the left-hand side of the matching rule).

In effect, this is a shift-reduce, bottom-up parser. Because the parser has no state and decision tables, a few additional hacks are implemented in order to support operator precedence and if-elif-else chains.

The interpreter is really straightforward. It starts from the top of the parse tree and walks down through the child nodes, executing the statements and evaluating the expressions. Any warnings during the execution of the program are written to standard error with a `warn:` prefix.

## The Language

* Control-flow statements (the curly braces are mandatory):
  * `if (Expr) { N✕Stmt } elif (Expr) { N✕Stmt } else { N✕Stmt }`
  * `while (Expr) { N✕Stmt }` 
  * `do { N✕Stmt } while (Expr);`

* Variable and array assignment (integers only, `Name` is equivalent to `Name[0]`):
  * `Name = Expr;`
  * `Name[Expr] = Expr;`

* Printing to standard output (integers only):
  * `print "Placeholder: " Expr;`
  * `print Expr;`

* Parenthesised expressions (integers only):
  * `(Expr)`

* Binary expressions (between two integers):
  * `Expr OP Expr`, where `OP` is `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&` or `||`

* Unary expressions (integers only):
  * `OP Expr`, where `OP` is `-`, `+` or `!`

* A ternary expression (integers only):
  * `Expr ? Expr : Expr`

* Line and block comments:
  * `// line comment`
  * `/* block comment */`

## Sample Output
You start the interpreter by specifying the file containing the code.

Once the file is opened and mapped into memory, the lexer starts. The tokens will be written to standard output as they appear in the file, in alternating colours (green and yellow), so that you can clearly see where each token starts and ends.

If the lexing was successful (all the tokens were recognised), the parser starts. On each shift or reduce operation, it outputs a single line with the current contents of the parse stack. Non-terminals are in yellow, terminals are in green. Finally, if the parsing was successful, the parse stack should contain a single non-terminal called "Unit".

The interpreter then starts from the root of the tree (which is always "Unit"), and executes the tree produced by the parser.
```
$ ./interp tests/fizzbuzz.txt 
*** Lexing ***
number = 1;

do {
    if (number % 3 == 0 && number % 5 == 0) {
        print "FizzBuzz " number;
    } elif (number % 5 == 0) {
        print "Fizz " number;
    } elif (number % 3 == 0) {
        print "Buzz " number;
    } else {
        print number;
    }
    
    number = number + 1;
} while (number <= 100);

*** Parsing ***
Shift: ^ 
Shift: ^ number 
Shift: ^ number = 
Shift: ^ number = 1 
Red19: ^ number = Atom 
Red20: ^ number = Expr 
Shift: ^ number = Expr ; 
Red05: ^ Assn 
Red02: ^ Stmt 
Shift: ^ Stmt do 
Shift: ^ Stmt do { 
Shift: ^ Stmt do { if 
Shift: ^ Stmt do { if ( 
Shift: ^ Stmt do { if ( number 
Red18: ^ Stmt do { if ( Atom 
Red20: ^ Stmt do { if ( Expr 
Shift: ^ Stmt do { if ( Expr % 
Shift: ^ Stmt do { if ( Expr % 3 
Red19: ^ Stmt do { if ( Expr % Atom 
Red20: ^ Stmt do { if ( Expr % Expr 
Red39: ^ Stmt do { if ( Bexp 
Red22: ^ Stmt do { if ( Expr 
Shift: ^ Stmt do { if ( Expr == 
...
Red21: ^ Stmt do { Stmt Stmt } while Expr 
Shift: ^ Stmt do { Stmt Stmt } while Expr ; 
Red16: ^ Stmt Dowh 
Red11: ^ Stmt Ctrl 
Red04: ^ Stmt Stmt 
Shift: ^ Stmt Stmt $ 
Red01: Unit 
ACCEPT Unit 

*** Running ***
1
2
Buzz 3
4
Fizz 5
Buzz 6
7
8
Buzz 9
Fizz 10
11
Buzz 12
13
14
FizzBuzz 15
...
94
Fizz 95
Buzz 96
97
98
Buzz 99
Fizz 100
