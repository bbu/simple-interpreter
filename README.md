# The Language

# Sample Output
You start the interpreter by specifying the file containing the code.

Once the file is opened and mapped into memory successfully, the lexer starts. The tokens will be written to standard output as they appear in the file, in alternating colors (green and yellow), so that you can clearly see where each token starts and ends.

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
} while (number < 100);

*** Parsing ***
Shift: ^ 
Shift: ^ number 
Shift: ^ number = 
Shift: ^ number = 1 
Red18: ^ number = Atom 
Red19: ^ number = Expr 
Shift: ^ number = Expr ; 
Red05: ^ Assn 
Red02: ^ Stmt 
Shift: ^ Stmt do 
Shift: ^ Stmt do { 
Shift: ^ Stmt do { if 
Shift: ^ Stmt do { if ( 
Shift: ^ Stmt do { if ( number 
Red17: ^ Stmt do { if ( Atom 
Red19: ^ Stmt do { if ( Expr 
Shift: ^ Stmt do { if ( Expr % 
Shift: ^ Stmt do { if ( Expr % 3
Red18: ^ Stmt do { if ( Expr % Atom 
Red19: ^ Stmt do { if ( Expr % Expr 
Red37: ^ Stmt do { if ( Bexp 
...
Red20: ^ Stmt do { Stmt Stmt } while Expr 
Shift: ^ Stmt do { Stmt Stmt } while Expr ; 
Red15: ^ Stmt Dowh 
Red10: ^ Stmt Ctrl 
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